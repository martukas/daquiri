#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QInputDialog>
#include <gui/analysis/widget_isotopes.h>
#include "ui_widget_isotopes.h"

#include <core/gamma/nuclides/std_importer.h>

#include <core/util/logger.h>
#include <gui/widgets/QFileExtensions.h>

#include <gui/Profiles.h>
#include <core/util/json_file.h>

TableGammas::TableGammas(QObject* parent) :
    QAbstractTableModel(parent)
{
}

int TableGammas::rowCount(const QModelIndex& /*parent*/) const
{
  return gammas_.size();
}

int TableGammas::columnCount(const QModelIndex& /*parent*/) const
{
  return 2;
}

QVariant TableGammas::data(const QModelIndex& index, int role) const
{
  int col = index.column();
  int row = index.row();
  if (role == Qt::DisplayRole)
  {
    switch (col)
    {
      case 0:
        return QString::number(gammas_[row].energy.value())
            + "\u00B1" + QString::number(gammas_[row].energy.sigma());
      case 1:
        return QString::number(gammas_[row].abundance.value())
            + "\u00B1" + QString::number(gammas_[row].abundance.sigma());
    }
  }
  else if (role == Qt::EditRole)
  {
    switch (col)
    {
      case 0:
        return QVariant::fromValue(gammas_[row].energy);
      case 1:
        return QVariant::fromValue(gammas_[row].abundance);
    }
  }
  else if (role == Qt::ForegroundRole)
  {
    if (gammas_[row].marked)
      return QBrush(Qt::darkGreen);
    else
      return QBrush(Qt::black);
  }
  return QVariant();
}

QVariant TableGammas::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::DisplayRole)
  {
    if (orientation == Qt::Horizontal)
    {
      switch (section)
      {
        case 0:
          return QString("Energy");
        case 1:
          return QString("Intensity");
      }
    }
    else if (orientation == Qt::Vertical)
    {
      return QString::number(section);
    }
  }
  return QVariant();
}

void TableGammas::set_gammas(const Container<DAQuiri::Radiation>& newgammas)
{
  gammas_ = newgammas.to_vector();

  QModelIndex start_ix = createIndex(0, 0);
  QModelIndex end_ix = createIndex((rowCount() - 1), (columnCount() - 1));
  emit dataChanged(start_ix, end_ix);
  emit layoutChanged();
}

Container<DAQuiri::Radiation> TableGammas::get_gammas()
{
  Container<DAQuiri::Radiation> gammas;
  gammas.from_vector(gammas_);
  return gammas;
}

void TableGammas::clear()
{
  set_gammas({});
  QModelIndex start_ix = createIndex(0, 0);
  QModelIndex end_ix = createIndex((rowCount() - 1), (columnCount() - 1));
  emit dataChanged(start_ix, end_ix);
  emit layoutChanged();
}

Qt::ItemFlags TableGammas::flags(const QModelIndex& index) const
{
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | QAbstractTableModel::flags(index);
}

bool TableGammas::setData(const QModelIndex& index, const QVariant& value, int role)
{
  int row = index.row();
  int col = index.column();
  QModelIndex ix = createIndex(row, col);

  if (role == Qt::EditRole)
  {
    if (col == 0)
      gammas_[row].energy.set_value(value.toDouble());
    else if (col == 1)
      gammas_[row].abundance.set_value(value.toDouble());
  }

  QModelIndex start_ix = createIndex(row, col);
  QModelIndex end_ix = createIndex(row, col);
  emit dataChanged(start_ix, end_ix);
  //emit layoutChanged();
  emit energiesChanged();
  return true;
}

WidgetIsotopes::WidgetIsotopes(QWidget* parent)
    : QWidget(parent)
      , ui(new Ui::WidgetIsotopes)
      , table_gammas_(this)
      , special_delegate_(this)
      , sort_model_(this)
{
  ui->setupUi(this);

  ui->tableGammas->setModel(&table_gammas_);
  ui->tableGammas->setItemDelegate(&special_delegate_);
  ui->tableGammas->verticalHeader()->hide();
  ui->tableGammas->setSortingEnabled(true);
  ui->tableGammas->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableGammas->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui->tableGammas->horizontalHeader()->setStretchLastSection(true);
  ui->tableGammas->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableGammas->show();

  sort_model_.setSourceModel(&table_gammas_);
  sort_model_.setSortRole(Qt::EditRole);
  ui->tableGammas->setModel(&sort_model_);

  connect(ui->tableGammas->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
          this, SLOT(selection_changed(QItemSelection, QItemSelection)));

  connect(ui->listIsotopes, SIGNAL(currentTextChanged(QString)), this, SLOT(isotopeChosen(QString)));
  connect(&table_gammas_, SIGNAL(energiesChanged()), this, SLOT(energies_changed()));

//  auto filename = Profiles::settings_dir().toStdString() + "/gamma/isotopes.json";
//  nlohmann::json isotopes_json = from_json_file(filename);
////  WARN("Got isotopes from {}:\n {}", filename, isotopes_json.dump(0));
//  isotopes_ = isotopes_json["isotopes"];
  on_pushImport_clicked();

  ui->listIsotopes->clear();
  for (const auto& i : isotopes_)
    ui->listIsotopes->addItem(QString::fromStdString(i.name));
}

