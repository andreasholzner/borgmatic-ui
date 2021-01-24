#include "BorgmaticManager.h"

std::shared_ptr<BackupConfig> BorgmaticManager::newBorgmaticConfig() {
  std::shared_ptr<BackupConfig> newConfig = std::make_shared<BackupConfig>();
  configs.push_back(newConfig);
  return newConfig;
}

void BorgmaticManager::removeConfig(int index) { configs.erase(configs.cbegin() + index); }
