#include <QAction>
#include <QPushButton>
#include <QTabWidget>
#include <QtTest>
#include <catch2/catch.hpp>
#include <catch2/trompeloeil.hpp>
#include <memory>
#include <vector>

#include "BackupConfig.h"
#include "BorgmaticManager.h"
#include "ConfigTab.h"
#include "MainWindow.h"
#include "mocks.h"

using namespace trompeloeil;

std::vector<std::shared_ptr<BackupConfig>> prepareConfigs(std::vector<std::string> const &configNames) {
  std::vector<std::shared_ptr<BackupConfig>> res{configNames.size()};
  std::transform(configNames.begin(), configNames.end(), res.begin(), [](std::string const &configName) {
    auto backupConfig = std::make_shared<BackupConfigImpl<BorgmaticBackupWorker>>();
    backupConfig->borgmaticConfigFile(configName);
    return std::static_pointer_cast<BackupConfig>(backupConfig);
  });
  return res;
}

std::vector<std::shared_ptr<BackupConfig>> prepareMockedConfigs(
    std::initializer_list<std::shared_ptr<BackupConfigMock>> mocks) {
  std::vector<std::shared_ptr<BackupConfig>> res{mocks.size()};
  std::transform(mocks.begin(), mocks.end(), res.begin(), [](std::shared_ptr<BackupConfigMock> const &mock) {
    return std::static_pointer_cast<BackupConfig>(mock);
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

    REQUIRE_CALL(*manager, newBorgmaticConfig()).RETURN(std::make_shared<BackupConfigImpl<BorgmaticBackupWorker>>());

    mainWindow.findChild<QAction *>("menuNew")->trigger();

    REQUIRE(tabWidget->count() == 1);
    REQUIRE(tabWidget->tabText(0) == QString{});
  }

  SECTION("Delete config button removes the active tab") {
    REQUIRE_CALL(*manager, configs()).RETURN(prepareConfigs({"name1", "name2"}));
    auto mainWindow = MainWindow{std::move(uniqueManager)};

    int tabIndexToDelete = 0;
    REQUIRE_CALL(*manager, removeConfig(eq(tabIndexToDelete)));
    auto tabWidget = mainWindow.findChild<QTabWidget *>("borgmaticTabWidget");
    tabWidget->setCurrentIndex(tabIndexToDelete);
    auto currentTab = qobject_cast<ConfigTab *>(tabWidget->currentWidget());
    currentTab->findChild<QPushButton *>("deleteConfigButton")->click();

    REQUIRE(tabWidget->count() == 1);
    REQUIRE(tabWidget->tabText(0) == QString{"name2"});
  }

  SECTION("Updates the tab after a switch") {
    auto config1 = std::make_shared<BackupConfigMock>();
    auto config2 = std::make_shared<BackupConfigMock>();
    auto mockConfigs = prepareMockedConfigs({config1, config2});
    REQUIRE_CALL(*manager, configs()).RETURN(mockConfigs);
    ALLOW_CALL(*config1, borgmaticConfigFile(_));
    ALLOW_CALL(*config1, borgmaticConfigFile()).RETURN("name1");
    ALLOW_CALL(*config1, isBackupPurging()).RETURN(false);
    ALLOW_CALL(*config1, isMountPointToBeOpened()).RETURN(false);
    REQUIRE_CALL(*config1, info()).TIMES(1).RETURN(backup::helper::Info{});
    REQUIRE_CALL(*config1, list()).TIMES(1).RETURN(std::vector<backup::helper::ListItem>{});

    int const number_updates_tab2 = 2;
    ALLOW_CALL(*config2, borgmaticConfigFile(_));
    ALLOW_CALL(*config2, borgmaticConfigFile()).RETURN("name2");
    ALLOW_CALL(*config2, isBackupPurging()).RETURN(false);
    ALLOW_CALL(*config2, isMountPointToBeOpened()).RETURN(false);
    REQUIRE_CALL(*config2, info()).TIMES(number_updates_tab2).RETURN(backup::helper::Info{});
    REQUIRE_CALL(*config2, list()).TIMES(number_updates_tab2).RETURN(std::vector<backup::helper::ListItem>{});

    auto mainWindow = MainWindow{std::move(uniqueManager)};
    auto tabWidget = mainWindow.findChild<QTabWidget *>("borgmaticTabWidget");
    REQUIRE(tabWidget->currentIndex() == 0);

    tabWidget->setCurrentIndex(1);
  }
}
