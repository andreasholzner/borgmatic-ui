#ifndef BORGMATIC_UI_BORGMATICMANAGER_H
#define BORGMATIC_UI_BORGMATICMANAGER_H

#include <memory>
#include <vector>

#include "BackupConfig.h"

class BorgmaticManager {
 public:
  virtual ~BorgmaticManager() = default;
  virtual std::shared_ptr<BackupConfig> newBorgmaticConfig() = 0;
  virtual void removeConfig(int index) = 0;
  virtual void loadSettings() = 0;
  virtual void saveSettings() = 0;
  virtual std::vector<std::shared_ptr<BackupConfig>> configs() = 0;
};

class BorgmaticManagerImpl : public BorgmaticManager {
 public:
  std::shared_ptr<BackupConfig> newBorgmaticConfig() override;
  void removeConfig(int index) override;
  std::vector<std::shared_ptr<BackupConfig>> configs() override;
  void loadSettings() override;
  void saveSettings() override;

 private:
  std::string store();
  void load(const std::string&);
  void serialize(std::stringstream&);

  std::vector<std::shared_ptr<BackupConfigImpl>> configs_;
};

#endif  // BORGMATIC_UI_BORGMATICMANAGER_H
