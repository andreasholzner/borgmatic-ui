#include <spdlog/cfg/env.h>
#include <spdlog/spdlog.h>

#include <QApplication>
#include <memory>

#include "BorgmaticManager.h"
#include "MainWindow.h"

int main(int argc, char *argv[]) {
  // Defaults to info; e.g. SPDLOG_LEVEL=debug enables debug output.
  spdlog::set_level(spdlog::level::info);
  spdlog::cfg::load_env_levels();
  QApplication app(argc, argv);

  QCoreApplication::setOrganizationName("holzner");
  QCoreApplication::setApplicationName("borgmatic-ui");

  auto borgmaticManager = std::make_unique<BorgmaticManagerImpl>();
  borgmaticManager->loadSettings();
  MainWindow window(std::move(borgmaticManager));
  window.show();

  return app.exec();
}
