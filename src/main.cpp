#include <memory>
#include <QApplication>

#include "BorgmaticManager.h"
#include "MainWindow.h"

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);

  QCoreApplication::setOrganizationName("holzner");
  QCoreApplication::setApplicationName("borgmatic-ui");

  auto borgmaticManager = std::make_unique<BorgmaticManager>();
  borgmaticManager->loadSettings();
  MainWindow w(std::move(borgmaticManager));
  w.show();

  return a.exec();
}
