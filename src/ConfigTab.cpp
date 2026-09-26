#include "ConfigTab.h"

#include <spdlog/spdlog.h>

#include <QLocale>
#include <QString>
#include <QUrl>
#include <QtConcurrent/QtConcurrent>

#include "ui_tabContent.h"

ConfigTab::ConfigTab(std::shared_ptr<BackupConfig> config,
                     std::shared_ptr<DesktopServicesWrapper> desktopServicesWrapper, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::TabContent),
      backupTableModel(new BackupListModel),
      backupConfig(config),
      desktop_services_wrapper_(desktopServicesWrapper) {
  ui->setupUi(this);
  ui->backupsTableView->setModel(backupTableModel);
  ui->backupsTableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::Stretch);
  ui->backupsTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::ResizeToContents);

  refreshTimer_.setSingleShot(true);
  refreshTimer_.setInterval(refreshDelayMs);
  connect(&refreshTimer_, &QTimer::timeout, this, &ConfigTab::updateFromBackupConfig);
  connect(ui->backupsTableView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
          &ConfigTab::tableSelectionChanged);
  connect(&info_watcher_, &QFutureWatcher<backup::helper::Info>::finished, this, &ConfigTab::updateBackupInfos);
  connect(&list_watcher_, &QFutureWatcher<std::vector<backup::helper::ListItem>>::finished, this,
          &ConfigTab::updateBackupList);

  ui->configEdit->setText(backupConfig->borgmaticConfigFile().c_str());
  // The initial load doesn't need to wait for further edits.
  refreshTimer_.stop();
  updateFromBackupConfig();
}

ConfigTab::~ConfigTab() {
  // The backup's output handler and completion callback refer to this tab.
  if (backupRunning_) {
    backupConfig->cancelBackupAndWait();
  }
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
  refreshTimer_.start();
}

void ConfigTab::on_configEditFileButton_clicked() {
  auto selectedFile = desktop_services_wrapper_->selectBorgmaticConfigFile(this);
  if (!selectedFile.isEmpty()) {
    ui->configEdit->setText(selectedFile);
  }
}

void ConfigTab::on_configShowFileButton_clicked() {
  auto selectedFile = backupConfig->borgmaticConfigFile();
  spdlog::debug("Opening config file '{}' in default editor...", selectedFile);
  desktop_services_wrapper_->openLocation(QString::fromStdString(selectedFile));
}

void ConfigTab::on_deleteConfigButton_clicked() { emit deleteTab(getTabWidget()->indexOf(this)); }

bool ConfigTab::isBackupRunning() const { return backupRunning_; }

bool ConfigTab::hasMountedArchives() const {
  for (int row = 0; row != backupTableModel->rowCount(); ++row) {
    if (backupTableModel->rowData(row).is_mounted) {
      return true;
    }
  }
  return false;
}

bool ConfigTab::umountAllArchives() {
  bool allUmounted = true;
  for (int row = 0; row != backupTableModel->rowCount(); ++row) {
    auto const &archive = backupTableModel->rowData(row);
    if (!archive.is_mounted) {
      continue;
    }
    if (backupConfig->umountArchive(archive.mount_path)) {
      backupTableModel->setMountInfos(row, false, "");
    } else {
      allUmounted = false;
    }
  }
  return allUmounted;
}

void ConfigTab::on_startBackupButton_clicked() {
  backupRunning_ = true;
  backupCancelled_ = false;
  ui->startBackupButton->setEnabled(false);
  ui->cancelBackupButton->setEnabled(true);
  backupConfig->startBackup([this](int exitCode) { backupFinished(exitCode); },
                            [this](std::string const &line) { emit setStatusMessage(line.c_str()); });
}

void ConfigTab::on_cancelBackupButton_clicked() {
  // Start is re-enabled and the UI refreshed in backupFinished, once borgmatic has actually exited and released the
  // repository lock.
  backupCancelled_ = true;
  backupConfig->cancelBackup();
  ui->cancelBackupButton->setEnabled(false);
}

