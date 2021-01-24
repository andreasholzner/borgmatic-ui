#ifndef BORGMATIC_UI_BACKUPCONFIG_H
#define BORGMATIC_UI_BACKUPCONFIG_H

#include <string>
#include <filesystem>

class BackupConfig {
 public:
  std::string borgmaticConfig() const;
  void borgmaticConfig(const std::string&);
 private:
  std::filesystem::path pathToConfig;
};

#endif  // BORGMATIC_UI_BACKUPCONFIG_H
