#include "MainWindow.h"

#include <QFileInfo>
#include <QSettings>
#include <QStatusBar>
#include <QString>

#include "ConfigTab.h"
#include "ui_mainwindow.h"

static char const* const GEOMETRY_KEY = "mainwindow/geometry";
static char const* const STATE_KEY = "mainwindow/state";

MainWindow::MainWindow(std::unique_ptr<BorgmaticManager> manager, QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      borgmaticManager(std::move(manager)),
      desktop_services_wrapper_(std::make_shared<DesktopServicesWrapperImpl>()) {
  ui->setupUi(this);
  ui->borgmaticTabWidget->clear();
  for (auto&& config : borgmaticManager->configs()) {
    addTabForConfig(config);
  }
  readWindowSettings();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::closeEvent(QCloseEvent* event) {
  saveWindowSettings();
  event->accept();
}

void MainWindow::on_menuNew_triggered() {
  auto borgmaticConfig = borgmaticManager->newBorgmaticConfig();
  addTabForConfig(borgmaticConfig);
}

void MainWindow::on_menuQuit_triggered() {
  close();
  borgmaticManager->saveSettings();
}

void MainWindow::deleteConfigTab(int index) {
  ui->borgmaticTabWidget->removeTab(index);
  borgmaticManager->removeConfig(index);
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
