#ifndef BORGMATIC_UI_TEST_TEST_HELPER_H_
#define BORGMATIC_UI_TEST_TEST_HELPER_H_

#include <spdlog/spdlog.h>

#include <catch2/trompeloeil.hpp>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

#include "BackupConfig.h"
#include "BorgmaticManager.h"
#include "DesktopServicesWrapper.h"

class BorgmaticManagerMock : public trompeloeil::mock_interface<BorgmaticManager> {
  IMPLEMENT_MOCK0(newBorgmaticConfig);
  IMPLEMENT_MOCK1(removeConfig);
  IMPLEMENT_MOCK0(loadSettings);
  IMPLEMENT_MOCK0(saveSettings);
  IMPLEMENT_MOCK0(configs);
};

class BackupConfigMock : public trompeloeil::mock_interface<BackupConfig> {
  IMPLEMENT_MOCK1(borgmaticConfigFile);
  IMPLEMENT_CONST_MOCK0(borgmaticConfigFile);
  IMPLEMENT_MOCK1(isBackupPurging);
  IMPLEMENT_CONST_MOCK0(isBackupPurging);
  IMPLEMENT_MOCK1(isMountPointToBeOpened);
  IMPLEMENT_CONST_MOCK0(isMountPointToBeOpened);
  IMPLEMENT_MOCK0(list);
  IMPLEMENT_MOCK0(info);
  IMPLEMENT_MOCK2(startBackup);
  IMPLEMENT_MOCK0(cancelBackup);
  IMPLEMENT_MOCK2(mountArchive);
  IMPLEMENT_MOCK1(umountArchive);
};

struct DesktopServicesWrapperMock : public trompeloeil::mock_interface<DesktopServicesWrapper> {
  IMPLEMENT_MOCK1(selectBorgmaticConfigFile);
  IMPLEMENT_MOCK1(selectMountPoint);
  IMPLEMENT_MOCK1(openLocation);
};

struct BackupWorkerMockImpl {
  MAKE_MOCK2(configure, void(std::filesystem::path, bool));
  MAKE_MOCK2(start, void(std::function<void()>, std::function<void(std::string)>));
  MAKE_MOCK0(cancel, void());
};

static void wait_for_qthreads_to_finish() {
  using namespace std::chrono_literals;
  for (int i = 0; i != 1000; ++i) {
    if (QThreadPool::globalInstance()->activeThreadCount() == 0) {
      return;
    }
    std::this_thread::sleep_for(50ms);
  }
  spdlog::warn("Thread count did not decrease to 0 within waiting period.");
}

#endif  // BORGMATIC_UI_TEST_TEST_HELPER_H_
