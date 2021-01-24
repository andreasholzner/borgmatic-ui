#ifndef BORGMATIC_UI_CONFIGTAB_H
#define BORGMATIC_UI_CONFIGTAB_H

#include <QTabWidget>
#include <QWidget>
#include <memory>

#include "BackupConfig.h"

namespace Ui {
class TabContent;
}

class ConfigTab : public QWidget {
  Q_OBJECT

 public:
  ConfigTab(std::shared_ptr<BackupConfig> config, QWidget *parent = nullptr);

  ~ConfigTab() override;

 private slots:
  void on_configEdit_textChanged(const QString &fileName);
  void on_configEditFileButton_clicked();
  void on_deleteConfigButton_clicked();

 signals:
  void deleteTab(int index);

 private:
  QTabWidget *getTabWidget() const;
  Ui::TabContent *ui;
  std::shared_ptr<BackupConfig> backupConfig;
};

#endif  // BORGMATIC_UI_CONFIGTAB_H
