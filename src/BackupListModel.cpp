#include "BackupListModel.h"

#include <QDateTime>

QVariant BackupListModel::data(QModelIndex const &index, int role) const {
  if (index.row() > backups.size()-1 || (role != Qt::DisplayRole && role  != Qt::ToolTipRole)) {
    return QVariant();
  }
  auto row = backups[index.row()];
  switch (index.column()) {
    case 0:
      return QVariant(row.name.c_str());
    case 1:
      return QVariant(QDateTime::fromString(row.start.c_str(), Qt::ISODateWithMs));
    default:
      return QVariant();
  }
}

QVariant BackupListModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Orientation::Vertical || role != Qt::DisplayRole) {
    return QVariant();
  }
  switch (section) {
    case 0:
      return QVariant("Backup Name");
    case 1:
      return QVariant("Datum");
    default:
      return QVariant();
  }
}

int BackupListModel::rowCount(QModelIndex const &parent) const { return backups.size(); }

int BackupListModel::columnCount(QModelIndex const &parent) const { return 2; }

void BackupListModel::updateBackups(const std::vector<backup::helper::ListItem> &data) {
  beginResetModel();
  backups = data;
  endResetModel();
}
