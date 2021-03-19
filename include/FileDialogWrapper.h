#ifndef BORGMATIC_UI_INCLUDE_FILEDIALOGWRAPPER_H_
#define BORGMATIC_UI_INCLUDE_FILEDIALOGWRAPPER_H_

#include <QString>
#include <QWidget>
#include <QDir>
#include <QFileDialog>

struct FileDialogWrapper {
  virtual ~FileDialogWrapper() = default;
  virtual QString selectBorgmaticConfigFile(QWidget *parent) = 0;
  virtual QString selectMountPoint(QWidget *parent) = 0;
};

struct FileDialogWrapperImpl : public FileDialogWrapper {
  QString selectBorgmaticConfigFile(QWidget *parent) override {
    auto defaultPath = QDir::home();
    defaultPath.cd(".config/borgmatic");
    return QFileDialog::getOpenFileName(parent, "Borgmatic Config", defaultPath.absolutePath(), "YAML Files (*.yaml *.yml)");
  }
  QString selectMountPoint(QWidget *parent) override {
    return QFileDialog::getExistingDirectory(parent, "Backup Mountpoint");
  }
};

#endif  // BORGMATIC_UI_INCLUDE_FILEDIALOGWRAPPER_H_
