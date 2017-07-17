#include "tree_settings.h"
#include "widget_pattern.h"
#include "qt_util.h"
#include <QDateTime>
#include "bin_hex_print.h"

Q_DECLARE_METATYPE(Setting)
Q_DECLARE_METATYPE(boost::posix_time::time_duration)

TreeItem::TreeItem(const Setting &data, TreeItem *parent)
{
  parentItem = parent;
  itemData = data;

  if (itemData.is(SettingType::stem))
  {
    childItems.resize(itemData.branches.size());
    for (size_t i=0; i < itemData.branches.size(); ++i)
      childItems[i] = new TreeItem(itemData.branches.get(i), this);
  }
}

bool TreeItem::eat_data(const Setting &data)
{
  itemData = data;

  if (itemData.is(SettingType::stem))
  {
    if (static_cast<int>(itemData.branches.size()) != childItems.size())
      return false;
    for (size_t i=0; i < itemData.branches.size(); ++i)
    {
      Setting s = itemData.branches.get(i);
      if (childItems[i]->itemData.id() != s.id())
        return false;
      if (!childItems[i]->eat_data(itemData.branches.get(i)))
        return false;
    }
  }
  return (!itemData.is(SettingType::none));

  /*if (itemData.branches.size() != childItems.size()) {
      WARN << "Setting branch size mismatch " << itemData.id();
//      for (int i=0; i <  childItems.size(); ++i)
//        delete childItems.takeAt(i);

      qDeleteAll(childItems);
      childItems.resize(itemData.branches.size());
      for (int i=0; i < itemData.branches.size(); ++i)
        childItems[i] = new TreeItem(itemData.branches.get(i), this);
    } else {
      for (int i=0; i < itemData.branches.size(); ++i) {
        Setting s = itemData.branches.get(i);
        if (childItems[i]->itemData.id() == s.id())
          childItems[i]->eat_data(itemData.branches.get(i));
        else {
          delete childItems.takeAt(i);
          childItems[i] = new TreeItem(itemData.branches.get(i), this);
        }
      }
    }*/
}

TreeItem::~TreeItem()
{
  parentItem == nullptr;
  qDeleteAll(childItems);
}

TreeItem *TreeItem::child(int number)
{
  return childItems.value(number);
}

int TreeItem::childCount() const
{
  return childItems.count();
}

int TreeItem::childNumber() const
{
  if (parentItem && !parentItem->childItems.empty())
    return parentItem->childItems.indexOf(const_cast<TreeItem*>(this));

  return 0;
}

int TreeItem::columnCount() const
{
  return 6; //name, indices, value, units, address, description
}

QVariant TreeItem::display_data(int column) const
{
  if (column == 0)
  {
    QString name;
    if (!itemData.metadata().get_string("name", "").empty())
      name = QString::fromStdString(itemData.metadata().get_string("name", ""));
    else
      name = QString::fromStdString(itemData.id());
    return name;
  }
  else if (column == 1)
  {
    QString ret = QString::fromStdString(itemData.indices_to_string(true));
    if (ret.size())
      return "  " + ret + "  ";
    else
      return QVariant();
  }
  else if ((column == 2) &&
           !itemData.is(SettingType::none) &&
           !itemData.is(SettingType::stem))
  {
    return QVariant::fromValue(itemData);
  }
  else if (column == 3)
    return QString::fromStdString(itemData.metadata().get_string("unit",""));
  else if (column == 4)
  {
    QString text;
    if (itemData.metadata().get_num("address", -1) >= 0)
      text +=  "0x" + QString::fromStdString(itohex32(itemData.metadata().get_num("address", -1)));
    else if (itemData.metadata().get_num("address", -1) != -1)
      text += QString::number(itemData.metadata().get_num("address", -1));
    return text;
  }
  else if (column == 5)
    return QString::fromStdString(itemData.metadata().get_string("description",""));
  else
    return QVariant();
}


QVariant TreeItem::edit_data(int column) const
{
  if (column == 2)
    return QVariant::fromValue(itemData);
  else if ((column == 1) &&
           (itemData.metadata().get_num("chans", 0) > 0))
  {
    QList<QVariant> qlist;
    qlist.push_back(QVariant::fromValue(itemData.metadata().get_num("chans", 0)));
    for (auto &q : itemData.indices())
      qlist.push_back(q);
    return qlist;
  } else
    return QVariant();
}

bool TreeItem::is_editable(int column) const
{
  if ((column == 1)  && (itemData.metadata().get_num("chans", 0) > 0))
    return true;
  else if ((column != 2) ||
           itemData.is(SettingType::stem) ||
           itemData.is(SettingType::indicator))
    return false;
  else if (itemData.is(SettingType::binary))
    return true;
  else
    return ((column == 2) && (!itemData.metadata().has_flag("readonly")));
}




