#include "ConfigTab.h"

#include <QFileDialog>
#include <iostream>

#include "ui_tabContent.h"

ConfigTab::ConfigTab(std::shared_ptr<BackupConfig> config, QWidget *parent)
    : QWidget(parent), ui(new Ui::TabContent), backupConfig(config) {
  ui->setupUi(this);
}

ConfigTab::~ConfigTab() { delete ui; }

void ConfigTab::on_configEdit_textChanged(const QString &fileName) {
  backupConfig->borgmaticConfig(fileName.toStdString());
}

void ConfigTab::on_configEditFileButton_clicked() {
  auto defaultPath = QDir::home();
  defaultPath.cd(".config/borgmatic");
  auto selectedFile =
      QFileDialog::getOpenFileName(this, "Borgmatic Config", defaultPath.absolutePath(), "YAML Files (*.yaml *.yml)");
  if (!selectedFile.isEmpty()) {
    ui->configEdit->setText(selectedFile);
    backupConfig->borgmaticConfig(selectedFile.toStdString());
    auto tabWidget = getTabWidget();
    auto index = tabWidget->indexOf(this);
    tabWidget->setTabText(index, QFileInfo(selectedFile).baseName());
  }
}

void ConfigTab::on_deleteConfigButton_clicked() {
  emit deleteTab(getTabWidget()->indexOf(this));
}

QTabWidget *ConfigTab::getTabWidget() const { return qobject_cast<QTabWidget *>(parentWidget()->parentWidget()); }
