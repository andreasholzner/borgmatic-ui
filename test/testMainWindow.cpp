#include <QAction>
#include <QCoreApplication>
#include <QPushButton>
#include <QPointer>
#include <QTabWidget>
#include <QtTest>
#include <catch2/catch_all.hpp>
#include <catch2/trompeloeil.hpp>
#include <memory>
#include <vector>

#include "BackupConfig.h"
#include "ConfigTab.h"
#include "MainWindow.h"
#include "test_helper.h"

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

using Expectations = std::vector<std::unique_ptr<trompeloeil::expectation>>;

// Allows everything a ConfigTab needs to display the config.
void allowTabCalls(BackupConfigMock &config, Expectations &expectations,
                   std::vector<backup::helper::ListItem> const &list = {}) {
  expectations.push_back(NAMED_ALLOW_CALL(config, borgmaticConfigFile()).RETURN(std::string("name")));
  expectations.push_back(NAMED_ALLOW_CALL(config, borgmaticConfigFile(_)));
  expectations.push_back(NAMED_ALLOW_CALL(config, isBackupPurging()).RETURN(false));
  expectations.push_back(NAMED_ALLOW_CALL(config, isMountPointToBeOpened()).RETURN(false));
  expectations.push_back(NAMED_ALLOW_CALL(config, info()).RETURN(backup::helper::Info{}));
  expectations.push_back(NAMED_ALLOW_CALL(config, list()).RETURN(list));
}

void processDeferredDeletes() { QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); }

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
    wait_for_qthreads_to_finish();
    QApplication::processEvents();
    auto currentTab = qobject_cast<ConfigTab *>(tabWidget->currentWidget());
    currentTab->findChild<QPushButton *>("deleteConfigButton")->click();

    REQUIRE(tabWidget->count() == 1);
    REQUIRE(tabWidget->tabText(0) == QString{"name2"});
  }

  SECTION("Closing the window saves the backup configs") {
    ALLOW_CALL(*manager, configs()).RETURN(prepareConfigs({}));
    auto mainWindow = MainWindow{std::move(uniqueManager)};
    mainWindow.show();

    REQUIRE_CALL(*manager, saveSettings());
    REQUIRE(mainWindow.close());
  }

  SECTION("Quit menu entry saves the backup configs once") {
    ALLOW_CALL(*manager, configs()).RETURN(prepareConfigs({}));
    auto mainWindow = MainWindow{std::move(uniqueManager)};
    mainWindow.show();

    REQUIRE_CALL(*manager, saveSettings()).TIMES(1);
    mainWindow.findChild<QAction *>("menuQuit")->trigger();
    REQUIRE_FALSE(mainWindow.isVisible());
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
    wait_for_qthreads_to_finish();
    auto tabWidget = mainWindow.findChild<QTabWidget *>("borgmaticTabWidget");
    REQUIRE(tabWidget->currentIndex() == 0);

    tabWidget->setCurrentIndex(1);
    wait_for_qthreads_to_finish();
  }
}

TEST_CASE("MainWindow with active backups or mounts", "[ui]") {
  auto uniqueManager = std::make_unique<BorgmaticManagerMock>();
  auto manager = uniqueManager.get();
  auto desktopServices = std::make_shared<DesktopServicesWrapperMock>();
  auto config = std::make_shared<BackupConfigMock>();
  Expectations expectations;
  allowTabCalls(*config, expectations, {{"id1", "archive1", "2000-10-05 10:15:30.500", true, "/mnt/archive1"}});
  expectations.push_back(NAMED_ALLOW_CALL(*manager, configs()).RETURN(prepareMockedConfigs({config})));

  auto mainWindow = MainWindow{std::move(uniqueManager), desktopServices};
  wait_for_qthreads_to_finish();
  QApplication::processEvents();
  auto tabWidget = mainWindow.findChild<QTabWidget *>("borgmaticTabWidget");
  auto tab = qobject_cast<ConfigTab *>(tabWidget->widget(0));
  auto deleteButton = tab->findChild<QPushButton *>("deleteConfigButton");
  REQUIRE(tab->hasMountedArchives());

  SECTION("deleting a tab with mounted archives is not done without confirmation") {
    REQUIRE_CALL(*desktopServices, confirm(_, _, _)).RETURN(false);
    FORBID_CALL(*config, umountArchive(_));
    FORBID_CALL(*manager, removeConfig(_));

    deleteButton->click();

    REQUIRE(tabWidget->count() == 1);
  }

  SECTION("deleting a tab unmounts its archives and destroys it") {
    QPointer<ConfigTab> tabPointer{tab};
    REQUIRE_CALL(*desktopServices, confirm(_, _, _)).RETURN(true);
    REQUIRE_CALL(*config, umountArchive(eq(std::string{"/mnt/archive1"}))).RETURN(true);
    REQUIRE_CALL(*manager, removeConfig(eq(0)));

    deleteButton->click();
    processDeferredDeletes();

    REQUIRE(tabWidget->count() == 0);
    REQUIRE(tabPointer.isNull());
  }

  SECTION("a tab whose archives can't be unmounted is kept") {
    REQUIRE_CALL(*desktopServices, confirm(_, _, _)).RETURN(true);
    REQUIRE_CALL(*config, umountArchive(_)).RETURN(false);
    FORBID_CALL(*manager, removeConfig(_));

    deleteButton->click();
    processDeferredDeletes();

    REQUIRE(tabWidget->count() == 1);
    REQUIRE(tab->hasMountedArchives());
  }

  SECTION("deleting a tab with a running backup cancels the backup") {
    REQUIRE_CALL(*config, startBackup(_, _));
    tab->findChild<QPushButton *>("startBackupButton")->click();

    REQUIRE_CALL(*desktopServices, confirm(_, _, _)).RETURN(true);
    REQUIRE_CALL(*config, umountArchive(_)).RETURN(true);
    REQUIRE_CALL(*manager, removeConfig(eq(0)));
    REQUIRE_CALL(*config, cancelBackupAndWait());

    deleteButton->click();
    processDeferredDeletes();

    REQUIRE(tabWidget->count() == 0);
  }

  SECTION("closing is not done without confirmation") {
    mainWindow.show();
    REQUIRE_CALL(*desktopServices, confirm(_, _, _)).RETURN(false);
    FORBID_CALL(*manager, saveSettings());

    REQUIRE_FALSE(mainWindow.close());
  }

  SECTION("closing with a running backup asks for both backup and mounts") {
    mainWindow.show();
    REQUIRE_CALL(*config, startBackup(_, _));
    tab->findChild<QPushButton *>("startBackupButton")->click();

    REQUIRE_CALL(*desktopServices, confirm(_, _, _)).TIMES(2).RETURN(true);
    REQUIRE_CALL(*manager, saveSettings());
    REQUIRE(mainWindow.close());

    // destroying the window cancels the backup
    expectations.push_back(NAMED_REQUIRE_CALL(*config, cancelBackupAndWait()));
  }
}
