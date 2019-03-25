#include <gui/analysis/form_fwhm_calibration.h>
//#include "widget_detectors.h"
#include "ui_form_fwhm_calibration.h"
#include <core/calibration/coef_function_factory.h>
#include <gui/widgets/qt_util.h>

#include <core/fitting/optimizers/optlib_adapter.h>

FormFwhmCalibration::FormFwhmCalibration(DAQuiri::Detector& dets,
                                         DAQuiri::Fitter& fit, QWidget* parent)
    : QWidget(parent)
      , ui(new Ui::FormFwhmCalibration)
      , fit_data_(fit)
      , detector_(dets)
{
  ui->setupUi(this);

  loadSettings();

  QColor point_color;
  QColor selected_color;

  point_color.setHsv(180, 215, 150, 40);
  style_pts.default_pen = QPen(point_color, 9);
  selected_color.setHsv(225, 255, 230, 210);
  style_pts.themes["selected"] = QPen(selected_color, 9);

  point_color.setHsv(180, 215, 150, 140);
  style_relevant.default_pen = QPen(point_color, 9);
  selected_color.setHsv(225, 255, 230, 210);
  style_relevant.themes["selected"] = QPen(selected_color, 9);

  style_fit.default_pen = QPen(Qt::darkCyan, 2);

  ui->PlotCalib->setAxisLabels("energy", "FWHM");

  ui->tablePeaks->verticalHeader()->hide();
  ui->tablePeaks->setColumnCount(7);
  ui->tablePeaks->setHorizontalHeaderLabels({"energy", "\u03C3", "%err",
                                             "fwhm", "\u03C3", "%err",
                                             "chi_sq_norm"});
  ui->tablePeaks->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tablePeaks->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui->tablePeaks->setEditTriggers(QTableView::NoEditTriggers);
  ui->tablePeaks->horizontalHeader()->setStretchLastSection(true);
  ui->tablePeaks->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tablePeaks->show();

  connect(ui->tablePeaks, SIGNAL(itemSelectionChanged()), this, SLOT(selection_changed_in_table()));
  connect(ui->PlotCalib, SIGNAL(selection_changed()), this, SLOT(selection_changed_in_plot()));
}

FormFwhmCalibration::~FormFwhmCalibration()
{
  delete ui;
}

bool FormFwhmCalibration::save_close()
{
  saveSettings();
  return true;
}

void FormFwhmCalibration::loadSettings()
{
  QSettings settings;

  settings.beginGroup("Program");
  data_directory_ = settings.value("save_directory", QDir::homePath() + "/qpx/data").toString();
  settings.endGroup();

  settings.beginGroup("FWHM_calibration");
  ui->spinTerms->setValue(settings.value("fit_function_terms", 2).toInt());
  ui->doubleMaxFitErr->setValue(settings.value("max_fit_err", 1).toDouble());
  ui->doubleMaxWidthErr->setValue(settings.value("max_width_err", 5).toDouble());

  settings.endGroup();
}

void FormFwhmCalibration::saveSettings()
{
  QSettings settings;
  settings.beginGroup("FWHM_calibration");
  settings.setValue("fit_function_terms", ui->spinTerms->value());
  settings.setValue("max_fit_err", ui->doubleMaxFitErr->value());
  settings.setValue("max_width_err", ui->doubleMaxWidthErr->value());
  settings.endGroup();
}

void FormFwhmCalibration::clear()
{
  new_calibration_ = DAQuiri::Calibration();
  ui->tablePeaks->clearContents();
  ui->tablePeaks->setRowCount(0);
  toggle_push();
  ui->PlotCalib->clearAll();
  ui->PlotCalib->replot();
  ui->pushApplyCalib->setEnabled(false);
  ui->pushFromDB->setEnabled(false);
}

void FormFwhmCalibration::newSpectrum()
{
  new_calibration_ = fit_data_.settings().calib.cali_fwhm_;
  update_data();
}

void FormFwhmCalibration::update_data()
{
  rebuild_table();
  replot_calib();

  if (fit_data_.peaks().empty())
    selected_peaks_.clear();

  select_in_table();
  select_in_plot();
  toggle_push();
}

void FormFwhmCalibration::rebuild_table()
{
  ui->tablePeaks->blockSignals(true);
  this->blockSignals(true);

  ui->tablePeaks->clearContents();
  ui->tablePeaks->setRowCount(fit_data_.peaks().size());
  int i = 0;

  auto calib = fit_data_.settings().calib;

  for (auto& q : fit_data_.peaks())
  {
    auto width = q.second.fwhm_energy(calib.cali_nrg_);
    bool significant = (width.error_percent() < ui->doubleMaxWidthErr->value());
    add_peak_to_table(q.second, i, significant);
    ++i;
  }

  ui->tablePeaks->blockSignals(false);
  this->blockSignals(false);
}