WidgetIsotopes::~WidgetIsotopes()
{
  delete ui;
}

void WidgetIsotopes::set_editable(bool enable)
{
  ui->groupEnergies->setVisible(enable);
  ui->groupIsotopes->setVisible(enable);
  if (enable)
  {
    ui->tableGammas->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->tableGammas->setEditTriggers(QAbstractItemView::AllEditTriggers);
  }
  else
  {
    ui->tableGammas->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableGammas->setEditTriggers(QAbstractItemView::NoEditTriggers);
  }
}

std::list<DAQuiri::Radiation> WidgetIsotopes::current_isotope_gammas() const
{
  if (ui->listIsotopes->currentItem() != nullptr)
    return isotopes_.get(DAQuiri::Isotope(ui->listIsotopes->currentItem()->text().toStdString())).gammas.data();
  else
    return std::list<DAQuiri::Radiation>();
}

void WidgetIsotopes::isotopeChosen(QString choice)
{

  ui->labelPeaks->setText("Energies");
  ui->pushRemoveIsotope->setEnabled(false);
  ui->pushAddGamma->setEnabled(false);
  table_gammas_.clear();

  DAQuiri::Isotope iso(choice.toStdString());
  if (isotopes_.has_a(iso))
  {
    ui->labelPeaks->setText(choice);
    ui->pushRemoveIsotope->setEnabled(true);
    iso = isotopes_.get(iso);
    table_gammas_.set_gammas(iso.gammas);
    ui->pushAddGamma->setEnabled(true);
  }
  selection_changed(QItemSelection(), QItemSelection());

  emit isotopeSelected();
}

void WidgetIsotopes::selection_changed(QItemSelection, QItemSelection)
{
  current_gammas_.clear();

      foreach (QModelIndex idx, ui->tableGammas->selectionModel()->selectedRows())
    {
      QModelIndex nrg_ix = table_gammas_.index(idx.row(), 0);
      current_gammas_.push_back(qvariant_cast<UncertainDouble>(table_gammas_.data(nrg_ix, Qt::UserRole)));
    }

  ui->pushSum->setEnabled(false);

  if (current_gammas_.size() > 0)
    ui->pushRemove->setEnabled(true);
  if (current_gammas_.size() > 1)
    ui->pushSum->setEnabled(true);

  emit energiesSelected();

}

std::vector<UncertainDouble> WidgetIsotopes::current_gammas() const
{
  return current_gammas_;
}

void WidgetIsotopes::on_pushSum_clicked()
{
  UncertainDouble sum_energy{0.0, 0.0};
  for (const auto& q : current_gammas_)
    sum_energy += q;
  INFO("sum={}", sum_energy.to_string(false));

  DAQuiri::Radiation newgamma;
  newgamma.energy = sum_energy;
  newgamma.abundance = {0.0, 0.0};
  DAQuiri::Isotope modified = isotopes_.get(DAQuiri::Isotope(ui->listIsotopes->currentItem()->text().toStdString()));
  modified.gammas.add(newgamma);
  INFO("modifying {} to have {} gammas", modified.name, modified.gammas.size());
  isotopes_.replace(modified);
  isotopeChosen(QString::fromStdString(modified.name));

  modified_ = true;
}

void WidgetIsotopes::on_pushRemove_clicked()
{
  DAQuiri::Isotope modified = isotopes_.get(DAQuiri::Isotope(ui->listIsotopes->currentItem()->text().toStdString()));
  for (const auto& g : current_gammas_)
    modified.gammas.remove_a({g, {0.0, 0.0}});
  isotopes_.replace(modified);
  isotopeChosen(QString::fromStdString(modified.name));

  modified_ = true;
}

QString WidgetIsotopes::current_isotope() const
{
  if (ui->listIsotopes->currentItem() != nullptr)
    return ui->listIsotopes->currentItem()->text();
  else
    return "";
}

