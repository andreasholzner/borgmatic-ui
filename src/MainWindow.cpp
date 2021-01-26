#include "MainWindow.h"

#include <QFileInfo>
#include <QString>

#include "ConfigTab.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(std::unique_ptr<BorgmaticManager> manager, QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), borgmaticManager(std::move(manager)) {
  ui->setupUi(this);
  ui->borgmaticTabWidget->clear();
  for (auto&& config : borgmaticManager->configs) {
    addTabForConfig(config);
  }
}

MainWindow::~MainWindow() { delete ui; }

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
  auto newTab = new ConfigTab(borgmaticConfig);
  QString label("");
  if (!borgmaticConfig->borgmaticConfigFile().empty()) {
    label = QFileInfo(borgmaticConfig->borgmaticConfigFile().c_str()).baseName();
  }
  ui->borgmaticTabWidget->addTab(newTab, label);
  connect(newTab, &ConfigTab::deleteTab, this, &MainWindow::deleteConfigTab);
}