void FormFwhmCalibration::update_selection(std::set<double> selected_peaks)
{
  bool changed = (selected_peaks_ != selected_peaks);
  selected_peaks_ = selected_peaks;

  if (changed)
  {
    select_in_table();
    select_in_plot();
  }
}

void FormFwhmCalibration::select_in_table()
{
  ui->tablePeaks->blockSignals(true);
  this->blockSignals(true);
  ui->tablePeaks->clearSelection();

  QItemSelectionModel* selectionModel = ui->tablePeaks->selectionModel();
  QItemSelection itemSelection = selectionModel->selection();

  for (int i = 0; i < ui->tablePeaks->rowCount(); ++i)
    if (selected_peaks_.count(ui->tablePeaks->item(i, 0)->data(Qt::UserRole).toDouble()))
    {
      ui->tablePeaks->selectRow(i);
      itemSelection.merge(selectionModel->selection(), QItemSelectionModel::Select);
    }

  selectionModel->clearSelection();
  selectionModel->select(itemSelection, QItemSelectionModel::Select);

  ui->tablePeaks->blockSignals(false);
  this->blockSignals(false);
}

void FormFwhmCalibration::select_in_plot()
{
  auto calib = fit_data_.settings().calib;

  std::set<double> selected_energies;
  for (auto& p : fit_data_.peaks())
  {
    auto energy = p.second.peak_energy(calib.cali_nrg_);
    if (selected_peaks_.count(p.first))
      selected_energies.insert(energy.value());
  }
  ui->PlotCalib->set_selected_pts(selected_energies);
  ui->PlotCalib->replotAll();
}

void FormFwhmCalibration::add_peak_to_table(const DAQuiri::Peak& p, int row, bool gray)
{
  QBrush background(gray ? Qt::lightGray : Qt::white);

  auto calib = fit_data_.settings().calib;
  auto energy = p.peak_energy(calib.cali_nrg_);
  auto width = p.fwhm_energy(calib.cali_nrg_);

  // \todo reintroduce uncertainties
  add_to_table(ui->tablePeaks, row, 0,
               QString::number(energy.value()),
               QVariant::fromValue(p.id()), background);
  add_to_table(ui->tablePeaks, row, 1, QString::number(energy.sigma()), {}, background);
  add_to_table(ui->tablePeaks, row, 2, QS(energy.error_percent_fancy()), {}, background);
  add_to_table(ui->tablePeaks, row, 3, QString::number(width.value()), {}, background);
  add_to_table(ui->tablePeaks, row, 4, QString::number(width.sigma()), {}, background);
  add_to_table(ui->tablePeaks, row, 5, QS(width.error_percent_fancy()), {}, background);
  //add_to_table(ui->tablePeaks, row, 4, (p.good() ? "T" : "F"), QVariant(), background);
//  UncertainDouble chi_sq_norm(1, (1 - p.hypermet().chi2()), 2);
//  add_to_table(ui->tablePeaks, row, 5, chi_sq_norm.error_percent(), QVariant(), background);
}

void FormFwhmCalibration::replot_calib()
{
  ui->PlotCalib->clearAll();
  QVector<double> xx_relevant, yy_relevant,
      xx_relevant_sigma, yy_relevant_sigma;
  QVector<double> xx, yy;

  double xmin = std::numeric_limits<double>::max();
  double xmax = -std::numeric_limits<double>::max();

  auto calib = fit_data_.settings().calib;

  for (auto& q : fit_data_.peaks())
  {
    auto energy = q.second.peak_energy(calib.cali_nrg_);
    auto width = q.second.fwhm_energy(calib.cali_nrg_);

    double x = energy.value();
    double y = width.value();
    double x_sigma = energy.sigma();
    double y_sigma = width.sigma();

    if (width.error_percent() < ui->doubleMaxWidthErr->value())
    {
      xx_relevant.push_back(x);
      yy_relevant.push_back(y);
      xx_relevant_sigma.push_back(x_sigma);
      yy_relevant_sigma.push_back(y_sigma);
    }
    else
    {
      xx.push_back(x);
      yy.push_back(y);
    }

    if (x < xmin)
      xmin = x;
    if (x > xmax)
      xmax = x;
  }

  double x_margin = (xmax - xmin) / 10;
  xmax += x_margin;
  xmin -= x_margin;

  if (xx.size() > 0)
  {
    ui->PlotCalib->addPoints(style_pts, xx, yy, QVector<double>(), QVector<double>());
    ui->PlotCalib->addPoints(style_relevant, xx_relevant, yy_relevant, xx_relevant_sigma, yy_relevant_sigma);
    if (new_calibration_.valid())
    {

      double step = (xmax - xmin) / 50.0;
      xx.clear();
      yy.clear();

      for (double x = xmin; x < xmax; x += step)
      {
        double y = new_calibration_.transform(x);
        xx.push_back(x);
        yy.push_back(y);
      }

      ui->PlotCalib->setTitle("FWHM = " + QS(new_calibration_.function()->to_UTF8(5, true)));
      ui->PlotCalib->setFit(xx, yy, style_fit);
    }
  }
}

