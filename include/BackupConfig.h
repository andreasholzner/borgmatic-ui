#ifndef BORGMATIC_UI_BACKUPCONFIG_H
#define BORGMATIC_UI_BACKUPCONFIG_H

#include <filesystem>
#include <string>

class BackupConfig {
 public:
  std::string borgmaticConfigFile() const;
  void borgmaticConfigFile(std::string const&);

  template <class Archive>
  void save(Archive& ar) const {
    ar(pathToConfig.string());
  }
  template <class Archive>
  void load(Archive& ar) {
    std::string filePath;
    ar(filePath);
    pathToConfig = std::filesystem::path(filePath);
  }

 private:
  std::filesystem::path pathToConfig;
};

#endif  // BORGMATIC_UI_BACKUPCONFIG_H
