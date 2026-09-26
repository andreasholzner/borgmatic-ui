#include <catch2/catch_all.hpp>
#include <filesystem>
#include <string>

#include "ProcessRunner.h"
#include "test_helper.h"

using backup::helper::findExecutable;
using backup::helper::runProcess;

TEST_CASE("findExecutable", "[logic]") {
  FakeBorgmatic executable{"exit 0"};
  auto directory = executable.path().parent_path().string();
  QTemporaryDir otherDirectory;

  SECTION("finds an executable in the search path") {
    REQUIRE(findExecutable("borgmatic", directory) == executable.path());
  }

  SECTION("searches all directories in order") {
    auto searchPath = "/does-not-exist::" + otherDirectory.path().toStdString() + ":" + directory;
    REQUIRE(findExecutable("borgmatic", searchPath) == executable.path());
  }

  SECTION("ignores files that are not executable") {
    std::filesystem::permissions(executable.path(), std::filesystem::perms::owner_read);
    REQUIRE_FALSE(findExecutable("borgmatic", directory).has_value());
  }

  SECTION("ignores directories") {
    std::filesystem::create_directory(otherDirectory.path().toStdString() + "/borgmatic");
    REQUIRE_FALSE(findExecutable("borgmatic", otherDirectory.path().toStdString()).has_value());
  }

  SECTION("returns nothing for an empty search path") { REQUIRE_FALSE(findExecutable("borgmatic", "").has_value()); }
}

TEST_CASE("runProcess", "[logic]") {
  SECTION("collects exit code and output") {
    FakeBorgmatic borgmatic{R"(echo "out $1"; echo "err $2" >&2; exit 3)"};
    auto result = runProcess(borgmatic.path(), {"a", "b"});
    REQUIRE(result.exitCode == 3);
    REQUIRE(result.stdOut == "out a\n");
    REQUIRE(result.stdErr == "err b\n");
  }

  SECTION("does not block on output larger than the pipe buffer") {
    FakeBorgmatic borgmatic{"head -c 1000000 /dev/zero; head -c 1000000 /dev/zero >&2"};
    auto result = runProcess(borgmatic.path(), {});
    REQUIRE(result.exitCode == 0);
    REQUIRE(result.stdOut.size() == 1000000);
    REQUIRE(result.stdErr.size() == 1000000);
  }

  SECTION("throws if the executable can't be started") { REQUIRE_THROWS(runProcess("/not-found", {})); }
}
