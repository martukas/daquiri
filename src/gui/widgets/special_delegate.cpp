#include "special_delegate.h"
#include "setting.h"
#include "qt_util.h"
#include <QComboBox>
#include <QPainter>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QFileDialog>
#include <QFileInfo>
#include <QBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QDateTimeEdit>
#include <QPushButton>
#include <QColorDialog>
#include "time_duration_widget.h"
#include "color_selector.hpp"

#include <QApplication>

using namespace color_widgets;
using namespace DAQuiri;

void DAQuiriSpecialDelegate::set_detectors(const Container<Detector> &detectors)
{
  detectors_ = detectors;
}

void DAQuiriSpecialDelegate::paint(QPainter *painter,
                                   const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
  if ((option.state & QStyle::State_Selected) &&
      !(option.state & QStyle::State_HasFocus))
  {
    QStyledItemDelegate::paint(painter, option, index);
    return;
  }

  if (!index.data().canConvert<Setting>())
  {
    QStyledItemDelegate::paint(painter, option, index);
    return;
  }
  Setting itemData = qvariant_cast<Setting>(index.data());

  if (itemData.is(SettingType::command))
  {
    QStyleOptionButton button;
    button.rect = option.rect;
    button.text = QString::fromStdString(itemData.metadata().get_string("name", ""));
    if (!itemData.metadata().has_flag("readonly"))
      button.state = QStyle::State_Enabled;
    QApplication::style()->drawControl( QStyle::CE_PushButton, &button, painter);
  }
  else if (itemData.is(SettingType::pattern))
  {
    DAQuiriPatternEditor pat(itemData.pattern(), 20, 8);
    pat.setEnabled(!itemData.metadata().has_flag("readonly"));
    if (option.state & QStyle::State_Selected)
      painter->fillRect(option.rect, option.palette.highlight());
    pat.paint(painter, option.rect, option.palette);
  }
  else
  {
    int flags = Qt::TextWordWrap | Qt::AlignVCenter;

    std::string raw_txt = itemData.val_to_pretty_string();
    if (raw_txt.size() > 32)
      raw_txt = raw_txt.substr(0,32) + "...";
    raw_txt = " " + raw_txt + " ";
    QString text = QString::fromStdString(raw_txt);

    painter->save();

    if (itemData.is(SettingType::indicator))
    {
      auto ii = itemData.find(Setting(itemData.metadata().enum_name(itemData.get_number())));
      QColor bkgCol = QColor(QString::fromStdString(ii.metadata().get_string("color", "")));
      painter->fillRect(option.rect, bkgCol);
      painter->setPen(QPen(Qt::white, 3));
      QFont f = painter->font();
      f.setBold(true);
      painter->setFont(f);
      flags |= Qt::AlignCenter;
    }
    else if (itemData.is(SettingType::color))
    {
      QColor c(QString::fromStdString(itemData.get_text()));
      QBrush b;
      b.setTexture(QPixmap(QStringLiteral(":/color_widgets/alphaback.png")));
      if (c.alpha() < 255)
        painter->fillRect(option.rect, b);
      painter->fillRect(option.rect, c);
      QColor inverse = QColor::fromRgb(255 - c.red(),
                                       255 - c.green(),
                                       255 - c.blue());
      painter->setPen(QPen(inverse, 4));
      QFont f = painter->font();
      f.setBold(true);
      painter->setFont(f);
      flags |= Qt::AlignCenter;
    }
    else if (itemData.is(SettingType::detector))
    {
      QVector<QColor> palette {Qt::darkCyan, Qt::darkBlue, Qt::darkGreen, Qt::darkRed, Qt::darkYellow, Qt::darkMagenta, Qt::red, Qt::blue};
      uint16_t index = 0;
      if (itemData.indices().size())
        index = *itemData.indices().begin();
      painter->setPen(QPen(palette[index % palette.size()], 2));
      QFont f = painter->font();
      f.setBold(true);
      painter->setFont(f);
    }
    else
    {
      if (option.state & QStyle::State_Selected)
      {
        painter->fillRect(option.rect, option.palette.highlight());
        painter->setPen(option.palette.highlightedText().color());
      } else {
        if (itemData.metadata().has_flag("readonly"))
          painter->setPen(option.palette.color(QPalette::Disabled, QPalette::Text));
        else
          painter->setPen(option.palette.color(QPalette::Active, QPalette::Text));
      }
    }

    painter->drawText(option.rect, flags, text);
    painter->restore();
  }
}

