#ifndef BORGMATIC_UI_BACKUPCONFIG_H
#define BORGMATIC_UI_BACKUPCONFIG_H

#include <spdlog/spdlog.h>

#include <filesystem>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
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
}  // namespace backup::helper

class BackupConfig {
 public:
  std::string borgmaticConfigFile() const;
  bool isBackupPurging() const;
  void isBackupPurging(bool state);
  void borgmaticConfigFile(std::string const&);
  std::vector<backup::helper::ListItem> list();
  backup::helper::Info info();
  void startBackup(
      std::function<void()> onFinished,
      std::function<void(std::string)> const& outputHandler = [](std::string const&) {});
  void cancelBackup();

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
  nlohmann::json runSimpleBorgmaticCommandOnConfig(std::string const& action) const;

  std::filesystem::path pathToConfig;
  bool purgeFlag;
  BackupWorker worker;
};

#endif  // BORGMATIC_UI_BACKUPCONFIG_H
