#include <catch2/catch.hpp>
#include <catch2/trompeloeil.hpp>
#include <memory>

#include "BackupConfig.h"
#include "mocks.h"

using namespace trompeloeil;

BackupWorkerMockImpl workerMock;

struct BackupWorkerMock {
  void configure(std::filesystem::path pathToConfig, bool purgeFlag) { workerMock.configure(pathToConfig, purgeFlag); }
  void start(std::function<void()> onFinished, std::function<void(std::string)> logHandler) {
    workerMock.start(onFinished, logHandler);
  }
  void cancel() { workerMock.cancel(); }
};

TEST_CASE("BackupConfig", "[logic]") {
  BackupConfigImpl<BackupWorkerMock> backupConfig;

  SECTION("cancelBackup calls cancel on worker") {
    REQUIRE_CALL(workerMock, cancel());

    backupConfig.cancelBackup();
  }

  SECTION("startBackup configures worker and calls start on worker") {
    std::filesystem::path configPath{"/tmp/test.yml"};
    bool purgeFlag = true;
    REQUIRE_CALL(workerMock, configure(eq(configPath), eq(purgeFlag)));
    REQUIRE_CALL(workerMock, start(_, _));

    backupConfig.borgmaticConfigFile(configPath.string());
    backupConfig.isBackupPurging(purgeFlag);

    backupConfig.startBackup([]() {});
  }

  SECTION("list returns empty data for non-existing location") {
    std::vector<backup::helper::ListItem> list = backupConfig.list();

    REQUIRE(list.empty());
  }

  SECTION("info returns empty data for non-existing location") {
    backup::helper::Info info = backupConfig.info();

    REQUIRE(info.id.empty());
    REQUIRE(info.location.empty());
    REQUIRE(info.originalSize == 0);
    REQUIRE(info.compressedSize == 0);
  }
}