QSize DAQuiriSpecialDelegate::sizeHint(const QStyleOptionViewItem &option,
                                       const QModelIndex &index) const
{
  if (!index.data().canConvert<Setting>())
    return QStyledItemDelegate::sizeHint(option, index);

  Setting itemData = qvariant_cast<Setting>(index.data());

  if (itemData.is(SettingType::command))
  {
    QPushButton button;
    button.setText(QString::fromStdString(itemData.metadata().get_string("name","")));
    return button.sizeHint();
  }
  else if (itemData.is(SettingType::time))
  {
    QDateTimeEdit editor;
    editor.setCalendarPopup(true);
    editor.setTimeSpec(Qt::UTC);
    editor.setDisplayFormat("yyyy-MM-dd HH:mm:ss.zzz");
    QSize sz = editor.sizeHint();
    sz.setWidth(sz.width() + 20);
    return sz;
  }
  else if (itemData.is(SettingType::pattern))
  {
    DAQuiriPatternEditor pattern(itemData.pattern(), 20, 8);
    return pattern.sizeHint();
  }
  else if (itemData.is(SettingType::duration))
  {
    TimeDurationWidget editor;
    return editor.sizeHint();
  }
  else
  {
    std::string raw_txt = itemData.val_to_pretty_string();
    if (raw_txt.size() > 32)
      raw_txt = raw_txt.substr(0,32) + "...";
    raw_txt = " " + raw_txt + " ";
    QString text = QString::fromStdString(raw_txt);

    QRect r = option.rect;
    QFontMetrics fm(QApplication::font());
    QRect qr = fm.boundingRect(r, Qt::AlignLeft | Qt::AlignVCenter, text);
    QSize size(qr.size());
    return size;
  }

  return QStyledItemDelegate::sizeHint(option, index);
}

QWidget *DAQuiriSpecialDelegate::createEditor(QWidget *parent,
                                              const QStyleOptionViewItem &option,
                                              const QModelIndex &index) const

{
  emit begin_editing();

  if (!index.data(Qt::EditRole).canConvert<Setting>())
    return QStyledItemDelegate::createEditor(parent, option, index);

  Setting set = qvariant_cast<Setting>(index.data(Qt::EditRole));
  if (set.is(SettingType::floating) || set.is(SettingType::precise))
    return new QDoubleSpinBox(parent);
  else if (set.is(SettingType::integer))
    return new QSpinBox(parent);
  else if (set.is(SettingType::duration))
    return new TimeDurationWidget(parent);
  else if (set.is(SettingType::text))
    return new QLineEdit(parent);
  else if (set.is(SettingType::binary))
  {
    emit ask_binary(set, index);
    return nullptr;
  }
  else if (set.is(SettingType::command) &&
           !set.metadata().has_flag("readonly"))
  {
    emit ask_execute(set, index);
    return nullptr;
  }
  else if (set.is(SettingType::color))
  {
    ColorSelector *editor = new ColorSelector(parent);
    editor->setDisplayMode(ColorPreview::AllAlpha);
    editor->setUpdateMode(ColorSelector::Confirm);
    return editor;
  }
  else if (set.is(SettingType::time))
  {
    QDateTimeEdit *editor = new QDateTimeEdit(parent);
    editor->setCalendarPopup(true);
    editor->setTimeSpec(Qt::UTC);
    editor->setDisplayFormat("yyyy-MM-dd HH:mm:ss.zzz");
    return editor;
  }
  else if (set.is(SettingType::pattern))
  {
    DAQuiriPatternEditor *editor = new DAQuiriPatternEditor(parent);
    editor->set_pattern(set.pattern(), 20, 8);
    return editor;
  }
  else if (set.is(SettingType::detector))
  {
    QComboBox *editor = new QComboBox(parent);
    editor->addItem("none", "none");
    for (size_t i=0; i < detectors_.size(); i++)
    {
      QString name = QString::fromStdString(detectors_.get(i).name());
      editor->addItem(name, name);
    }
    return editor;
  }
  else if (set.is(SettingType::boolean))
  {
    QComboBox *editor = new QComboBox(parent);
    editor->addItem("True", QVariant::fromValue(true));
    editor->addItem("False", QVariant::fromValue(false));
    return editor;
  }
  else if (set.is(SettingType::file))
  {
    QFileDialog *editor = new QFileDialog(parent, QString("Chose File"),
                                          QFileInfo(QString::fromStdString(set.get_text())).dir().absolutePath(),
                                          QString::fromStdString(set.metadata().get_string("wildcards","")));
    editor->setFileMode(QFileDialog::ExistingFile);
    return editor;
  }
  else if (set.is(SettingType::dir))
  {
    QFileDialog *editor = new QFileDialog(parent, QString("Chose Directory"),
                                          QFileInfo(QString::fromStdString(set.get_text())).dir().absolutePath());
    editor->setFileMode(QFileDialog::Directory);
    return editor;
  }
  else if (set.is(SettingType::menu))
  {
    QComboBox *editor = new QComboBox(parent);
    for (auto &q : set.metadata().enum_map())
      editor->addItem(QString::fromStdString(q.second),
                      QVariant::fromValue(q.first));
    return editor;
  }

  return QStyledItemDelegate::createEditor(parent, option, index);
}

