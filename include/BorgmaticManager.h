#ifndef BORGMATIC_UI_BORGMATICMANAGER_H
#define BORGMATIC_UI_BORGMATICMANAGER_H

#include <vector>
#include <memory>

#include "BackupConfig.h"

class BorgmaticManager {
public:
    std::shared_ptr<BackupConfig> newBorgmaticConfig();
    void removeConfig(int index);

private:
    std::vector<std::shared_ptr<BackupConfig>> configs;
};

#endif //BORGMATIC_UI_BORGMATICMANAGER_H
