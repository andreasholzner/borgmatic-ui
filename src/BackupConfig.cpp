#include "BackupConfig.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <boost/process.hpp>
#include <ranges>

std::string BackupConfig::borgmaticConfigFile() const { return pathToConfig.string(); }

void BackupConfig::borgmaticConfigFile(std::string const& configFile) {
  pathToConfig = std::filesystem::path(configFile);
}

bool BackupConfig::isBackupPurging() const { return purgeFlag; }
void BackupConfig::isBackupPurging(bool state) { purgeFlag = state; }

std::vector<backup::helper::ListItem> BackupConfig::list() {
  auto jsonList = runSimpleBorgmaticCommandOnConfig("list");

  std::vector<backup::helper::ListItem> result;
  std::ranges::transform(
      jsonList[0]["archives"], std::back_inserter(result),
      [](nlohmann::json::reference j) -> auto { return j.get<backup::helper::ListItem>(); });
  return result;
}

backup::helper::Info BackupConfig::info() {
  using namespace backup::helper;
  auto jsonInfo = runSimpleBorgmaticCommandOnConfig("info")[0];
  Info info;
  info.id = safeJsonAccess<std::string>([&jsonInfo]() -> auto { return jsonInfo["repository"]["id"]; });
  info.location = safeJsonAccess<std::string>([&jsonInfo]() -> auto { return jsonInfo["repository"]["location"]; });
  info.originalSize =
      safeJsonAccess<std::uint64_t>([&jsonInfo]() -> auto { return jsonInfo["cache"]["stats"]["total_size"]; });
  info.compressedSize =
      safeJsonAccess<std::uint64_t>([&jsonInfo]() { return jsonInfo["cache"]["stats"]["total_csize"]; });
  return info;
}

nlohmann::json BackupConfig::runSimpleBorgmaticCommandOnConfig(std::string const& action) const {
  namespace bp = boost::process;
  bp::ipstream output;
  bp::child process("/usr/bin/borgmatic", action, "--json", "-c", pathToConfig.string(), bp::std_out > output);

  nlohmann::json jsonList;
  output >> jsonList;
  process.wait();
  spdlog::debug(jsonList.dump(4));
  return jsonList;
}
