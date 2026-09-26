#include "MainWindow.h"

#include <QFileInfo>
#include <QSettings>
#include <QStatusBar>
#include <QString>
#include <algorithm>

#include "ConfigTab.h"
#include "ui_mainwindow.h"

static char const* const GEOMETRY_KEY = "mainwindow/geometry";
static char const* const STATE_KEY = "mainwindow/state";

MainWindow::MainWindow(std::unique_ptr<BorgmaticManager> manager,
                       std::shared_ptr<DesktopServicesWrapper> desktopServicesWrapper, QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      borgmaticManager(std::move(manager)),
      desktop_services_wrapper_(std::move(desktopServicesWrapper)) {
  ui->setupUi(this);
  connect(ui->menuNew, &QAction::triggered, this, &MainWindow::addNewConfig);
  connect(ui->menuQuit, &QAction::triggered, this, &MainWindow::quit);
  ui->borgmaticTabWidget->clear();
  for (auto&& config : borgmaticManager->configs()) {
    addTabForConfig(config);
  }
  readWindowSettings();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::closeEvent(QCloseEvent* event) {
  if (areAnyBackupsRunning()) {
    // Destroying the tabs cancels the backups and waits for borgmatic to exit.
    if (!desktop_services_wrapper_->confirm(this, "Quit", "A backup is still running. Cancel it and quit?")) {
      event->ignore();
      return;
    }
  }
  if (areAnyArchivesMounted()) {
    spdlog::debug("Some archive are still mounted.");
    if (!desktop_services_wrapper_->confirm(this, "Quit", "Archives are still mounted. Quit anyway?")) {
      event->ignore();
      return;
    }
  }
  saveWindowSettings();
  borgmaticManager->saveSettings();
  event->accept();
}

void MainWindow::addNewConfig() {
  auto borgmaticConfig = borgmaticManager->newBorgmaticConfig();
  addTabForConfig(borgmaticConfig);
}

void MainWindow::quit() { close(); }

void MainWindow::deleteConfigTab(int index) {
  auto tab = qobject_cast<ConfigTab*>(ui->borgmaticTabWidget->widget(index));
  if (tab->isBackupRunning() || tab->hasMountedArchives()) {
    if (!desktop_services_wrapper_->confirm(this, "Delete configuration",
                                            "A backup is running or archives are mounted. Cancel the backup, unmount "
                                            "the archives and delete the configuration?")) {
      return;
    }
    if (!tab->umountAllArchives()) {
      ui->statusbar->showMessage("Unmounting failed, the configuration is kept.");
      return;
    }
  }
  ui->borgmaticTabWidget->removeTab(index);
  borgmaticManager->removeConfig(index);
  // The tab is the sender of the signal that got us here, so it must not be destroyed right away. Its destructor
  // cancels a running backup.
  tab->deleteLater();
}

void MainWindow::addTabForConfig(std::shared_ptr<BackupConfig> borgmaticConfig) {
  auto newTab = new ConfigTab(borgmaticConfig, desktop_services_wrapper_);
  QString label("");
  if (!borgmaticConfig->borgmaticConfigFile().empty()) {
    label = QFileInfo(borgmaticConfig->borgmaticConfigFile().c_str()).baseName();
  }
  ui->borgmaticTabWidget->addTab(newTab, label);
  connect(newTab, &ConfigTab::deleteTab, this, &MainWindow::deleteConfigTab);
  connect(newTab, &ConfigTab::setStatusMessage, ui->statusbar, &QStatusBar::showMessage);
  connect(ui->borgmaticTabWidget, &QTabWidget::currentChanged, newTab, &ConfigTab::onCurrentTabChanged);
}

void MainWindow::readWindowSettings() {
  QSettings settings;
  restoreGeometry(settings.value(GEOMETRY_KEY).toByteArray());
  restoreState(settings.value(STATE_KEY).toByteArray());
}
void MainWindow::saveWindowSettings() {
  QSettings settings;
  settings.setValue(GEOMETRY_KEY, saveGeometry());
  settings.setValue(STATE_KEY, saveState());
}

bool MainWindow::areAnyBackupsRunning() {
  auto tabs = ui->borgmaticTabWidget->findChildren<ConfigTab*>();
  return std::ranges::any_of(tabs, [](ConfigTab const* tab) { return tab->isBackupRunning(); });
}

bool MainWindow::areAnyArchivesMounted() {
  auto tabs = ui->borgmaticTabWidget->findChildren<ConfigTab*>();
  return std::ranges::any_of(tabs, [](ConfigTab const* tab) { return tab->hasMountedArchives(); });
}
