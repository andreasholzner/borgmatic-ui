#include <QCoreApplication>
#include <QTemporaryDir>
#include <catch2/catch_all.hpp>
#include <catch2/trompeloeil.hpp>
#include <chrono>
#include <fstream>
#include <future>
#include <thread>
#include <memory>
#include <optional>

#include "BackupConfig.h"
#include "test_helper.h"

using namespace trompeloeil;

BackupWorkerMockImpl workerMock;

struct BackupWorkerMock {
  void configure(std::filesystem::path pathToConfig, bool purgeFlag) { workerMock.configure(pathToConfig, purgeFlag); }
  void start(std::function<void(int)> onFinished, std::function<void(std::string)> logHandler) {
    workerMock.start(onFinished, logHandler);
  }
  void cancel() { workerMock.cancel(); }
  void cancelAndWait() { workerMock.cancelAndWait(); }
  std::filesystem::path executable() const { return "/usr/bin/borgmatic"; }
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

    backupConfig.startBackup([](int) {});
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

struct BackupWorkerBrokenPathMock {
  void configure(std::filesystem::path pathToConfig, bool purgeFlag) {}
  void start(std::function<void(int)> onFinished, std::function<void(std::string)> logHandler) {}
  void cancel() {}
  void cancelAndWait() {}
  std::filesystem::path executable() const { return "/not-found"; }
};

TEST_CASE("BackupConfig with broken executable", "[logic]") {
  BackupConfigImpl<BackupWorkerBrokenPathMock> backupConfigWithBrokenPath;
  backupConfigWithBrokenPath.borgmaticConfigFile("/some/file");

  SECTION("info fails gracefully when borgmatic cannot be called") {
    REQUIRE_NOTHROW(backupConfigWithBrokenPath.info());
  }

  SECTION("list fails gracefully when borgmatic cannot be called") {
    REQUIRE_NOTHROW(backupConfigWithBrokenPath.list());
  }
}

// Stands in for borgmatic: a shell script whose body decides the behaviour per action ($1).
class FakeBorgmatic {
 public:
  explicit FakeBorgmatic(std::string const& body) : path_(std::filesystem::path(dir_.path().toStdString()) / "borgmatic") {
    std::ofstream{path_} << "#!/bin/sh\n" << body << "\n";
    std::filesystem::permissions(path_, std::filesystem::perms::owner_all);
  }
  std::filesystem::path const& path() const { return path_; }

 private:
  QTemporaryDir dir_;
  std::filesystem::path path_;
};

struct ScriptedWorker {
  static inline std::filesystem::path script;
  void configure(std::filesystem::path pathToConfig, bool purgeFlag) {}
  void start(std::function<void(int)> onFinished, std::function<void(std::string)> logHandler) {}
  void cancel() {}
  void cancelAndWait() {}
  std::filesystem::path executable() const { return script; }
};

TEST_CASE("BackupConfig handles failing borgmatic calls", "[logic]") {
  BackupConfigImpl<ScriptedWorker> backupConfig;
  backupConfig.borgmaticConfigFile("/some/config.yaml");

  SECTION("list returns no archives when borgmatic list fails on an accessible repository") {
    FakeBorgmatic borgmatic{R"(case "$1" in
  info) echo '[{"repository": {"id": "id", "location": "/tmp"}}]';;
  list) echo "Failed to create/acquire the lock" >&2; exit 2;;
esac)"};
    ScriptedWorker::script = borgmatic.path();

    std::vector<backup::helper::ListItem> list;
    REQUIRE_NOTHROW(list = backupConfig.list());
    REQUIRE(list.empty());
  }

  SECTION("list returns no archives for unexpected json") {
    FakeBorgmatic borgmatic{R"(case "$1" in
  info) echo '[{"repository": {"id": "id", "location": "/tmp"}}]';;
  list) echo '{"archives": 42}';;
esac)"};
    ScriptedWorker::script = borgmatic.path();

    std::vector<backup::helper::ListItem> list;
    REQUIRE_NOTHROW(list = backupConfig.list());
    REQUIRE(list.empty());
  }

  SECTION("list returns the archives on success") {
    FakeBorgmatic borgmatic{R"(case "$1" in
  info) echo '[{"repository": {"id": "id", "location": "/tmp"}}]';;
  list) echo '[{"archives": [{"id": "a1", "name": "n1", "start": "2024-01-01T10:00:00.000000"}]}]';;
