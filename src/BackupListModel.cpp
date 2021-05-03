#include "BackupListModel.h"

#include <spdlog/spdlog.h>

#include <QBrush>
#include <QColor>
#include <QDateTime>
#include <algorithm>

static QColor const MOUNTED_ROW_ODD = QColor::fromRgb(244, 237, 26);
static QColor const MOUNTED_ROW_EVEN = QColor::fromRgb(221, 215, 28);

QVariant BackupListModel::data(QModelIndex const &index, int role) const {
  if (index.row() > backups.size() - 1 ||
      (role != Qt::DisplayRole && role != Qt::ToolTipRole && role != Qt::BackgroundRole)) {
    return QVariant();
  }
  auto row = backups[index.row()];
  if (role == Qt::BackgroundRole) {
    if (row.is_mounted) {
      return QVariant(QBrush(QColor(index.row() % 2 ? MOUNTED_ROW_EVEN : MOUNTED_ROW_ODD)));
    } else {
      return QVariant();
    }
  }
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

void BackupListModel::updateBackups(std::vector<backup::helper::ListItem> const &data) {
  beginResetModel();
  auto oldBackups = backups;
  backups = data;
  for (backup::helper::ListItem &backupEntry : backups) {
    auto matchingOldEntry = std::ranges::find_if(
        oldBackups, [id = backupEntry.id](backup::helper::ListItem entry) { return entry.id == id; });
    if (matchingOldEntry != oldBackups.end()) {
      backupEntry.is_mounted = matchingOldEntry->is_mounted;
      backupEntry.mount_path = matchingOldEntry->mount_path;
    }
  }
  endResetModel();
}

backup::helper::ListItem const &BackupListModel::rowData(size_t row) const { return backups.at(row); }

void BackupListModel::setMountInfos(size_t row, bool is_mounted, const std::string &mount_point) {
  if (row > backups.size()) {
    spdlog::warn("Trying to update mount data: row index out of range: {}.", row);
    return;
  }
  backups.at(row).is_mounted = is_mounted;
  backups.at(row).mount_path = mount_point;
  emit dataChanged(index(row, 0), index(row, 1));
}
