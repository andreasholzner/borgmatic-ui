#include "ConfigTab.h"

#include <spdlog/spdlog.h>

#include <QDesktopServices>
#include <QLocale>
#include <QString>
#include <QUrl>

#include "ui_tabContent.h"

ConfigTab::ConfigTab(std::shared_ptr<BackupConfig> config, std::shared_ptr<FileDialogWrapper> fileDialogWrapper,
                     QWidget *parent)
    : QWidget(parent),
      ui(new Ui::TabContent),
      backupTableModel(new BackupListModel),
      backupConfig(config),
      file_dialog_wrapper_(fileDialogWrapper) {
  ui->setupUi(this);
  ui->backupsTableView->setModel(backupTableModel);
  ui->backupsTableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::Stretch);
  ui->backupsTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::ResizeToContents);
  ui->configEdit->setText(backupConfig->borgmaticConfigFile().c_str());

  connect(ui->backupsTableView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
          &ConfigTab::tableSelectionChanged);
}

ConfigTab::~ConfigTab() {
  delete ui;
  delete backupTableModel;
}

void ConfigTab::on_configEdit_textChanged(QString const &fileName) {
  auto tabWidget = getTabWidget();
  if (tabWidget) {
    auto index = tabWidget->indexOf(this);
    tabWidget->setTabText(index, QFileInfo(fileName).baseName());
  }

  backupConfig->borgmaticConfigFile(fileName.toStdString());
  updateFromBackupConfig();
}

void ConfigTab::on_configEditFileButton_clicked() {
  auto selectedFile = file_dialog_wrapper_->selectBorgmaticConfigFile(this);
  if (!selectedFile.isEmpty()) {
    ui->configEdit->setText(selectedFile);
  }
}

void ConfigTab::on_configShowFileButton_clicked() {
  auto selectedFile = backupConfig->borgmaticConfigFile();
  spdlog::debug("Opening config file '{}' in default editor...", selectedFile);
  QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(selectedFile)));
}

void ConfigTab::on_deleteConfigButton_clicked() { emit deleteTab(getTabWidget()->indexOf(this)); }

void ConfigTab::on_startBackupButton_clicked() {
  backupConfig->startBackup([this]() { backupFinished(); },
                            [this](std::string const &line) { emit setStatusMessage(line.c_str()); });
  ui->startBackupButton->setEnabled(false);
  ui->cancelBackupButton->setEnabled(true);
}

void ConfigTab::on_cancelBackupButton_clicked() {
  backupConfig->cancelBackup();
  updateFromBackupConfig();
  ui->startBackupButton->setEnabled(true);
  ui->cancelBackupButton->setEnabled(false);
}

void ConfigTab::on_purgeCheckBox_stateChanged(int state) {
  backupConfig->isBackupPurging(state == Qt::CheckState::Checked);
}

void ConfigTab::on_backupMountButton_clicked() {
  if (!ui->backupsTableView->selectionModel()->hasSelection()) {
    spdlog::warn("MountButton used without valid selection.");
    return;
  }
  auto dir = file_dialog_wrapper_->selectMountPoint(this);
  if (dir.isEmpty()) {
    spdlog::warn("Invalid directory selected as mount point.");
    return;
  }
  auto backupName = backupTableModel->rowData(ui->backupsTableView->selectionModel()->currentIndex().row()).name;
  spdlog::debug("selected backup name: {}", backupName);

  backupConfig->mountArchive(backupName, dir.toStdString());
}

void ConfigTab::on_backupUmountButton_clicked() {
  if (!isRowSelected()) {
    spdlog::warn("UmountButton used without valid selection.");
    return;
  }
}

void ConfigTab::tableSelectionChanged(QItemSelection const &current, QItemSelection const &previous) {
  if (ui->backupsTableView->selectionModel()->hasSelection()) {
    spdlog::debug("row changed. new row: {}", ui->backupsTableView->selectionModel()->currentIndex().row());
    ui->backupMountButton->setEnabled(true);
  } else {
    ui->backupMountButton->setDisabled(true);
  }
}

void ConfigTab::backupFinished() {
  emit setStatusMessage("Backup is done", 30000);
  spdlog::debug("Backup is done");
  updateFromBackupConfig();
  ui->startBackupButton->setEnabled(true);
  ui->cancelBackupButton->setDisabled(true);
}

QTabWidget *ConfigTab::getTabWidget() const {
  return parentWidget() ? qobject_cast<QTabWidget *>(parentWidget()->parentWidget()) : nullptr;
}

void ConfigTab::updateFromBackupConfig() {
  ui->purgeCheckBox->setCheckState(backupConfig->isBackupPurging() ? Qt::CheckState::Checked
                                                                   : Qt::CheckState::Unchecked);
  auto info = backupConfig->info();
  ui->infoLocationLabel->setText(info.location.c_str());
  QLocale locale;
  ui->infoOriginalSizeLabel->setText(info.originalSize ? locale.formattedDataSize(info.originalSize) : "-");
  ui->infoCompressedSizeLabel->setText(info.compressedSize ? locale.formattedDataSize(info.compressedSize) : "-");

  backupTableModel->updateBackups(backupConfig->list());
}