void DAQuiriSpecialDelegate::setEditorData(QWidget *editor,
                                           const QModelIndex &index) const
{
  if (!index.data(Qt::EditRole).canConvert<Setting>())
  {
    QStyledItemDelegate::setEditorData(editor, index);
    return;
  }
  Setting set = qvariant_cast<Setting>(index.data(Qt::EditRole));

  if (QComboBox *cb = qobject_cast<QComboBox*>(editor))
  {
    if (set.is(SettingType::detector))
    {
      int cbIndex = cb->findText(QString::fromStdString(set.get_text()));
      if(cbIndex >= 0)
        cb->setCurrentIndex(cbIndex);
    }
    else if (set.is(SettingType::boolean))
    {
      int cbIndex = cb->findData(QVariant::fromValue(set.get_number() != 0));
      if(cbIndex >= 0)
        cb->setCurrentIndex(cbIndex);
    }
    else if (set.is(SettingType::menu) &&
             set.metadata().enum_map().count(set.get_number()))
    {
      int cbIndex = cb->findText(QString::fromStdString(set.metadata().enum_name(set.get_number())));
      if(cbIndex >= 0)
        cb->setCurrentIndex(cbIndex);
    }
  }
  else if (QDoubleSpinBox *sb = qobject_cast<QDoubleSpinBox*>(editor))
  {
    sb->setRange(set.metadata().min<double>(), set.metadata().max<double>());
    sb->setSingleStep(set.metadata().step<double>());
    sb->setDecimals(6); //generalize
    sb->setValue(set.get_number());
  }
  else if (QSpinBox *sb = qobject_cast<QSpinBox*>(editor))
  {
    sb->setRange(set.metadata().min<int32_t>(),
                 set.metadata().max<int32_t>());
    sb->setSingleStep(set.metadata().step<int32_t>());
    sb->setValue(static_cast<int32_t>(set.get_number()));
  }
  else if (QDateTimeEdit *dte = qobject_cast<QDateTimeEdit*>(editor))
    dte->setDateTime(fromBoostPtime(set.time()));
  else if (TimeDurationWidget *dte = qobject_cast<TimeDurationWidget*>(editor))
    dte->set_duration(set.duration());
  else if (ColorSelector *cp = qobject_cast<ColorSelector*>(editor))
    cp->setColor(QString::fromStdString(set.get_text()));
  else if (QLineEdit *le = qobject_cast<QLineEdit*>(editor))
    le->setText(QString::fromStdString(set.get_text()));
  else
    QStyledItemDelegate::setEditorData(editor, index);
}

void DAQuiriSpecialDelegate::setModelData(QWidget *editor,
                                          QAbstractItemModel *model,
                                          const QModelIndex &index) const
{
  if (DAQuiriPatternEditor *pe = qobject_cast<DAQuiriPatternEditor*>(editor))
    model->setData(index, QVariant::fromValue(pe->pattern()), Qt::EditRole);
  else if (QComboBox *cb = qobject_cast<QComboBox*>(editor))
    model->setData(index, cb->currentData(), Qt::EditRole);
  else if (QDoubleSpinBox *sb = qobject_cast<QDoubleSpinBox*>(editor))
    model->setData(index, QVariant::fromValue(sb->value()), Qt::EditRole);
  else if (QSpinBox *sb = qobject_cast<QSpinBox*>(editor))
    model->setData(index, QVariant::fromValue(sb->value()), Qt::EditRole);
  else if (QLineEdit *le = qobject_cast<QLineEdit*>(editor))
    model->setData(index, le->text(), Qt::EditRole);
  else if (QDateTimeEdit *dte = qobject_cast<QDateTimeEdit*>(editor))
    model->setData(index, dte->dateTime(), Qt::EditRole);
  else if (TimeDurationWidget *dte = qobject_cast<TimeDurationWidget*>(editor))
    model->setData(index, QVariant::fromValue(dte->get_duration()), Qt::EditRole);
  else if (ColorSelector *cp = qobject_cast<ColorSelector*>(editor))
    model->setData(index, cp->color().name(QColor::HexArgb), Qt::EditRole);
  else if (QCheckBox *cb = qobject_cast<QCheckBox*>(editor))
    model->setData(index, QVariant::fromValue(cb->isChecked()), Qt::EditRole);
  else if (QFileDialog *fd = qobject_cast<QFileDialog*>(editor)) {
    if ((!fd->selectedFiles().isEmpty()) /*&& (validateFile(parent, fd->selectedFiles().front(), false))*/)
      model->setData(index, QVariant::fromValue(fd->selectedFiles().front()), Qt::EditRole);
  }
  else
    QStyledItemDelegate::setModelData(editor, model, index);
}

void DAQuiriSpecialDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  editor->setGeometry(option.rect);
}