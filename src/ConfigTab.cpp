#include "ConfigTab.h"

#include <QFileDialog>
#include <QLocale>
#include <QThread>

#include "ui_tabContent.h"

ConfigTab::ConfigTab(std::shared_ptr<BackupConfig> config, QWidget *parent)
    : QWidget(parent), ui(new Ui::TabContent), backupTableModel(new BackupListModel), backupConfig(config) {
  ui->setupUi(this);
  ui->backupsTableView->setModel(backupTableModel);
  ui->backupsTableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::Stretch);
  ui->backupsTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::ResizeToContents);
  updateFromBackupConfig();

  connect(ui->backupsTableView->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
          &ConfigTab::tableRowChanged);
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

void ConfigTab::on_startBackupButton_clicked() {
  backupConfig->startBackup([this]() { backupFinished(); }, [](std::string const &line) { spdlog::debug(line); });
  ui->startBackupButton->setEnabled(false);
  ui->cancelBackupButton->setEnabled(true);
}

void ConfigTab::on_cancelBackupButton_clicked() {
  backupConfig->cancelBackup();
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
  auto dir = QFileDialog::getExistingDirectory(this, "Backup Mountpoint");
  if (dir.isEmpty()) {
    spdlog::warn("Invalid directory selected as mount point.");
    return;
  }
  auto backupName = backupTableModel->rowData(ui->backupsTableView->selectionModel()->currentIndex().row()).name;
  spdlog::debug("selected backup name: {}", backupName);
}

void ConfigTab::on_backupUmountButton_clicked() {}

void ConfigTab::tableRowChanged(const QModelIndex &current, const QModelIndex &previous) {
  if (ui->backupsTableView->selectionModel()->hasSelection()) {
    spdlog::debug("row changed. new row: {}", current.row());
    ui->backupMountButton->setEnabled(true);
    ui->backupDeleteButton->setEnabled(true);
  } else {
    ui->backupMountButton->setDisabled(true);
    ui->backupDeleteButton->setDisabled(true);
  }
}

void ConfigTab::backupFinished() { spdlog::debug("Backup is done"); }

QTabWidget *ConfigTab::getTabWidget() const { return qobject_cast<QTabWidget *>(parentWidget()->parentWidget()); }

void ConfigTab::updateFromBackupConfig() {
  ui->configEdit->setText(backupConfig->borgmaticConfigFile().c_str());
  ui->purgeCheckBox->setCheckState(backupConfig->isBackupPurging() ? Qt::CheckState::Checked
                                                                   : Qt::CheckState::Unchecked);
  auto info = backupConfig->info();
  ui->infoLocationLabel->setText(info.location.c_str());
  QLocale locale;
  ui->infoOriginalSizeLabel->setText(locale.formattedDataSize(info.originalSize));
  ui->infoCompressedSizeLabel->setText(locale.formattedDataSize(info.compressedSize));

  backupTableModel->updateBackups(backupConfig->list());
}
