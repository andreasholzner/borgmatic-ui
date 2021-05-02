#ifndef BORGMATIC_UI_CONFIGTAB_H
#define BORGMATIC_UI_CONFIGTAB_H

#include <QFuture>
#include <QString>
#include <QTabWidget>
#include <QWidget>
#include <memory>

#include "BackupConfig.h"
#include "BackupListModel.h"
#include "DesktopServicesWrapper.h"

namespace Ui {
class TabContent;
}

class ConfigTab : public QWidget {
  Q_OBJECT

 public:
  ConfigTab(std::shared_ptr<BackupConfig> config, std::shared_ptr<DesktopServicesWrapper> desktopServicesWrapper,
            QWidget *parent = nullptr);

  ~ConfigTab() override;

 public slots:
  void onCurrentTabChanged(int index);

 private slots:
  void on_configEdit_textChanged(QString const &fileName);
  void on_configEditFileButton_clicked();
  void on_configShowFileButton_clicked();
  void on_startBackupButton_clicked();
  void on_cancelBackupButton_clicked();
  void on_deleteConfigButton_clicked();
  void on_purgeCheckBox_stateChanged(int state);
  void on_openMountPointCheckBox_stateChanged(int state);
  void on_backupMountButton_clicked();
  void on_backupUmountButton_clicked();
  void tableSelectionChanged(QItemSelection const &current, QItemSelection const &previous);
  void backupFinished();
  void updateBackupInfos();
  void updateBackupList();

 signals:
  void deleteTab(int index);
  void setStatusMessage(QString const &message, int timeout = 0);

 private:
  QTabWidget *getTabWidget() const;
  void updateFromBackupConfig();
  bool isRowSelected() const;

  Ui::TabContent *ui;
  BackupListModel *backupTableModel;
  std::shared_ptr<BackupConfig> backupConfig;
  std::shared_ptr<DesktopServicesWrapper> desktop_services_wrapper_;
  QFuture<backup::helper::Info> info_future_;
  QFutureWatcher<backup::helper::Info> info_watcher_;
  QFuture<std::vector<backup::helper::ListItem>> list_future_;
  QFutureWatcher<std::vector<backup::helper::ListItem>> list_watcher_;
};

#endif  // BORGMATIC_UI_CONFIGTAB_H
