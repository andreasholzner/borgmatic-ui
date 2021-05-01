#ifndef BORGMATIC_UI_INCLUDE_DESKTOPSERVICESWRAPPER_H_
#define BORGMATIC_UI_INCLUDE_DESKTOPSERVICESWRAPPER_H_

#include <QString>
#include <QWidget>
#include <QDir>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>

struct DesktopServicesWrapper {
  virtual ~DesktopServicesWrapper() = default;
  virtual QString selectBorgmaticConfigFile(QWidget *parent) = 0;
  virtual QString selectMountPoint(QWidget *parent) = 0;
  virtual void openLocation(QString const& path) = 0;
};

struct DesktopServicesWrapperImpl : public DesktopServicesWrapper {
  QString selectBorgmaticConfigFile(QWidget *parent) override {
    auto defaultPath = QDir::home();
    defaultPath.cd(".config/borgmatic");
    return QFileDialog::getOpenFileName(parent, "Borgmatic Config", defaultPath.absolutePath(), "YAML Files (*.yaml *.yml)");
  }
  QString selectMountPoint(QWidget *parent) override {
    return QFileDialog::getExistingDirectory(parent, "Backup Mountpoint");
  }
  void openLocation(QString const& path) override {
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
  }
};

#endif  // BORGMATIC_UI_INCLUDE_DESKTOPSERVICESWRAPPER_H_