void FormFwhmCalibration::selection_changed_in_plot()
{
  std::set<double> selected_energies = ui->PlotCalib->get_selected_pts();
  selected_peaks_.clear();

  auto calib = fit_data_.settings().calib;

  for (auto& p : fit_data_.peaks())
  {
    auto energy = p.second.peak_energy(calib.cali_nrg_);
    if (selected_energies.count(energy.value()))
      selected_peaks_.insert(p.first);
  }
  select_in_table();
  if (isVisible())
      emit selection_changed(selected_peaks_);
  toggle_push();
}

void FormFwhmCalibration::selection_changed_in_table()
{
  selected_peaks_.clear();
      foreach (QModelIndex i,
               ui->tablePeaks->selectionModel()->selectedRows())selected_peaks_.insert(ui->tablePeaks->item(i.row(),
                                                                                                            0)->data(Qt::UserRole).toDouble());

  select_in_plot();
  if (isVisible())
      emit selection_changed(selected_peaks_);
  toggle_push();
}

void FormFwhmCalibration::toggle_push()
{
  if (detector_.get_calibration({"energy", fit_data_.detector_.id()},
                                {"fwhm", fit_data_.detector_.id()}).valid())
    ui->pushFromDB->setEnabled(true);
  else
    ui->pushFromDB->setEnabled(false);

  if (static_cast<int>(fit_data_.peaks().size()) > 1)
  {
    ui->spinTerms->setEnabled(true);
    ui->pushFit->setEnabled(true);
  }
  else
  {
    ui->pushFit->setEnabled(false);
    ui->spinTerms->setEnabled(false);
  }

  ui->pushApplyCalib->setEnabled(fit_data_.settings().calib.cali_fwhm_ != new_calibration_);
}

void FormFwhmCalibration::on_pushFit_clicked()
{
  fit_calibration();
  replot_calib();
  select_in_plot();
  toggle_push();
  emit new_fit();
}

void FormFwhmCalibration::fit_calibration()
{
  DAQuiri::OptlibOptimizer optimizer;

  auto calib = fit_data_.settings().calib;

  std::vector<double> xx, yy, xx_sigma, yy_sigma;
  for (auto& q : fit_data_.peaks())
  {
    auto energy = q.second.peak_energy(calib.cali_nrg_);
    auto width = q.second.peak_energy(calib.cali_nrg_);

    if (width.error_percent() < ui->doubleMaxWidthErr->value())
    {

//      DBG << "Adding point e=" << q.second.energy().to_string()
//          << " w=" << q.second.fwhm().to_string();
      xx.push_back(energy.value());
//      if (std::isfinite(q.second.energy().uncertainty()))
//        xx_sigma.push_back(q.second.energy().uncertainty());
//      else
      xx_sigma.push_back(0);
      yy.push_back(std::pow(width.value(), 2));
//      if (std::isfinite(q.second.fwhm().uncertainty()))
//        yy_sigma.push_back(2*q.second.fwhm().uncertainty()*q.second.fwhm().value());
//      else
      yy_sigma.push_back(0);
    }
  }

//  SqrtPoly p;
  auto p = DAQuiri::CoefFunctionFactory::singleton().create_type("Polynomial");
  for (int i = 0; i <= ui->spinTerms->value(); ++i)
    p->set_coeff(i, {0, 50, 0});

  //optimizer->fit(p, xx, yy, xx_sigma, yy_sigma);

  if (p->coeffs().size())
  {
    new_calibration_ = DAQuiri::Calibration({"energy", fit_data_.detector_.id(), "keV"},
                                            {"fwhm", fit_data_.detector_.id(), "keV"});
    new_calibration_.function(p);
  }
  else
    WARN("<WFHM calibration> DAQuiri::Calibration failed");
}

void FormFwhmCalibration::on_pushApplyCalib_clicked()
{
  emit update_detector();
}

void FormFwhmCalibration::on_pushFromDB_clicked()
{
  new_calibration_ = detector_.get_calibration({"energy", fit_data_.detector_.id()},
                                               {"fwhm", fit_data_.detector_.id()});
  replot_calib();
  select_in_plot();
  toggle_push();
  emit new_fit();
}

void FormFwhmCalibration::on_pushDetDB_clicked()
{
  // \todo bring it back
//  WidgetDetectors *det_widget = new WidgetDetectors(this);
//  det_widget->setData(detector_);
//  connect(det_widget, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));
//  det_widget->exec();
}

void FormFwhmCalibration::on_doubleMaxFitErr_valueChanged(double)
{
  replot_calib();
  select_in_plot();
}

void FormFwhmCalibration::on_doubleMaxWidthErr_valueChanged(double)
{
  replot_calib();
  select_in_plot();
}
