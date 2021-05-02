#ifndef BORGMATIC_UI_BACKUPCONFIG_H
#define BORGMATIC_UI_BACKUPCONFIG_H

#include <spdlog/spdlog.h>

#include <filesystem>
#include <functional>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "BorgmaticBackupWorker.h"

namespace backup::helper {
struct ListItem {
  std::string id;
  std::string name;
  std::string start;
  bool is_mounted;
  std::string mount_path;

  ListItem() : id(), name(), start(), is_mounted(false), mount_path() {}
  ListItem(std::string const& id_, std::string const& name_, std::string const& start_, bool is_mounted_ = false,
           std::string const& mount_path_ = "")
      : id(id_), name(name_), start(start_), is_mounted(is_mounted_), mount_path(mount_path_) {}
  bool operator==(ListItem const&) const = default;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ListItem, id, name, start)

struct Info {
  std::string id;
  std::string location;
  std::int64_t originalSize;
  std::int64_t compressedSize;

  Info() : id(), location(), originalSize(0), compressedSize(0) {}
};

template <typename T>
T safeJsonAccess(std::function<T()> input, T defaultValue = T()) {
  T result = defaultValue;
  try {
    result = input();
  } catch (nlohmann::json::exception& e) {
    spdlog::warn("Error parsing json: error code={}, description={}", e.id, e.what());
  }
  return result;
}

constexpr void handler() {}
constexpr void logHandler(std::string const& msg) {}
}  // namespace backup::helper

class BackupConfig {
 public:
  virtual ~BackupConfig() = default;
  virtual std::string borgmaticConfigFile() const = 0;
  virtual void borgmaticConfigFile(std::string const&) = 0;
  virtual bool isBackupPurging() const = 0;
  virtual void isBackupPurging(bool state) = 0;
  virtual bool isMountPointToBeOpened() const = 0;
  virtual void isMountPointToBeOpened(bool state) = 0;
  virtual std::vector<backup::helper::ListItem> list() = 0;
  virtual backup::helper::Info info() = 0;
  virtual void startBackup(
      std::function<void()> onFinished,
      std::function<void(std::string)> const& outputHandler = [](std::string const&) {}) = 0;
  virtual void cancelBackup() = 0;
  virtual void mountArchive(std::string const& archiveName, std::string const& mountPoint) = 0;
  virtual void umountArchive(std::string const& mountPoint) = 0;
};

template <class T>
concept Worker = requires(T w) {
  w.configure(std::filesystem::path{}, true);
  w.start(&backup::helper::handler, &backup::helper::logHandler);
  w.cancel();
};

template <Worker W>
class BackupConfigImpl : public BackupConfig {
 public:
  std::string borgmaticConfigFile() const override;
  bool isBackupPurging() const override;
  void isBackupPurging(bool state) override;
  bool isMountPointToBeOpened() const override;
  void isMountPointToBeOpened(bool state) override;
  void borgmaticConfigFile(std::string const&) override;
  std::vector<backup::helper::ListItem> list() override;
  backup::helper::Info info() override;
  void startBackup(
      std::function<void()> onFinished,
      std::function<void(std::string)> const& outputHandler = [](std::string const&) {}) override;
  void cancelBackup() override;
  void mountArchive(std::string const& archiveName, std::string const& mountPoint) override;
  void umountArchive(std::string const& mountPoint) override;

  template <typename Archive>
  void save(Archive& ar) const {
    ar(pathToConfig.string(), purgeFlag, openMountFlag);
  }
  template <typename Archive>
  void load(Archive& ar) {
    std::string filePath;
    ar(filePath, purgeFlag, openMountFlag);
    pathToConfig = std::filesystem::path(filePath);
  }

 private:
  std::variant<nlohmann::json, std::vector<std::string>> runSimpleBorgmaticCommandWithJsonOutputOnConfig(
      std::string const& action) const;
  template <typename... Arg>
  void runSimpleBorgmaticCommandOnConfig(std::string const& action, Arg... args) const;
  bool isAccessible();

  std::filesystem::path pathToConfig;
  bool purgeFlag;
  bool openMountFlag;
  W worker;
  std::optional<backup::helper::Info> info_;
};

#include "BackupConfig.inc"

#endif  // BORGMATIC_UI_BACKUPCONFIG_H