esac)"};
    ScriptedWorker::script = borgmatic.path();

    auto list = backupConfig.list();
    REQUIRE(list.size() == 1);
    REQUIRE(list[0].name == "n1");
  }

  SECTION("info survives invalid json") {
    FakeBorgmatic borgmatic{"echo 'no json'"};
    ScriptedWorker::script = borgmatic.path();

    backup::helper::Info info;
    REQUIRE_NOTHROW(info = backupConfig.info());
    REQUIRE(info.location.empty());
  }

  SECTION("info survives a failure without error output") {
    FakeBorgmatic borgmatic{"exit 1"};
    ScriptedWorker::script = borgmatic.path();

    backup::helper::Info info;
    REQUIRE_NOTHROW(info = backupConfig.info());
    REQUIRE(info.location.empty());
  }

  SECTION("info survives a failure with a single word error") {
    FakeBorgmatic borgmatic{"echo 'Error' >&2; exit 1"};
    ScriptedWorker::script = borgmatic.path();

    backup::helper::Info info;
    REQUIRE_NOTHROW(info = backupConfig.info());
    REQUIRE(info.location.empty());
  }

  SECTION("info takes the repository location from the error of an inaccessible repository") {
    FakeBorgmatic borgmatic{"echo 'Repository /mnt/usb/repo does not exist.' >&2; exit 2"};
    ScriptedWorker::script = borgmatic.path();

    REQUIRE(backupConfig.info().location == "/mnt/usb/repo");
  }

  SECTION("info is not cached for a config file replaced while borgmatic was running") {
    // $4 is the config file passed via -c
    FakeBorgmatic borgmatic{R"(sleep 0.5; echo "[{\"repository\": {\"id\": \"id\", \"location\": \"$4\"}}]")"};
    ScriptedWorker::script = borgmatic.path();

    auto runningInfo = std::async(std::launch::async, [&backupConfig] { return backupConfig.info(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    backupConfig.borgmaticConfigFile("/other/config.yaml");

    REQUIRE(runningInfo.get().location == "/some/config.yaml");
    REQUIRE(backupConfig.info().location == "/other/config.yaml");
  }

  SECTION("mount and umount report success") {
    FakeBorgmatic borgmatic{"exit 0"};
    ScriptedWorker::script = borgmatic.path();

    REQUIRE(backupConfig.mountArchive("archive", "/mnt/point") == true);
    REQUIRE(backupConfig.umountArchive("/mnt/point") == true);
  }

  SECTION("mount and umount report failure") {
    FakeBorgmatic borgmatic{"echo 'mount failed' >&2; exit 1"};
    ScriptedWorker::script = borgmatic.path();

    REQUIRE(backupConfig.mountArchive("archive", "/mnt/point") == false);
    REQUIRE(backupConfig.umountArchive("/mnt/point") == false);
  }
}

static std::optional<int> runBackup(BorgmaticBackupWorker& worker, std::vector<std::string>& output) {
  std::optional<int> exitCode;
  worker.configure("/some/config.yaml", false);
  worker.start([&exitCode](int code) { exitCode = code; }, [&output](std::string const& line) { output.push_back(line); });
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while (!exitCode && std::chrono::steady_clock::now() < deadline) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
  }
  return exitCode;
}

TEST_CASE("BorgmaticBackupWorker reports borgmatic's exit code", "[logic]") {
  std::vector<std::string> output;

  SECTION("successful backup") {
    FakeBorgmatic borgmatic{R"(echo "args: $*"; exit 0)"};
    BorgmaticBackupWorker worker{borgmatic.path()};

    REQUIRE(runBackup(worker, output) == 0);
    REQUIRE(output == std::vector<std::string>{"args: --config /some/config.yaml create --progress check"});
  }

  SECTION("failed backup") {
    FakeBorgmatic borgmatic{"echo 'working'; exit 2"};
    BorgmaticBackupWorker worker{borgmatic.path()};

    REQUIRE(runBackup(worker, output) == 2);
  }

  SECTION("borgmatic cannot be started") {
    BorgmaticBackupWorker worker{"/not-found"};

    REQUIRE(runBackup(worker, output) == -1);
  }
}

TEST_CASE("BorgmaticBackupWorker cancelAndWait", "[logic]") {
  FakeBorgmatic borgmatic{R"(trap 'echo interrupted; exit 130' INT
echo started
i=0; while [ $i -lt 100 ]; do sleep 0.1; i=$((i+1)); done)"};

  SECTION("stops a running backup without calling onFinished") {
    BorgmaticBackupWorker worker{borgmatic.path()};
    bool finishedCalled = false;
    worker.configure("/some/config.yaml", false);
    worker.start([&finishedCalled](int) { finishedCalled = true; }, [](std::string const&) {});
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    REQUIRE(worker.isRunning());

    auto start = std::chrono::steady_clock::now();
    worker.cancelAndWait();

    REQUIRE_FALSE(worker.isRunning());
    REQUIRE(std::chrono::steady_clock::now() - start < std::chrono::seconds(5));
    QCoreApplication::processEvents();
    REQUIRE_FALSE(finishedCalled);
  }

  SECTION("destroying a worker with a running backup is safe") {
    std::vector<std::string> output;
    {
      BorgmaticBackupWorker worker{borgmatic.path()};
      worker.configure("/some/config.yaml", false);
      worker.start([](int) {}, [&output](std::string const& line) { output.push_back(line); });
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    QCoreApplication::processEvents();
    REQUIRE(output.front() == "started");
  }
}
