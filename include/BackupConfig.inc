#include <spdlog/spdlog.h>

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <sstream>

#include "ProcessRunner.h"

template <Worker W>
std::string BackupConfigImpl<W>::borgmaticConfigFile() const {
  return pathToConfig.string();
}

template <Worker W>
void BackupConfigImpl<W>::borgmaticConfigFile(std::string const& configFile) {
  pathToConfig = std::filesystem::path(configFile);
}

template <Worker W>
bool BackupConfigImpl<W>::isBackupPurging() const {
  return purgeFlag;
}
template <Worker W>
void BackupConfigImpl<W>::isBackupPurging(bool state) {
  purgeFlag = state;
}
template <Worker W>
bool BackupConfigImpl<W>::isMountPointToBeOpened() const {
  return openMountFlag;
}
template <Worker W>
void BackupConfigImpl<W>::isMountPointToBeOpened(bool state) {
  openMountFlag = state;
}

template <Worker W>
std::vector<backup::helper::ListItem> BackupConfigImpl<W>::list() {
  if (!isAccessible()) {
    return {};
  }

  auto listResult = runSimpleBorgmaticCommandWithJsonOutputOnConfig("list");
  auto jsonList = std::get_if<nlohmann::json>(&listResult);
  if (!jsonList) {
    spdlog::warn("Could not list archives of {}", pathToConfig.string());
    return {};
  }

  std::vector<backup::helper::ListItem> result;
  try {
    std::ranges::transform(
        (*jsonList)[0]["archives"], std::back_inserter(result),
        [](nlohmann::json::reference j) -> auto{ return j.get<backup::helper::ListItem>(); });
  } catch (nlohmann::json::exception const& e) {
    spdlog::warn("Unexpected borgmatic list output: {}", e.what());
    return {};
  }
  return result;
}

template <Worker W>
backup::helper::Info BackupConfigImpl<W>::info() {
  if (!info_) {
    if (pathToConfig.empty()) {
      spdlog::info("skipping borgmatic info due to unknown config");
      return {};
    }
    using namespace backup::helper;
    auto infoResult = runSimpleBorgmaticCommandWithJsonOutputOnConfig("info");
    Info info;
    if (auto jsonInfo = std::get_if<nlohmann::json>(&infoResult)) {
      info.id = safeJsonAccess<std::string>([jsonInfo]() -> auto{ return (*jsonInfo)[0]["repository"]["id"]; });
      info.location =
          safeJsonAccess<std::string>([jsonInfo]() -> auto{ return (*jsonInfo)[0]["repository"]["location"]; });
      info.originalSize =
          safeJsonAccess<std::uint64_t>([jsonInfo]() -> auto{ return (*jsonInfo)[0]["cache"]["stats"]["total_size"]; });
      info.compressedSize =
          safeJsonAccess<std::uint64_t>([jsonInfo]() { return (*jsonInfo)[0]["cache"]["stats"]["total_csize"]; });
    } else {
      auto const& errorLines = std::get<1>(infoResult);
      if (!errorLines.empty()) {
        std::vector<std::string> parts;
        boost::split(parts, errorLines[0], [](auto c) { return c == ' '; });
        if (parts.size() > 1) {
          info.location = parts[1];
        }
      }
    }
    info_ = info;
  }
  return info_.value();
}

template <Worker W>
void BackupConfigImpl<W>::startBackup(std::function<void(int)> onFinished,
                                      std::function<void(std::string)> const& outputHandler) {
  worker.configure(pathToConfig, purgeFlag);
  worker.start(std::move(onFinished), [outputHandler](std::string line) {
    spdlog::debug(line);
    outputHandler(line);
  });
}

template <Worker W>
void BackupConfigImpl<W>::cancelBackup() {
  worker.cancel();
}

template <Worker W>
bool BackupConfigImpl<W>::mountArchive(std::string const& archiveName, std::string const& mountPoint) {
  if (archiveName.empty()) {
    spdlog::error("Trying to mount unspecified archive!");
    return false;
  }
  if (mountPoint.empty()) {
    spdlog::error("Trying to mount a archive without a mount point!");
    return false;
  }
  return runSimpleBorgmaticCommandOnConfig("mount", "--archive", archiveName, "--mount-point", mountPoint);
}

template <Worker W>
bool BackupConfigImpl<W>::umountArchive(std::string const& mountPoint) {
  if (mountPoint.empty()) {
    spdlog::error("Trying to umount archive without a mount point!");
    return false;
  }
  return runSimpleBorgmaticCommandOnConfig("umount", "--mount-point", mountPoint);
}

template <Worker W>
std::variant<nlohmann::json, std::vector<std::string>>
BackupConfigImpl<W>::runSimpleBorgmaticCommandWithJsonOutputOnConfig(std::string const& action) const {
  backup::helper::ProcessResult result;
  try {
    result = backup::helper::runProcess(worker.executable(), {action, "--json", "-c", pathToConfig.string()});
  } catch (std::exception const& e) {
    spdlog::error("Error calling borgmatic executable {}, reason: {}", worker.executable().string(), e.what());
    return std::vector<std::string>{};
  }
  auto code = result.exitCode;
  std::istringstream output{result.stdOut}, errorOutput{result.stdErr};

  if (code) {
    spdlog::info("Calling borgmatic executable {} {} failed: {}", worker.executable().string(), action, code);
    std::vector<std::string> collectedOutput;
    std::string line;
    while (std::getline(errorOutput, line)) {
      collectedOutput.push_back(line);
    }
    return collectedOutput;
  }
  nlohmann::json jsonList;
  try {
    output >> jsonList;
  } catch (nlohmann::json::parse_error const& e) {
    spdlog::error("Invalid JSON output of borgmatic {}: {}", action, e.what());
    return std::vector<std::string>{};
  }
  spdlog::debug(jsonList.dump(4));
  return jsonList;
}

template <Worker W>
template <typename... Arg>
bool BackupConfigImpl<W>::runSimpleBorgmaticCommandOnConfig(std::string const& action, Arg... args) const {
  backup::helper::ProcessResult result;
  try {
    result = backup::helper::runProcess(worker.executable(),
                                        {action, "-c", pathToConfig.string(), std::string(args)...});
  } catch (std::exception const& e) {
    spdlog::error("Error calling borgmatic executable {}, reason: {}", worker.executable().string(), e.what());
    return false;
  }
  auto code = result.exitCode;
  std::istringstream output{result.stdOut}, errorOutput{result.stdErr};

  if (code) {
    spdlog::warn("Calling borgmatic executable {} {} failed: {}", worker.executable().string(), action, code);
  }
  std::string line;
  while (std::getline(output, line)) {
    spdlog::info("[borgmatic {} stdout]: {}", action, line);
  }
  while (std::getline(errorOutput, line)) {
    spdlog::warn("[borgmatic {} stderr]: {}", action, line);
  }
  return code == 0;
}

template <Worker W>
bool BackupConfigImpl<W>::isAccessible() {
  auto backupInfo = info();
  if (backupInfo.location.starts_with("ssh:")) {
    return true;
  }
  return std::filesystem::exists(backupInfo.location);
}
