#ifndef BORGMATIC_UI_CONFIGTAB_H
#define BORGMATIC_UI_CONFIGTAB_H

#include <QString>
#include <QTabWidget>
#include <QWidget>
#include <memory>

#include "BackupConfig.h"
#include "BackupListModel.h"
#include "FileDialogWrapper.h"

namespace Ui {
class TabContent;
}

class ConfigTab : public QWidget {
  Q_OBJECT

 public:
  ConfigTab(std::shared_ptr<BackupConfig> config, std::shared_ptr<FileDialogWrapper> fileDialogWrapper,
            QWidget *parent = nullptr);

  ~ConfigTab() override;

 private slots:
  void on_configEdit_textChanged(QString const &fileName);
  void on_configEditFileButton_clicked();
  void on_configShowFileButton_clicked();
  void on_startBackupButton_clicked();
  void on_cancelBackupButton_clicked();
  void on_deleteConfigButton_clicked();
  void on_purgeCheckBox_stateChanged(int state);
  void on_backupMountButton_clicked();
  void on_backupUmountButton_clicked();
  void tableRowChanged(QModelIndex const &current, QModelIndex const &previous);
  void backupFinished();

 signals:
  void deleteTab(int index);
  void setStatusMessage(QString const &message, int timeout = 0);

 private:
  QTabWidget *getTabWidget() const;
  void updateFromBackupConfig();

  Ui::TabContent *ui;
  BackupListModel *backupTableModel;
  std::shared_ptr<BackupConfig> backupConfig;
  std::shared_ptr<FileDialogWrapper> file_dialog_wrapper_;
  QString currentMountPath;
};

#endif  // BORGMATIC_UI_CONFIGTAB_H