void WidgetIsotopes::set_current_isotope(QString choice)
{
  for (int i = 0; i < ui->listIsotopes->count(); ++i)
    if (ui->listIsotopes->item(i)->text() == choice)
      ui->listIsotopes->setCurrentRow(i);
}

bool WidgetIsotopes::save_close()
{

  auto filename = Profiles::settings_dir().toStdString() + "/gamma/isotopes.json";

  nlohmann::json isotopes_json;
  isotopes_json["daquiri_git_version"] = std::string(BI_GIT_HASH);
  isotopes_json["isotopes"] = isotopes_;

  to_json_file(isotopes_json, filename);

/*  if (modified_)
  {
    int reply = QMessageBox::warning(this, "Isotopes modified",
                                     "Save?",
                                     QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
      to_json_file(isotopes_json, filename);
      modified_ = false;
    } else if (reply == QMessageBox::Cancel) {
      return false;
    }
  }*/

  return true;
}

void WidgetIsotopes::on_pushAddGamma_clicked()
{
  DAQuiri::Isotope modified = isotopes_.get(DAQuiri::Isotope(current_isotope().toStdString()));
  modified.gammas.add_a(DAQuiri::Radiation({1.0, 0.0}, {0.0, 0.0}));
  isotopes_.replace(modified);
  isotopeChosen(QString::fromStdString(modified.name));
  modified_ = true;
}

void WidgetIsotopes::on_pushRemoveIsotope_clicked()
{
  table_gammas_.clear();
  isotopes_.remove_a(DAQuiri::Isotope(current_isotope().toStdString()));
  ui->listIsotopes->clear();
  for (const auto& i : isotopes_)
    ui->listIsotopes->addItem(QString::fromStdString(i.name));
  modified_ = true;
  isotopeChosen("");
}

void WidgetIsotopes::on_pushAddIsotope_clicked()
{
  bool ok;
  QString text = QInputDialog::getText(this, "New Isotope",
                                       "Isotope name:", QLineEdit::Normal,
                                       "", &ok);
  if (ok && !text.isEmpty())
  {
    if (isotopes_.has_a(DAQuiri::Isotope(text.toStdString())))
      QMessageBox::warning(this, "Already exists", "Isotope " + text + " already exists", QMessageBox::Ok);
    else
    {
      isotopes_.add(DAQuiri::Isotope(text.toStdString()));
      ui->listIsotopes->clear();
      for (const auto& i : isotopes_)
        ui->listIsotopes->addItem(QString::fromStdString(i.name));
      modified_ = true;
      ui->listIsotopes->setCurrentRow(ui->listIsotopes->count() - 1);
    }
  }
}

void WidgetIsotopes::on_pushImport_clicked()
{
  auto filename = Profiles::settings_dir().toStdString() + "/gamma/nuclides/Nuclid.std";
  DAQuiri::NuclideStdImporter importer;
  importer.open(filename);
  importer.parse_nuclides();

  for (const auto& n : importer.nuclides)
    isotopes_.add(n);
}

void WidgetIsotopes::energies_changed()
{
  DAQuiri::Isotope modified = isotopes_.get(DAQuiri::Isotope(current_isotope().toStdString()));
  modified.gammas = table_gammas_.get_gammas();
  isotopes_.replace(modified);
  modified_ = true;
}

void WidgetIsotopes::push_energies(std::vector<UncertainDouble> new_energies)
{
  DAQuiri::Isotope modified = isotopes_.get(DAQuiri::Isotope(current_isotope().toStdString()));
  for (const auto& q : new_energies)
    modified.gammas.add(DAQuiri::Radiation(q, {}));
  isotopes_.replace(modified);
  modified_ = true;
  isotopeChosen(current_isotope());
}

void WidgetIsotopes::select_energies(std::set<UncertainDouble> sel_energies)
{
  Container<DAQuiri::Radiation> gammas = table_gammas_.get_gammas();
  for (auto& q : gammas)
    q.marked = (sel_energies.count(q.energy) > 0);
  table_gammas_.set_gammas(gammas);
}

void WidgetIsotopes::select_next_energy()
{
  current_gammas_.clear();

  int top_row = -1;
      foreach (QModelIndex idx, ui->tableGammas->selectionModel()->selectedRows())
    {
      if (idx.row() > top_row)
        top_row = idx.row();
    }

  top_row++;
  if (top_row < table_gammas_.rowCount())
  {
    ui->tableGammas->clearSelection();
    ui->tableGammas->selectRow(top_row);
  }

  emit energiesSelected();
}



