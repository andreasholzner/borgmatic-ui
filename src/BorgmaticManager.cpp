#include "BorgmaticManager.h"

#include <QSettings>
#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>

static char const *const SETTINGS_KEY = "backup_configs";

std::shared_ptr<BackupConfig> BorgmaticManager::newBorgmaticConfig() {
  std::shared_ptr<BackupConfig> newConfig = std::make_shared<BackupConfig>();
  configs.push_back(newConfig);
  return newConfig;
}

void BorgmaticManager::removeConfig(int index) { configs.erase(configs.cbegin() + index); }

std::string BorgmaticManager::store() {
  std::stringstream ss;
  serialize(ss);
  return ss.str();
}

void BorgmaticManager::loadSettings() {
  QSettings settings;
  auto backupConfigs = settings.value(SETTINGS_KEY);
  if (!backupConfigs.isNull()) {
    load(backupConfigs.toString().toStdString());
  }
}

void BorgmaticManager::saveSettings() {
  QSettings settings;
  settings.setValue(SETTINGS_KEY, QString(store().c_str()));
}

void BorgmaticManager::load(std::string const &data) {
  std::stringstream ss(data);
  cereal::JSONInputArchive archive(ss);
  archive(configs);
}

void BorgmaticManager::serialize(std::stringstream &archiveStream) {
  cereal::JSONOutputArchive archive(archiveStream);
  archive(configs);
}
