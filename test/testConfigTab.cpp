#include <QLabel>
#include <QLineEdit>
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
  std::string configFileName{"file1"};

  SECTION("initializes all displayed data from BackupConfig via info and list") {
    REQUIRE_CALL(*config, borgmaticConfigFile(eq(configFileName)));
    REQUIRE_CALL(*config, borgmaticConfigFile()).RETURN(configFileName);
    ALLOW_CALL(*config, isBackupPurging()).RETURN(false);
    REQUIRE_CALL(*config, info()).RETURN(prepareInfo());
    REQUIRE_CALL(*config, list()).RETURN(prepareList());

    auto configTab = ConfigTab{config};

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

TEST_CASE("ConfigTab", "[ui") {
  auto config = std::make_shared<BackupConfigMock>();
  std::shared_ptr<ConfigTab> configTab = nullptr;
  {
    ALLOW_CALL(*config, borgmaticConfigFile()).RETURN("");
    ALLOW_CALL(*config, isBackupPurging()).RETURN(false);
    ALLOW_CALL(*config, info()).RETURN(prepareInfo());
    ALLOW_CALL(*config, list()).RETURN(prepareList());
    configTab = std::make_shared<ConfigTab>(config);
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
}
