#include "ConfigTab.h"

#include <QFileDialog>
#include <QLocale>

#include "ui_tabContent.h"

ConfigTab::ConfigTab(std::shared_ptr<BackupConfig> config, QWidget *parent)
    : QWidget(parent), ui(new Ui::TabContent), backupTableModel(new BackupListModel), backupConfig(config) {
  ui->setupUi(this);
  ui->backupsTableView->setModel(backupTableModel);
  ui->backupsTableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::Stretch);
  ui->backupsTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::ResizeToContents);
  updateFromBackupConfig();
}

ConfigTab::~ConfigTab() {
  delete ui;
  delete backupTableModel;
}

void ConfigTab::on_configEdit_textChanged(QString const &fileName) {
  backupConfig->borgmaticConfigFile(fileName.toStdString());
}

void ConfigTab::on_configEditFileButton_clicked() {
  auto defaultPath = QDir::home();
  defaultPath.cd(".config/borgmatic");
  auto selectedFile =
      QFileDialog::getOpenFileName(this, "Borgmatic Config", defaultPath.absolutePath(), "YAML Files (*.yaml *.yml)");
  if (!selectedFile.isEmpty()) {
    backupConfig->borgmaticConfigFile(selectedFile.toStdString());
    auto tabWidget = getTabWidget();
    auto index = tabWidget->indexOf(this);
    tabWidget->setTabText(index, QFileInfo(selectedFile).baseName());
    updateFromBackupConfig();
  }
}

void ConfigTab::on_deleteConfigButton_clicked() { emit deleteTab(getTabWidget()->indexOf(this)); }

void ConfigTab::on_startBackupButton_clicked() {}

QTabWidget *ConfigTab::getTabWidget() const { return qobject_cast<QTabWidget *>(parentWidget()->parentWidget()); }

void ConfigTab::updateFromBackupConfig() {
  ui->configEdit->setText(backupConfig->borgmaticConfigFile().c_str());
  auto info = backupConfig->info();
  ui->infoLocationLabel->setText(info.location.c_str());
  QLocale locale;
  ui->infoOriginalSizeLabel->setText(locale.formattedDataSize(info.originalSize));
  ui->infoCompressedSizeLabel->setText(locale.formattedDataSize(info.compressedSize));

  backupTableModel->updateBackups(backupConfig->list());
}
