#include "MainWindow.h"

#include <iostream>

#include "ConfigTab.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(std::unique_ptr<BorgmaticManager> borgmaticManager, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), borgmaticManager(std::move(borgmaticManager)) {
  ui->setupUi(this);
  ui->borgmaticTabWidget->clear();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::on_menuNew_triggered() {
  auto borgmaticConfig = borgmaticManager->newBorgmaticConfig();
  ConfigTab *newTab = new ConfigTab(borgmaticConfig);
  ui->borgmaticTabWidget->addTab(newTab, "");
  connect(newTab, &ConfigTab::deleteTab, this, &MainWindow::deleteConfigTab);
}

void MainWindow::on_menuQuit_triggered() {
  close();
}
void MainWindow::deleteConfigTab(int index) {
  ui->borgmaticTabWidget->removeTab(index);
  borgmaticManager->removeConfig(index);
}
