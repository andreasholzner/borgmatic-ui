#include "ProcessRunner.h"

#include <spdlog/spdlog.h>
#include <unistd.h>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <cstdlib>
#include <vector>

std::optional<std::filesystem::path> backup::helper::findExecutable(std::string const& name,
                                                                   std::string const& searchPath) {
  std::vector<std::string> directories;
  boost::split(directories, searchPath, [](auto c) { return c == ':'; });
  for (auto const& directory : directories) {
    if (directory.empty()) {
      continue;
    }
    auto candidate = std::filesystem::path(directory) / name;
    std::error_code ec;
    if (std::filesystem::is_regular_file(candidate, ec) && access(candidate.c_str(), X_OK) == 0) {
      return candidate;
    }
  }
  return std::nullopt;
}

std::filesystem::path const& backup::helper::borgmaticExecutable() {
  static std::filesystem::path const executable = [] {
    auto const* path = std::getenv("PATH");
    auto found = findExecutable("borgmatic", path ? path : "");
    if (!found) {
      spdlog::warn("borgmatic not found on PATH, falling back to /usr/bin/borgmatic");
      return std::filesystem::path{"/usr/bin/borgmatic"};
    }
    spdlog::info("Using borgmatic executable {}", found->string());
    return *found;
  }();
  return executable;
}

backup::helper::ProcessResult backup::helper::runProcess(std::filesystem::path const& executable,
                                                         std::vector<std::string> const& args) {
  namespace asio = boost::asio;
  namespace bp = boost::process;

  asio::io_context ioContext;
  asio::readable_pipe outPipe{ioContext}, errPipe{ioContext};
  bp::process process{ioContext, executable.string(), args, bp::process_stdio{{}, outPipe, errPipe}};

  // Both pipes are drained concurrently, so the child can't block on a full pipe buffer.
  ProcessResult result{};
  auto ignoreEof = [](boost::system::error_code const&, std::size_t) {};
  asio::async_read(outPipe, asio::dynamic_buffer(result.stdOut), ignoreEof);
  asio::async_read(errPipe, asio::dynamic_buffer(result.stdErr), ignoreEof);
  ioContext.run();

  result.exitCode = process.wait();
  return result;
}
