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

#include "BackupWorker.h"

namespace backup::helper {
struct ListItem {
  std::string id;
  std::string name;
  std::string start;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ListItem, id, name, start)

struct Info {
  std::string id;
  std::string location;
  std::int64_t originalSize;
  std::int64_t compressedSize;

  Info() : id(""), location(""), originalSize(0), compressedSize(0) {}
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
  virtual std::vector<backup::helper::ListItem> list() = 0;
  virtual backup::helper::Info info() = 0;
  virtual void startBackup(
      std::function<void()> onFinished,
      std::function<void(std::string)> const& outputHandler = [](std::string const&) {}) = 0;
  virtual void cancelBackup() = 0;
  virtual bool isAccessible() = 0;
};

template <class T>
concept Worker = requires(T w) {
  w.start(&backup::helper::handler, &backup::helper::logHandler);
  w.cancel();
};

template <Worker W>
class BackupConfigImpl : public BackupConfig {
 public:
  std::string borgmaticConfigFile() const override;
  bool isBackupPurging() const override;
  void isBackupPurging(bool state) override;
  void borgmaticConfigFile(std::string const&) override;
  std::vector<backup::helper::ListItem> list() override;
  backup::helper::Info info() override;
  void startBackup(
      std::function<void()> onFinished,
      std::function<void(std::string)> const& outputHandler = [](std::string const&) {}) override;
  void cancelBackup() override;
  bool isAccessible() override;

  template <typename Archive>
  void save(Archive& ar) const {
    ar(pathToConfig.string(), purgeFlag);
  }
  template <typename Archive>
  void load(Archive& ar) {
    std::string filePath;
    ar(filePath, purgeFlag);
    pathToConfig = std::filesystem::path(filePath);
  }

 private:
  std::variant<nlohmann::json, std::vector<std::string>> runSimpleBorgmaticCommandOnConfig(
      std::string const& action) const;

  std::filesystem::path pathToConfig;
  bool purgeFlag;
  W worker;
  std::optional<backup::helper::Info> info_;
};

#include "BackupConfig.inc"

#endif  // BORGMATIC_UI_BACKUPCONFIG_H
