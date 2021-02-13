#include <QAction>
#include <QTabWidget>
#include <QtTest>
#include <catch2/catch.hpp>
#include <catch2/trompeloeil.hpp>
#include <memory>
#include <vector>

#include "BackupConfig.h"
#include "BorgmaticManager.h"
#include "MainWindow.h"

class BorgmaticManagerMock : public trompeloeil::mock_interface<BorgmaticManager> {
  IMPLEMENT_MOCK0(newBorgmaticConfig);
  IMPLEMENT_MOCK1(removeConfig);
  IMPLEMENT_MOCK0(loadSettings);
  IMPLEMENT_MOCK0(saveSettings);
  IMPLEMENT_MOCK0(configs);
};

std::vector<std::shared_ptr<BackupConfig>> prepareConfigs(std::vector<std::string> const &configNames) {
  std::vector<std::shared_ptr<BackupConfig>> res{configNames.size()};
  std::transform(configNames.begin(), configNames.end(), res.begin(), [](std::string const &configName) {
    auto backupConfig = std::make_shared<BackupConfigImpl>();
    backupConfig->borgmaticConfigFile(configName);
    return std::static_pointer_cast<BackupConfig>(backupConfig);
  });
  return res;
}

TEST_CASE("MainWindow", "[ui]") {
  auto uniqueManager = std::make_unique<BorgmaticManagerMock>();
  auto manager = uniqueManager.get();

  SECTION("MainWindow creates a tab for every config entry upon creation") {
    REQUIRE_CALL(*manager, configs()).RETURN(prepareConfigs({"name1", "name2"}));

    auto mainWindow = MainWindow{std::move(uniqueManager)};

    auto tabWidget = mainWindow.findChild<QTabWidget *>("borgmaticTabWidget");
    REQUIRE(tabWidget->count() == 2);
    REQUIRE(tabWidget->tabText(0) == QString{"name1"});
    REQUIRE(tabWidget->tabText(1) == QString{"name2"});
  }

  SECTION("New Config menu entry adds a new tab") {
    ALLOW_CALL(*manager, configs()).RETURN(prepareConfigs({}));
    auto mainWindow = MainWindow{std::move(uniqueManager)};
    auto tabWidget = mainWindow.findChild<QTabWidget *>("borgmaticTabWidget");
    REQUIRE(tabWidget->count() == 0);

    REQUIRE_CALL(*manager, newBorgmaticConfig()).RETURN(std::make_shared<BackupConfigImpl>());

    mainWindow.findChild<QAction *>("menuNew")->trigger();

    REQUIRE(tabWidget->count() == 1);
    REQUIRE(tabWidget->tabText(0) == QString{});
  }
}