void ConfigTab::on_purgeCheckBox_stateChanged(int state) {
  backupConfig->isBackupPurging(state == Qt::CheckState::Checked);
}

void ConfigTab::on_openMountPointCheckBox_stateChanged(int state) {
  backupConfig->isMountPointToBeOpened(state == Qt::CheckState::Checked);
}

void ConfigTab::on_backupMountButton_clicked() {
  if (!isRowSelected()) {
    spdlog::warn("MountButton used without valid selection.");
    return;
  }
  auto dir = desktop_services_wrapper_->selectMountPoint(this);
  if (dir.isEmpty()) {
    spdlog::warn("Invalid directory selected as mount point.");
    ui->backupsTableView->selectionModel()->clearSelection();
    return;
  }
  size_t row = ui->backupsTableView->selectionModel()->currentIndex().row();
  auto archive = backupTableModel->rowData(row);
  ui->backupsTableView->selectionModel()->clearSelection();
  spdlog::debug("Mounting archive {} to mount point {}", archive.name, dir.toStdString());

  runMountOperation(
      QString("Mounting %1 ...").arg(archive.name.c_str()), archive.id,
      [config = backupConfig, name = archive.name, mountPoint = dir.toStdString()] {
        return config->mountArchive(name, mountPoint);
      },
      [this, name = archive.name, dir](bool mounted, std::optional<size_t> row) {
        if (!mounted) {
          emit setStatusMessage(QString("Mounting %1 failed").arg(name.c_str()));
          return;
        }
        emit setStatusMessage(QString("Mounted %1").arg(name.c_str()), 30000);
        if (row) {
          backupTableModel->setMountInfos(*row, true, dir.toStdString());
        }
        if (backupConfig->isMountPointToBeOpened()) {
          desktop_services_wrapper_->openLocation(dir);
        }
      });
}

void ConfigTab::on_backupUmountButton_clicked() {
  if (!isRowSelected()) {
    spdlog::warn("UmountButton used without valid selection.");
    return;
  }
  size_t row = ui->backupsTableView->selectionModel()->currentIndex().row();
  auto archive = backupTableModel->rowData(row);
  ui->backupsTableView->selectionModel()->clearSelection();
  spdlog::debug("Umounting mount point: {}", archive.mount_path);

  runMountOperation(
      QString("Unmounting %1 ...").arg(archive.mount_path.c_str()), archive.id,
      [config = backupConfig, mountPoint = archive.mount_path] { return config->umountArchive(mountPoint); },
      [this, mountPoint = archive.mount_path](bool umounted, std::optional<size_t> row) {
        if (!umounted) {
          emit setStatusMessage(QString("Unmounting %1 failed").arg(mountPoint.c_str()));
          return;
        }
        emit setStatusMessage(QString("Unmounted %1").arg(mountPoint.c_str()), 30000);
        if (row) {
          backupTableModel->setMountInfos(*row, false, "");
        }
      });
}

void ConfigTab::runMountOperation(QString const &message, std::string const &archiveId,
                                  std::function<bool()> operation,
                                  std::function<void(bool, std::optional<size_t>)> onFinished) {
  mountOperationRunning_ = true;
  updateMountButtons();
  emit setStatusMessage(message);

  // The watcher is a child of this tab, so onFinished isn't called after the tab has been destroyed. The operation
  // itself must not refer to the tab.
  auto watcher = new QFutureWatcher<bool>(this);
  connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, archiveId, onFinished] {
    mountOperationRunning_ = false;
    // The list may have been refreshed in the meantime, so look the archive up again.
    onFinished(watcher->result(), backupTableModel->rowOfArchive(archiveId));
    updateMountButtons();
    watcher->deleteLater();
  });
  watcher->setFuture(QtConcurrent::run(std::move(operation)));
}

