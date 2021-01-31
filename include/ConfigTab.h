#ifndef BORGMATIC_UI_CONFIGTAB_H
#define BORGMATIC_UI_CONFIGTAB_H

#include <QString>
#include <QTabWidget>
#include <QWidget>
#include <memory>

#include "BackupConfig.h"
#include "BackupListModel.h"

namespace Ui {
class TabContent;
}

class ConfigTab : public QWidget {
  Q_OBJECT

 public:
  ConfigTab(std::shared_ptr<BackupConfig> config, QWidget *parent = nullptr);

  ~ConfigTab() override;

 private slots:
  void on_configEdit_textChanged(QString const &fileName);
  void on_configEditFileButton_clicked();
  void on_startBackupButton_clicked();
  void on_deleteConfigButton_clicked();
  void on_purgeCheckBox_stateChanged(int state);

 signals:
  void deleteTab(int index);

 private:
  QTabWidget *getTabWidget() const;
  void updateFromBackupConfig();

  Ui::TabContent *ui;
  BackupListModel *backupTableModel;
  std::shared_ptr<BackupConfig> backupConfig;
};

#endif  // BORGMATIC_UI_CONFIGTAB_H
