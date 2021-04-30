#ifndef BORGMATIC_UI_INCLUDE_BACKUPLISTMODEL_H_
#define BORGMATIC_UI_INCLUDE_BACKUPLISTMODEL_H_

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QVariant>
#include <vector>

#include "BackupConfig.h"

class BackupListModel : public QAbstractTableModel {
  Q_OBJECT

 public:
  QVariant data(QModelIndex const &index, int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  int rowCount(QModelIndex const &parent = QModelIndex()) const override;
  int columnCount(QModelIndex const &parent = QModelIndex()) const override;
  void updateBackups(std::vector<backup::helper::ListItem> const &data);
  backup::helper::ListItem const &rowData(int row) const;

 private:
  std::vector<backup::helper::ListItem> backups;
};

#endif  // BORGMATIC_UI_INCLUDE_BACKUPLISTMODEL_H_