void ConfigTab::onCurrentTabChanged(int index) {
  if (getTabWidget()->indexOf(this) == index) {
    updateFromBackupConfig();
  }
}

void ConfigTab::tableSelectionChanged(QItemSelection const &current, QItemSelection const &previous) {
  updateMountButtons();
}

void ConfigTab::updateMountButtons() {
  if (!mountOperationRunning_ && ui->backupsTableView->selectionModel()->hasSelection()) {
    size_t row = ui->backupsTableView->selectionModel()->currentIndex().row();
    spdlog::trace("row changed. new row: {}", row);
    bool is_row_mounted = backupTableModel->rowData(row).is_mounted;
    ui->backupMountButton->setEnabled(!is_row_mounted);
    ui->backupUmountButton->setEnabled(is_row_mounted);
  } else {
    ui->backupMountButton->setDisabled(true);
    ui->backupUmountButton->setDisabled(true);
  }
}

void ConfigTab::backupFinished(int exitCode) {
  backupRunning_ = false;
  if (backupCancelled_) {
    emit setStatusMessage("Backup was cancelled", 30000);
    spdlog::info("Backup was cancelled, borgmatic exit code {}", exitCode);
  } else if (exitCode == 0) {
    emit setStatusMessage("Backup is done", 30000);
    spdlog::debug("Backup is done");
  } else {
    // No timeout: a failed backup must not go unnoticed.
    emit setStatusMessage(QString("Backup FAILED (borgmatic exit code %1)").arg(exitCode));
    spdlog::error("Backup failed, borgmatic exit code {}", exitCode);
  }
  updateFromBackupConfig();
  ui->startBackupButton->setEnabled(true);
  ui->cancelBackupButton->setDisabled(true);
}

void ConfigTab::updateBackupInfos() {
  auto info = info_future_.result();
  ui->infoLocationLabel->setText(info.location.c_str());
  QLocale locale;
  ui->infoOriginalSizeLabel->setText(info.originalSize ? locale.formattedDataSize(info.originalSize) : "-");
  ui->infoCompressedSizeLabel->setText(info.compressedSize ? locale.formattedDataSize(info.compressedSize) : "-");
  runPendingRefresh();
}

void ConfigTab::updateBackupList() {
  backupTableModel->updateBackups(list_future_.result());
  runPendingRefresh();
}

void ConfigTab::runPendingRefresh() {
  if (refreshPending_ && !info_future_.isRunning() && !list_future_.isRunning()) {
    refreshPending_ = false;
    updateFromBackupConfig();
  }
}

QTabWidget *ConfigTab::getTabWidget() const {
  return parentWidget() ? qobject_cast<QTabWidget *>(parentWidget()->parentWidget()) : nullptr;
}

void ConfigTab::updateFromBackupConfig() {
  ui->purgeCheckBox->setCheckState(backupConfig->isBackupPurging() ? Qt::CheckState::Checked
                                                                   : Qt::CheckState::Unchecked);
  ui->openMountPointCheckBox->setCheckState(backupConfig->isMountPointToBeOpened() ? Qt::CheckState::Checked
                                                                                   : Qt::CheckState::Unchecked);

  // A refresh still running may be for an outdated config file, so refresh again once it has finished.
  if (info_future_.isRunning() || list_future_.isRunning()) {
    refreshPending_ = true;
    return;
  }

  // Capture the config, not this: the tab may be destroyed before the task has finished.
  info_future_ = QtConcurrent::run([config = backupConfig]() -> backup::helper::Info { return config->info(); });
  info_watcher_.setFuture(info_future_);
  list_future_ = QtConcurrent::run(
      [config = backupConfig]() -> std::vector<backup::helper::ListItem> { return config->list(); });
  list_watcher_.setFuture(list_future_);
}

bool ConfigTab::isRowSelected() const { return ui->backupsTableView->selectionModel()->hasSelection(); }
