#include <spdlog/spdlog.h>

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/process.hpp>
#include <filesystem>

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
std::vector<backup::helper::ListItem> BackupConfigImpl<W>::list() {
  if (!isAccessible()) {
    return {};
  }

  auto jsonList = std::get<0>(runSimpleBorgmaticCommandOnConfig("list"));

  std::vector<backup::helper::ListItem> result;
  std::ranges::transform(
      jsonList[0]["archives"], std::back_inserter(result),
      [](nlohmann::json::reference j) -> auto { return j.get<backup::helper::ListItem>(); });
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
    auto infoResult = runSimpleBorgmaticCommandOnConfig("info");
    Info info;
    if (auto jsonInfo = std::get_if<nlohmann::json>(&infoResult)) {
      info.id = safeJsonAccess<std::string>([jsonInfo]() -> auto { return (*jsonInfo)[0]["repository"]["id"]; });
      info.location =
          safeJsonAccess<std::string>([jsonInfo]() -> auto { return (*jsonInfo)[0]["repository"]["location"]; });
      info.originalSize = safeJsonAccess<std::uint64_t>([jsonInfo]() -> auto {
        return (*jsonInfo)[0]["cache"]["stats"]["total_size"];
      });
      info.compressedSize =
          safeJsonAccess<std::uint64_t>([jsonInfo]() { return (*jsonInfo)[0]["cache"]["stats"]["total_csize"]; });
    } else {
      auto errorLines = std::get<1>(infoResult);
      std::vector<std::string> parts;
      boost::split(parts, errorLines[0], [](auto c) { return c == ' '; });
      info.location = parts[1];
    }
    info_ = info;
  }
  return info_.value();
}

template <Worker W>
void BackupConfigImpl<W>::startBackup(std::function<void()> onFinished,
                                      std::function<void(std::string)> const& outputHandler) {
  // TODO hand down config filename and purge flag
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
std::variant<nlohmann::json, std::vector<std::string>> BackupConfigImpl<W>::runSimpleBorgmaticCommandOnConfig(
    std::string const& action) const {
  namespace bp = boost::process;
  bp::ipstream output, errorOutput;
  auto code = bp::system("/usr/bin/borgmatic", action, "--json", "-c", pathToConfig.string(), bp::std_out > output,
                         bp::std_err > errorOutput);

  if (code) {
    spdlog::info("borgmatic {} failed: {}", action, code);
    std::vector<std::string> collectedOutput;
    std::string line;
    while (std::getline(errorOutput, line)) {
      collectedOutput.push_back(line);
    }
    return collectedOutput;
  }
  nlohmann::json jsonList;
  output >> jsonList;
  spdlog::debug(jsonList.dump(4));
  return jsonList;
}

template <Worker W>
bool BackupConfigImpl<W>::isAccessible() {
  auto backupInfo = info();
  if (backupInfo.location.starts_with("ssh:")) {
    return true;
  }
  return std::filesystem::exists(backupInfo.location);
}
