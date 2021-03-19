#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QTableView>
#include <QtTest>
#include <catch2/catch.hpp>
#include <catch2/trompeloeil.hpp>
#include <memory>
#include <string>
#include <vector>

#include "BackupConfig.h"
#include "ConfigTab.h"
#include "FileDialogWrapper.h"

using namespace trompeloeil;

class BackupConfigMock : public mock_interface<BackupConfig> {
  IMPLEMENT_MOCK1(borgmaticConfigFile);
  IMPLEMENT_CONST_MOCK0(borgmaticConfigFile);
  IMPLEMENT_MOCK1(isBackupPurging);
  IMPLEMENT_CONST_MOCK0(isBackupPurging);
  IMPLEMENT_MOCK0(list);
  IMPLEMENT_MOCK0(info);
  IMPLEMENT_MOCK2(startBackup);
  IMPLEMENT_MOCK0(cancelBackup);
  IMPLEMENT_MOCK0(isAccessible);
};

struct FileDialogWrapperMock : public mock_interface<FileDialogWrapper> {
  IMPLEMENT_MOCK1(selectBorgmaticConfigFile);
  IMPLEMENT_MOCK1(selectMountPoint);
};

auto prepareInfo() {
  backup::helper::Info info{};
  info.location = "location";
  info.originalSize = 1024000;
  info.compressedSize = 0;
  return info;
}

auto prepareList() {
  return std::vector<backup::helper::ListItem>{
      {"id1", "name1", "2000-10-05 10:15:30.500"},
      {"id2", "name2", "2000-10-06 10:15:30.500"}
  };
}

TEST_CASE("ConfigTab construction", "[ui]") {
  auto config = std::make_shared<BackupConfigMock>();
  std::shared_ptr<FileDialogWrapper> wrapperMock{std::make_shared<FileDialogWrapperMock>()};
  std::string configFileName{"file1"};

  SECTION("initializes all displayed data from BackupConfig via info and list") {
    REQUIRE_CALL(*config, borgmaticConfigFile(eq(configFileName)));
    REQUIRE_CALL(*config, borgmaticConfigFile()).RETURN(configFileName);
    ALLOW_CALL(*config, isBackupPurging()).RETURN(false);
    REQUIRE_CALL(*config, info()).RETURN(prepareInfo());
    REQUIRE_CALL(*config, list()).RETURN(prepareList());

    auto configTab = ConfigTab{config, wrapperMock};
    REQUIRE(configTab.findChild<QCheckBox *>("purgeCheckBox")->isEnabled() == true);
    REQUIRE(configTab.findChild<QPushButton *>("startBackupButton")->isEnabled() == true);
    REQUIRE(configTab.findChild<QPushButton *>("cancelBackupButton")->isEnabled() == false);

    // data from info
    REQUIRE(configTab.findChild<QLineEdit *>("configEdit")->text().toStdString() == configFileName);
    REQUIRE(configTab.findChild<QLabel *>("infoLocationLabel")->text().toStdString() == prepareInfo().location);
    REQUIRE(configTab.findChild<QLabel *>("infoOriginalSizeLabel")->text().toStdString() == "1.000,00 KiB");
    REQUIRE(configTab.findChild<QLabel *>("infoCompressedSizeLabel")->text().toStdString() == "-");

    // data from list
    auto model = configTab.findChild<QTableView *>("backupsTableView")->model();
    REQUIRE(model->data(model->index(0, 0)).value<QString>().toStdString() == prepareList()[0].name);
    REQUIRE(model->data(model->index(1, 0)).value<QString>().toStdString() == prepareList()[1].name);
  }
}

TEST_CASE("ConfigTab", "[ui]") {
  auto config = std::make_shared<BackupConfigMock>();
  std::shared_ptr<FileDialogWrapper> wrapperMock{std::make_shared<FileDialogWrapperMock>()};
  std::shared_ptr<ConfigTab> configTab = nullptr;
  {
    ALLOW_CALL(*config, borgmaticConfigFile()).RETURN("");
    ALLOW_CALL(*config, isBackupPurging()).RETURN(false);
    ALLOW_CALL(*config, info()).RETURN(prepareInfo());
    ALLOW_CALL(*config, list()).RETURN(prepareList());
    configTab = std::make_shared<ConfigTab>(config, wrapperMock);
  }

  SECTION("new borgmatic config fetches info and list data") {
    std::string newBorgmaticConfig = "new_config";
    REQUIRE_CALL(*config, borgmaticConfigFile(eq(newBorgmaticConfig)));
    REQUIRE_CALL(*config, info()).RETURN(prepareInfo());
    REQUIRE_CALL(*config, list()).RETURN(prepareList());
    ALLOW_CALL(*config, isBackupPurging()).RETURN(true);
    ALLOW_CALL(*config, isBackupPurging(eq(1)));

    configTab->findChild<QLineEdit *>("configEdit")->setText(newBorgmaticConfig.c_str());
  }

  SECTION("start backup button starts a backup, refreshes ui after completion") {
    auto startButton = configTab->findChild<QPushButton *>("startBackupButton");
    auto cancelButton = configTab->findChild<QPushButton *>("cancelBackupButton");
    REQUIRE(startButton->isEnabled() == true);
    REQUIRE(cancelButton->isEnabled() == false);

    std::function<void()> usedFinishedHandler;
    REQUIRE_CALL(*config, startBackup(_, _)).LR_SIDE_EFFECT(usedFinishedHandler = _1);

    startButton->click();

    REQUIRE(startButton->isEnabled() == false);
    REQUIRE(cancelButton->isEnabled() == true);

    ALLOW_CALL(*config, isBackupPurging()).RETURN(false);
    REQUIRE_CALL(*config, info()).RETURN(prepareInfo());
    REQUIRE_CALL(*config, list()).RETURN(prepareList());

    usedFinishedHandler();

    REQUIRE(startButton->isEnabled() == true);
    REQUIRE(cancelButton->isEnabled() == false);
  }

  SECTION("cancel backup button cancels a running backup and refreshes the ui") {
    auto startButton = configTab->findChild<QPushButton *>("startBackupButton");
    auto cancelButton = configTab->findChild<QPushButton *>("cancelBackupButton");
    REQUIRE(startButton->isEnabled() == true);
    REQUIRE(cancelButton->isEnabled() == false);

    REQUIRE_CALL(*config, startBackup(_, _));
    startButton->click();

    REQUIRE(startButton->isEnabled() == false);
    REQUIRE(cancelButton->isEnabled() == true);

    REQUIRE_CALL(*config, cancelBackup());
    ALLOW_CALL(*config, isBackupPurging()).RETURN(false);
    REQUIRE_CALL(*config, info()).RETURN(prepareInfo());
    REQUIRE_CALL(*config, list()).RETURN(prepareList());

    cancelButton->click();

    REQUIRE(startButton->isEnabled() == true);
    REQUIRE(cancelButton->isEnabled() == false);
  }
}
