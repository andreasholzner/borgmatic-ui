#include "BorgmaticBackupWorker.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <chrono>
#include <csignal>
#include <memory>
#include <thread>
#include <utility>

namespace {
// borg's progress output separates updates with '\r', so both '\r' and '\n' end a line.
struct LineEnd {
  template <typename Iterator>
  std::pair<Iterator, bool> operator()(Iterator begin, Iterator end) const {
    auto lineEnd = std::find_if(begin, end, [](char c) { return c == '\r' || c == '\n'; });
    return lineEnd == end ? std::pair{end, false} : std::pair{std::next(lineEnd), true};
  }
};

std::string takeLine(boost::asio::streambuf &buf, std::size_t size) {
  auto begin = boost::asio::buffers_begin(buf.data());
  std::string line(begin, begin + static_cast<std::ptrdiff_t>(size));
  buf.consume(size);
  while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
    line.pop_back();
  }
  return line;
}
}  // namespace

template <>
struct boost::asio::is_match_condition<LineEnd> : std::true_type {};

void backup::helper::readOutput(std::shared_ptr<boost::asio::readable_pipe> pipe, boost::asio::streambuf &buf,
                                std::function<void(std::string)> const &outputHandler) {
  boost::asio::async_read_until(
      *pipe, buf, LineEnd{}, [pipe, &buf, outputHandler](boost::system::error_code const &ec, std::size_t size) {
        if (!ec) {
          // A line end directly following another one gives an empty line; there's nothing to report then.
          if (auto line = takeLine(buf, size); !line.empty()) {
            outputHandler(line);
          }
          readOutput(pipe, buf, outputHandler);
          return;
        }
        // The last line may not be terminated.
        if (auto line = takeLine(buf, buf.size()); !line.empty()) {
          outputHandler(line);
        }
      });
}

void BorgmaticBackupWorker::configure(std::filesystem::path const &pathToConfig, bool purgeFlag) {
  pathToConfig_ = pathToConfig;
  purgeFlag_ = purgeFlag;
}

BorgmaticBackupWorker::~BorgmaticBackupWorker() {
  // The io thread uses buffer and the output handler, so it must not outlive the worker.
  cancelAndWait();
}

bool BorgmaticBackupWorker::isRunning() { return backupFuture.isRunning(); }

void BorgmaticBackupWorker::cancel() {
  if (isRunning() && backupProcess) {
    auto pid = backupProcess->id();
    kill(pid, SIGINT);
  }
}

void BorgmaticBackupWorker::cancelAndWait() {
  using namespace std::chrono_literals;
  backupWatcher.disconnect();
  if (!isRunning()) {
    return;
  }
  cancel();
  auto deadline = std::chrono::steady_clock::now() + 60s;
  while (!backupFuture.isFinished() && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(100ms);
  }
  if (!backupFuture.isFinished()) {
    spdlog::warn("borgmatic did not exit after SIGINT, killing it");
    kill(backupProcess->id(), SIGKILL);
    // Child processes of borgmatic may still hold the output pipe open, so don't wait for its end.
    ioContext->stop();
  }
  backupFuture.waitForFinished();
}

void BorgmaticBackupWorker::createChildProcess(boost::asio::readable_pipe &ioPipe) {
  namespace bp = boost::process;
  // stdout and stderr share one pipe, so their lines stay in order. borgmatic reports progress on stderr. Our copy of
  // the write end is closed when leaving this function, so the pipe ends when borgmatic exits.
  boost::asio::writable_pipe outputEnd{ioPipe.get_executor()};
  boost::asio::connect_pipe(ioPipe, outputEnd);
  if (purgeFlag_) {
    backupProcess.emplace(*ioContext, executable().string(),
                          std::vector<std::string>{"--config", pathToConfig_.string(), "prune", "create", "--progress",
                                                   "check"},
                          bp::process_stdio{{}, outputEnd, outputEnd});
    spdlog::info("{} --config {} prune create --progress check", executable_.string(), pathToConfig_.string());
  } else {
    backupProcess.emplace(*ioContext, executable().string(),
                          std::vector<std::string>{"--config", pathToConfig_.string(), "create", "--progress", "check"},
                          bp::process_stdio{{}, outputEnd, outputEnd});
    spdlog::info("{} --config {} create --progress check", executable_.string(), pathToConfig_.string());
  }
}
