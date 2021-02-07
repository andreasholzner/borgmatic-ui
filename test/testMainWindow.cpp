#include <catch2/catch.hpp>
#include <memory>

#include <QtTest>
#include <QAction>

#include "BorgmaticManager.h"
#include "MainWindow.h"
#include <QTabWidget>

TEST_CASE("MainWindow", "[ui]") {
  auto uniqueManager = std::make_unique<BorgmaticManager>();
  auto manager = uniqueManager.get();
  auto mainWindow = MainWindow{std::move(uniqueManager)};
  auto tabWidget = mainWindow.findChild<QTabWidget *>("borgmaticTabWidget");
  REQUIRE(tabWidget->count() == 0);

  SECTION("New Config menu entry adds a new tab") {
    mainWindow.findChild<QAction*>("menuNew")->trigger();

    REQUIRE(manager->configs.size() == 1);
    REQUIRE(tabWidget->count() == 1);
  }
}
