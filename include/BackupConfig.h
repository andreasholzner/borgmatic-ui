#ifndef BORGMATIC_UI_BACKUPCONFIG_H
#define BORGMATIC_UI_BACKUPCONFIG_H

#include <spdlog/spdlog.h>

#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

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
  void borgmaticConfigFile(std::string const&);
  std::vector<backup::helper::ListItem> list();
  backup::helper::Info info();

  template <typename Archive>
  void save(Archive& ar) const {
    ar(pathToConfig.string());
  }
  template <typename Archive>
  void load(Archive& ar) {
    std::string filePath;
    ar(filePath);
    pathToConfig = std::filesystem::path(filePath);
  }

 private:
  nlohmann::json runSimpleBorgmaticCommandOnConfig(std::string const& action) const;

  std::filesystem::path pathToConfig;
};

#endif  // BORGMATIC_UI_BACKUPCONFIG_H
