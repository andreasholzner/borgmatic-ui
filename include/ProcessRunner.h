#ifndef BORGMATIC_UI_INCLUDE_PROCESSRUNNER_H_
#define BORGMATIC_UI_INCLUDE_PROCESSRUNNER_H_

#include <filesystem>
#include <string>
#include <vector>

namespace backup::helper {
struct ProcessResult {
  int exitCode;
  std::string stdOut;
  std::string stdErr;
};

// Runs the executable synchronously and collects its complete output. Throws if the process cannot be started.
ProcessResult runProcess(std::filesystem::path const& executable, std::vector<std::string> const& args);
}  // namespace backup::helper

#endif  // BORGMATIC_UI_INCLUDE_PROCESSRUNNER_H_
