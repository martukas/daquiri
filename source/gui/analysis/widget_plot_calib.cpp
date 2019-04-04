#include <gui/analysis/widget_plot_calib.h>
#include <QFlag>

#include <core/util/logger.h>

WidgetPlotCalib::WidgetPlotCalib(QWidget *parent) :
  QPlot::Multi1D(parent)
{
  setInteraction(QCP::iSelectItems, false);
  setInteraction(QCP::iMultiSelect, true);  
  setInteraction(QCP::iSelectPlottables, true);
  setSelectionTolerance(6);
//  setMultiSelectModifier(Qt::);
  setSelectionRectMode(QCP::srmSelect);

  scale_log_x_ = false;
  xAxis->setPadding(8);

  setScaleType("Linear");
  setPlotStyle("Lines");
  setGridStyle("Grid + subgrid");

  setVisibleOptions(QPlot::ShowOption::zoom | QPlot::ShowOption::save |
                    QPlot::ShowOption::scale | QPlot::ShowOption::title);
  replotExtras();
}

void WidgetPlotCalib::executeButton(QPlot::Button *button)
{
  if (button->name() == "scale_type_x")
  {
    scale_log_x_ = !scale_log_x_;
    apply_scale_type_x();
  }
  else
    QPlot::GenericPlot::executeButton(button);
}

void WidgetPlotCalib::clearPrimary()
{
  Multi1D::clearPrimary();
  fit_ = PointSet();
  points_.clear();
//  selection_.clear();
}

void WidgetPlotCalib::set_log_x(bool log)
{
  scale_log_x_ = log;
  apply_scale_type_x();
}

bool WidgetPlotCalib::log_x() const
{
  return scale_log_x_;
}

void WidgetPlotCalib::replotPrimary()
{
  plotPoints();
  plotFit();
}

void WidgetPlotCalib::plotFit()
{
  if ((fit_.y.size() != fit_.x.size()) || fit_.x.isEmpty())
    return;
  QCPGraph* new_graph = QCustomPlot::addGraph();
  new_graph->addData(fit_.x, fit_.y);
  new_graph->setPen(fit_.appearance.default_pen);
  new_graph->setLineStyle(QCPGraph::lsLine);
  new_graph->setScatterStyle(QCPScatterStyle::ssNone);
  new_graph->setSelectable(QCP::stNone);
}

void WidgetPlotCalib::plotPoints()
{
  for (const PointSet &p : points_)
  {
    if ((p.y.size() != p.x.size()) || p.x.isEmpty())
      continue;

    QPen pen = p.appearance.default_pen.color();
    QPen sel_pen = p.appearance.get_pen("selected").color();

    QCPScatterStyle unsel(QCPScatterStyle::ssCircle,
                          sel_pen.color(),
                          pen.color(),
                          9);

    QCPScatterStyle sel(QCPScatterStyle::ssCircle,
                          sel_pen.color(),
                          sel_pen.color(),
                          9);


    QCPGraph* new_graph = QCustomPlot::addGraph();
    new_graph->setPen(pen);
    new_graph->setLineStyle(QCPGraph::lsNone);
    new_graph->setScatterStyle(unsel);
    new_graph->setSelectable(QCP::stMultipleDataRanges);
    new_graph->selectionDecorator()->setScatterStyle(sel);
    new_graph->setData(p.x, p.y);

    if (p.y_sigma.size() == p.y.size())
    {
      QCPErrorBars *errorBars = new QCPErrorBars(xAxis, yAxis);
      errorBars->setAntialiased(false);
      errorBars->setErrorType(QCPErrorBars::etValueError);
      errorBars->setDataPlottable(new_graph);
      errorBars->setPen(pen);
      errorBars->setData(p.y_sigma);
    }
    if (p.x_sigma.size() == p.x.size())
    {
      QCPErrorBars *errorBars = new QCPErrorBars(xAxis, yAxis);
      errorBars->setAntialiased(false);
      errorBars->setErrorType(QCPErrorBars::etKeyError);
      errorBars->setDataPlottable(new_graph);
      errorBars->setPen(pen);
      errorBars->setData(p.x_sigma);
    }
  }
}

void WidgetPlotCalib::set_selected_pts(std::set<double> sel)
{
//  selection_ = sel;
}


std::set<double> WidgetPlotCalib::get_selected_pts()
{
  std::set<double> selection;

//  for (auto &q : selectedItems())
//    if (QCPItemTracer *txt = qobject_cast<QCPItemTracer*>(q))
//      selection_.insert(txt->property("true_value").toDouble());

  return selection;
}

void WidgetPlotCalib::setFit(const QVector<double>& x, const QVector<double>& y, QPlot::Appearance style)
{
  fit_.appearance = style;
  fit_.x.clear();
  fit_.y.clear();

  if (!x.empty() && (x.size() == y.size()))
  {
    fit_.x = x;
    fit_.y = y;
  }
}

void WidgetPlotCalib::addPoints(QPlot::Appearance style,
                                const QVector<double>& x, const QVector<double>& y,
                                const QVector<double>& x_sigma, const QVector<double>& y_sigma)
{
  if (!x.empty() && (x.size() == y.size()))
  {
    PointSet ps;
    ps.appearance = style;
    ps.x = x;
    ps.y = y;
    ps.x_sigma = x_sigma;
    ps.y_sigma = y_sigma;
    points_.push_back(ps);

  }
}

void WidgetPlotCalib::replotExtras()
{
  QPlot::Multi1D::replotExtras();
  plotExtraButtons();
}

void WidgetPlotCalib::plotExtraButtons()
{
  QPlot::Button *newButton {nullptr};

  newButton = new QPlot::Button(this,
                                scale_log_x_ ? QPixmap(":/icons/oxy/16/games_difficult.png")
                                             : QPixmap(":/icons/oxy/16/view_statistics.png"),
                                "scale_type_x", "Linear/Log X",
                                Qt::AlignBottom | Qt::AlignLeft);

  newButton->setClipToAxisRect(false);
  newButton->bottomRight->setType(QCPItemPosition::ptViewportRatio);
  newButton->bottomRight->setCoords(1, 1);
}

void WidgetPlotCalib::apply_scale_type_x()
{
  if (scale_log_x_)
    xAxis->setScaleType(QCPAxis::stLogarithmic);
  else
    xAxis->setScaleType(QCPAxis::stLinear);

  for (int i=0; i < itemCount(); ++i)
  {
    QCPAbstractItem *q =  item(i);
    if (QPlot::Button *button = qobject_cast<QPlot::Button*>(q))
    {
      if (button->name() == "scale_type_x")
      {
        if (!scale_log_x_)
          button->setPixmap(QPixmap(":/icons/oxy/16/view_statistics.png"));
        else
          button->setPixmap(QPixmap(":/icons/oxy/16/games_difficult.png"));
        break;
      }
    }
  }
  //  plot_rezoom(true);
  replot();
}
