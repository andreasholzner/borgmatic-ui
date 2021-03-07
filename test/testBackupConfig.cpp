#include <catch2/catch.hpp>
#include <catch2/trompeloeil.hpp>
#include <memory>

#include "BackupConfig.h"

using namespace trompeloeil;

struct BackupWorkerMockImpl {
  MAKE_MOCK2(start, void(std::function<void()>, std::function<void(std::string)>));
  MAKE_MOCK0(cancel, void());
};

BackupWorkerMockImpl workerMock;

struct BackupWorkerMock {
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

  SECTION("startBackup calls start on worker") {
    REQUIRE_CALL(workerMock, start(_, _));

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