/*bool TreeItem::insertChildren(int position, int count, int columns)
{
  if (position < 0 || position > childItems.size())
    return false;

  std::string name = itemData.name;
  int start = itemData.branches.size();
  SettingType type = SettingType::integer;
  if (start) {
    name = itemData.branches.get(0).name;
    type = itemData.branches.get(0).metadata.setting_type;
  }
  for (int row = start; row < (start + count); ++row) {
    Setting newsetting;
    newsetting.name = name;
    newsetting.metadata.setting_type = type;
    itemData.branches.add(newsetting);
    TreeItem *item = new TreeItem(newsetting, this);
    childItems.insert(position, item);
  }

  return true;
}

*/

TreeItem *TreeItem::parent()
{
  return parentItem;
}

Setting TreeItem::rebuild()
{
  Setting root = itemData;

  root.branches.clear();
  for (int i=0; i < childCount(); i++)
    root.branches.add_a(child(i)->rebuild());
  return root;
}

/*bool TreeItem::removeChildren(int position, int count)
{
  if (position < 0 || position + count > childItems.size())
    return false;

  for (int row = 0; row < count; ++row)
    delete childItems.takeAt(position);

  return true;
}

*/

bool TreeItem::setData(int column, const QVariant &value)
{
  if ((column == 1)  && (itemData.metadata().get_num("chans", 0) > 0))
  {
    QString val = value.toString();
    QStringList ilist = val.split(QRegExp("\\W+"), QString::SkipEmptyParts);
    if (ilist.isEmpty())
    {
      itemData.clear_indices();
      return true;
    }

    std::set<int32_t> new_indices;
    foreach (QString idx, ilist)
    {
      bool ok;
      int i = idx.toInt(&ok);
      if (ok && (i >= 0) &&
          (static_cast<int>(new_indices.size()) < itemData.metadata().get_num("chans", 0)))
        new_indices.insert(i);
    }

    bool diff = (new_indices.size() != itemData.indices().size());
    for (auto &q : new_indices)
      if (!itemData.has_index(q))
        diff = true;

    if (!diff)
      return false;

    itemData.set_indices(new_indices);
    return true;
  }

  if (column != 2)
    return false;


  if (itemData.is(SettingType::integer)
      && (value.canConvert(QMetaType::LongLong)))
    itemData.set_number(value.toLongLong());
  else if ((itemData.is(SettingType::binary) ||
       itemData.is(SettingType::menu) ||
       itemData.is(SettingType::command))
      && value.canConvert(QMetaType::LongLong))
    itemData.select(value.toLongLong());
  else if (itemData.is(SettingType::boolean)
      && (value.type() == QVariant::Bool))
    itemData.select(value.toBool());
  else if (itemData.is(SettingType::floating)
      && (value.type() == QVariant::Double))
    itemData.set_number(value.toDouble());
  else if (itemData.is(SettingType::precise)
      && (value.type() == QVariant::Double))
    itemData.set_number(value.toDouble());
  else if (itemData.is(SettingType::text)
      && (value.type() == QVariant::String))
    itemData.set_text(value.toString().toStdString());
  else if (itemData.is(SettingType::color)
      && (value.type() == QVariant::String))
    itemData.set_text(value.toString().toStdString());
  else if (itemData.is(SettingType::file)
      && (value.type() == QVariant::String))
    itemData.set_text(value.toString().toStdString());
  else if (itemData.is(SettingType::pattern)
      && (value.canConvert<Pattern>()))
  {
    Pattern qpxPattern = qvariant_cast<Pattern>(value);
    itemData.set_pattern(qpxPattern);
  }
  else if (itemData.is(SettingType::dir)
      && (value.type() == QVariant::String))
    itemData.set_text(value.toString().toStdString());
  else if (itemData.is(SettingType::detector)
      && (value.type() == QVariant::String))
    itemData.set_text(value.toString().toStdString());
  else if (itemData.is(SettingType::time)
      && (value.type() == QVariant::DateTime))
    itemData.set_time(fromQDateTime(value.toDateTime()));
  else if (itemData.is(SettingType::duration)
      && (value.canConvert<boost::posix_time::time_duration>()))
    itemData.set_duration(qvariant_cast<boost::posix_time::time_duration>(value));
  else
    return false;

  return true;
}






TreeSettings::TreeSettings(QObject *parent)
  : QAbstractItemModel(parent), show_address_(true)
{
  rootItem = new TreeItem(Setting());
  show_read_only_ = true;
  edit_read_only_ = false;
}

TreeSettings::~TreeSettings()
{
  delete rootItem;
}

void TreeSettings::set_edit_read_only(bool edit_ro)
{
  edit_read_only_ = edit_ro;
  emit layoutChanged();
}

void TreeSettings::set_show_read_only(bool show_ro)
{
  show_read_only_ = show_ro;
}

void TreeSettings::set_show_address_(bool show_ad)
{
  show_address_ = show_ad;
  emit layoutChanged();
}

int TreeSettings::columnCount(const QModelIndex & /* parent */) const
{
  if (show_address_)
    return rootItem->columnCount();
  else
    return rootItem->columnCount() - 1;
}

