#include "BackupConfig.h"

std::string BackupConfig::borgmaticConfig() const { return pathToConfig.string(); }
void BackupConfig::borgmaticConfig(const std::string &configFile) { pathToConfig = std::filesystem::path(configFile); }
