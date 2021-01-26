#include "BackupConfig.h"

std::string BackupConfig::borgmaticConfigFile() const { return pathToConfig.string(); }

void BackupConfig::borgmaticConfigFile(std::string const& configFile) {
  pathToConfig = std::filesystem::path(configFile);
}
