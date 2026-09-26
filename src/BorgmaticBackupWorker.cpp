#include "BorgmaticBackupWorker.h"

#include <spdlog/spdlog.h>

#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <chrono>
#include <csignal>
#include <memory>
#include <thread>

void backup::helper::readOutput(std::shared_ptr<boost::asio::readable_pipe> pipe, boost::asio::streambuf &buf,
                                std::function<void(std::string)> const &outputHandler) {
  boost::asio::async_read_until(*pipe, buf, '\n',
                                [pipe, &buf, outputHandler](boost::system::error_code const &ec, std::size_t size) {
                                  if (size) {
                                    std::string line;
                                    std::getline(std::istream{&buf}, line);
                                    outputHandler(line);
                                  }
                                  if (!ec) {
                                    readOutput(pipe, buf, outputHandler);
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
  if (purgeFlag_) {
    backupProcess.emplace(*ioContext, executable().string(),
                          std::vector<std::string>{"--config", pathToConfig_.string(), "prune", "create", "--progress",
                                                   "check"},
                          bp::process_stdio{{}, ioPipe, {}});
    spdlog::info("{} --config {} prune create --progress check", executable_.string(), pathToConfig_.string());
  } else {
    backupProcess.emplace(*ioContext, executable().string(),
                          std::vector<std::string>{"--config", pathToConfig_.string(), "create", "--progress", "check"},
                          bp::process_stdio{{}, ioPipe, {}});
    spdlog::info("{} --config {} create --progress check", executable_.string(), pathToConfig_.string());
  }
}
