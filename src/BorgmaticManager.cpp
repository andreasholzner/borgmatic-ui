#include "BorgmaticManager.h"

#include <QSettings>
#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <spdlog/spdlog.h>

static char const *const SETTINGS_KEY = "backup_configs";

std::shared_ptr<BackupConfig> BorgmaticManagerImpl::newBorgmaticConfig() {
  auto newConfig = std::make_shared<BackupConfigImpl<BackupWorker>>();
  configs_.push_back(newConfig);
  return std::static_pointer_cast<BackupConfig>(newConfig);
}

void BorgmaticManagerImpl::removeConfig(int index) {
  if (index < configs_.size()) {
    configs_.erase(configs_.cbegin() + index);
  } else {
    spdlog::warn("Trying to remove non-existing config at index {} when size is {}", index, configs_.size());
  }
}

std::vector<std::shared_ptr<BackupConfig>> BorgmaticManagerImpl::configs() {
  std::vector<std::shared_ptr<BackupConfig>> res{configs_.size()};
  std::transform(configs_.begin(), configs_.end(), res.begin(), [](std::shared_ptr<BackupConfigImpl<BackupWorker>> c) {
    return std::static_pointer_cast<BackupConfig>(c);
  });
  return res;
}

std::string BorgmaticManagerImpl::store() {
  std::stringstream ss;
  serialize(ss);
  return ss.str();
}

void BorgmaticManagerImpl::loadSettings() {
  QSettings settings;
  spdlog::info("Loading backup settings");
  auto backupConfigs = settings.value(SETTINGS_KEY);
  if (!backupConfigs.isNull()) {
    auto loadedSettings = backupConfigs.toString().toStdString();
    spdlog::debug("loading settings: {}", loadedSettings);
    load(loadedSettings);
  }
}

void BorgmaticManagerImpl::saveSettings() {
  QSettings settings;
  spdlog::info("Saving backup settings");
  settings.setValue(SETTINGS_KEY, QString(store().c_str()));
}

void BorgmaticManagerImpl::load(std::string const &data) {
  std::stringstream ss(data);
  cereal::JSONInputArchive archive(ss);
  archive(configs_);
}
void BorgmaticManagerImpl::serialize(std::stringstream &archiveStream) {
  cereal::JSONOutputArchive archive(archiveStream);
  archive(configs_);
}
