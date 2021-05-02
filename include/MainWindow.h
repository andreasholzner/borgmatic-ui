#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCloseEvent>
#include <QMainWindow>
#include <memory>

#include "BackupConfig.h"
#include "BorgmaticManager.h"
#include "DesktopServicesWrapper.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(std::unique_ptr<BorgmaticManager> manager, QWidget *parent = nullptr);

  ~MainWindow() override;

 protected:
  void closeEvent(QCloseEvent *event) override;

 private slots:
  void on_menuNew_triggered();
  void on_menuQuit_triggered();
  void deleteConfigTab(int index);

 private:
  void addTabForConfig(std::shared_ptr<BackupConfig> borgmaticConfig);
  void readWindowSettings();
  void saveWindowSettings();
  bool areAnyArchivesMounted();

  Ui::MainWindow *ui;
  std::unique_ptr<BorgmaticManager> borgmaticManager;
  std::shared_ptr<DesktopServicesWrapper> desktop_services_wrapper_;
};

#endif  // MAINWINDOW_H
