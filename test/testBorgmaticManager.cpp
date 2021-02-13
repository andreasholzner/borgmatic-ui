#include <catch2/catch.hpp>
#include <memory>

#include "BackupConfig.h"
#include "BorgmaticManager.h"

TEST_CASE("BorgmaticManager", "[logic]") {
  std::shared_ptr<BorgmaticManager> borgmaticManager = std::make_shared<BorgmaticManagerImpl>();

  SECTION("newBorgmaticConfig returns new config and stores it") {
    auto config = borgmaticManager->newBorgmaticConfig();

    REQUIRE(borgmaticManager->configs().size() == 1);
    REQUIRE(borgmaticManager->configs()[0] == config);
  }

  SECTION("newBorgmaticConfig adds new config") {
    borgmaticManager->newBorgmaticConfig();
    borgmaticManager->newBorgmaticConfig();

    REQUIRE(borgmaticManager->configs().size() == 2);
  }

  SECTION("removeConfig removes config entry") {
    auto config1 = borgmaticManager->newBorgmaticConfig();
    config1->borgmaticConfigFile("name1");
    auto config2 = borgmaticManager->newBorgmaticConfig();
    config2->borgmaticConfigFile("name2");

    borgmaticManager->removeConfig(0);

    REQUIRE(borgmaticManager->configs().size() == 1);
    REQUIRE(borgmaticManager->configs()[0]->borgmaticConfigFile() == config2->borgmaticConfigFile());
  }

  SECTION("can be saved and read from settings") {
    QCoreApplication::setOrganizationName("test_holzner");
    QCoreApplication::setApplicationName("test-borgmatic-ui");

    {
      auto config1 = borgmaticManager->newBorgmaticConfig();
      config1->borgmaticConfigFile("name1");
      auto config2 = borgmaticManager->newBorgmaticConfig();
      config2->borgmaticConfigFile("name2");

      borgmaticManager->saveSettings();
    }
    {
      std::shared_ptr<BorgmaticManager> loadedBorgmaticManager = std::make_shared<BorgmaticManagerImpl>();
      loadedBorgmaticManager->loadSettings();

      REQUIRE(loadedBorgmaticManager->configs().size() == 2);
      REQUIRE(loadedBorgmaticManager->configs()[0]->borgmaticConfigFile() == "name1");
      REQUIRE(loadedBorgmaticManager->configs()[1]->borgmaticConfigFile() == "name2");
    }
  }
}