QVariant TreeSettings::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();
  TreeItem *item = getItem(index);

  int row = index.row();
  int col = index.column();

  if (role == Qt::DisplayRole) {
    if (!show_address_ && (col == 4))
      col = 5;
    return item->display_data(col);
  }
  else if (role == Qt::EditRole)
    return item->edit_data(col);
  else if (role == Qt::ForegroundRole)
  {
    if (item->edit_data(2).canConvert<Setting>())
    {
      Setting set = qvariant_cast<Setting>(item->edit_data(2));
      if ((col == 1) && (set.metadata().get_num("chans", 0) < 1))
      {
        QBrush brush(Qt::darkGray);
        return brush;
      }
      else
      {
        QBrush brush(Qt::black);
        return brush;
      }
    }
  }
  else
    return QVariant();
}

Qt::ItemFlags TreeSettings::flags(const QModelIndex &index) const
{
  if (!index.isValid())
    return 0;

  TreeItem *item = getItem(index);
  if (edit_read_only_ && (index.column() == 2))
    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
  else if (item->is_editable(index.column()))
    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
  else
    return QAbstractItemModel::flags(index);
}

TreeItem *TreeSettings::getItem(const QModelIndex &index) const
{
  if (index.isValid())
  {
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    if (item)
      return item;
  }
  return rootItem;
}

QVariant TreeSettings::headerData(int section, Qt::Orientation orientation,
                                  int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    if (section == 0)
      return "setting";
    else if (section == 1)
      return "indices";
    else if (section == 2)
      return "value";
    else if (section == 3)
      return "units";
    else if (section == 4)
      if (show_address_)
        return "address";
      else
        return "notes";
    else if (section == 5)
      return "notes";

  return QVariant();
}

QModelIndex TreeSettings::index(int row, int column, const QModelIndex &parent) const
{
  if (parent.isValid() && parent.column() != 0)
    return QModelIndex();

  TreeItem *parentItem = getItem(parent);

  TreeItem *childItem = parentItem->child(row);
  if (childItem)
    return createIndex(row, column, childItem);
  else
    return QModelIndex();
}

/*bool TreeSettings::insertRows(int position, int rows, const QModelIndex &parent)
{
  TreeItem *parentItem = getItem(parent);
  bool success;

  beginInsertRows(parent, position, position + rows - 1);
  success = parentItem->insertChildren(position, rows, rootItem->columnCount());
  endInsertRows();

  return success;
}*/

QModelIndex TreeSettings::parent(const QModelIndex &index) const
{
  if (!index.isValid())
    return QModelIndex();

  TreeItem *childItem = getItem(index);

  if (!childItem)
    return QModelIndex();

  if (childItem == 0x0)
    return QModelIndex();

  TreeItem *parentItem = childItem->parent();

  if (parentItem == rootItem)
    return QModelIndex();

  if (!parentItem)
    return QModelIndex();

  if (parentItem == 0x0)
    return QModelIndex();

  //DBG << parentItem;

  //DBG << "Index r" << index.row() << " c" << index.column() << " d=";
//  if (index.data(Qt::EditRole).canConvert<Setting>()) {
//    Setting set = qvariant_cast<Setting>(index.data(Qt::EditRole));
//    DBG << "id=" << set.id();
//  }

  return createIndex(parentItem->childNumber(), 0, parentItem);
}

/*bool TreeSettings::removeRows(int position, int rows, const QModelIndex &parent)
{
  TreeItem *parentItem = getItem(parent);
  bool success = true;

  beginRemoveRows(parent, position, position + rows - 1);
  success = parentItem->removeChildren(position, rows);
  endRemoveRows();

  return success;
}*/

int TreeSettings::rowCount(const QModelIndex &parent) const
{
  TreeItem *parentItem = getItem(parent);

  if (parentItem != nullptr)
    return parentItem->childCount();
  else
    return 0;
}

bool TreeSettings::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if (role != Qt::EditRole)
    return false;

  TreeItem *item = getItem(index);
  bool result = item->setData(index.column(), value);

  if (result)
  {
    if (index.column() == 2)
    {
      Setting set = qvariant_cast<Setting>(index.data(Qt::EditRole));
      data_ = rootItem->rebuild();

      emit dataChanged(index, index);
      if (set.is(SettingType::detector))
        emit detector_chosen(index.row() - 1, set.get_text());
      else
        emit tree_changed();
    }
    else if (index.column() == 1)
    {
      data_ = rootItem->rebuild();
      emit dataChanged(index, index);
      emit tree_changed();
    }
  }
  return result;
}

const Setting & TreeSettings::get_tree()
{
  return data_;
}

bool TreeSettings::setHeaderData(int section, Qt::Orientation orientation,
                                 const QVariant &value, int role)
{
  if (role != Qt::EditRole || orientation != Qt::Horizontal)
    return false;

  bool result = rootItem->setData(section, value);

  if (result)
  {
    emit headerDataChanged(orientation, section, section);
    //emit push settings to device
  }

  return result;
}


void TreeSettings::update(const Setting &data)
{
  data_ = data;
  data_.cull_hidden();
  if (!show_read_only_)
  {
    //DBG << "Culling read only";
    data_.cull_readonly();
  }

  if (!rootItem->eat_data(data_))
  {
//    DBG << "deleting root node";
    beginResetModel();
    delete rootItem;
    rootItem = new TreeItem(data_);
    endResetModel();
  }

  emit layoutChanged();
}
