#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "BorgmaticManager.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(std::unique_ptr<BorgmaticManager> borgmaticManager, QWidget *parent = nullptr);

  ~MainWindow() override;

 private slots:
  void on_menuNew_triggered();
  void on_menuQuit_triggered();
  void deleteConfigTab(int index);

 private:
  Ui::MainWindow *ui;
  std::unique_ptr<BorgmaticManager> borgmaticManager;
};

#endif  // MAINWINDOW_H