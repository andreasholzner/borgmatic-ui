#ifndef BORGMATIC_UI_BORGMATICMANAGER_H
#define BORGMATIC_UI_BORGMATICMANAGER_H

#include <memory>
#include <vector>

#include "BackupConfig.h"

class BorgmaticManager {
 public:
  std::shared_ptr<BackupConfig> newBorgmaticConfig();
  void removeConfig(int index);
  void loadSettings();
  void saveSettings();

  std::vector<std::shared_ptr<BackupConfig>> configs;

 private:
  std::string store();
  void load(const std::string&);
  void serialize(std::stringstream&);
};

#endif  // BORGMATIC_UI_BORGMATICMANAGER_H
