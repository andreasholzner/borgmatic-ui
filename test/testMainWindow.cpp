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

TEST_CASE("MainWindow", "[ui]") {
  auto uniqueManager = std::make_unique<BorgmaticManagerMock>();
  auto manager = uniqueManager.get();
  ALLOW_CALL(*manager, configs()).RETURN(std::vector<std::shared_ptr<BackupConfig>>());
  auto mainWindow = MainWindow{std::move(uniqueManager)};
  auto tabWidget = mainWindow.findChild<QTabWidget *>("borgmaticTabWidget");
  REQUIRE(tabWidget->count() == 0);

  SECTION("New Config menu entry adds a new tab") {
    REQUIRE_CALL(*manager, newBorgmaticConfig()).RETURN(std::make_shared<BackupConfigImpl>());

    mainWindow.findChild<QAction *>("menuNew")->trigger();

    REQUIRE(tabWidget->count() == 1);
  }
}
