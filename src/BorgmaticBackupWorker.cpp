#include "BorgmaticBackupWorker.h"

#include <spdlog/spdlog.h>

#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <memory>

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

bool BorgmaticBackupWorker::isRunning() { return backupFuture.isRunning(); }

void BorgmaticBackupWorker::cancel() {
  if (isRunning() && backupProcess) {
    auto pid = backupProcess->id();
    kill(pid, SIGINT);
  }
}

void BorgmaticBackupWorker::createChildProcess(boost::asio::readable_pipe &ioPipe) {
  namespace bp = boost::process;
  if (purgeFlag_) {
    backupProcess.emplace(*ioContext, executable().string(),
                          std::vector<std::string>{"--config", pathToConfig_.string(), "prune", "create", "--progress",
                                                   "check"},
                          bp::process_stdio{{}, ioPipe, {}});
    spdlog::info("/usr/bin/borgmatic --config {} prune create --progress check", pathToConfig_.string());
  } else {
    backupProcess.emplace(*ioContext, executable().string(),
                          std::vector<std::string>{"--config", pathToConfig_.string(), "create", "--progress", "check"},
                          bp::process_stdio{{}, ioPipe, {}});
    spdlog::info("/usr/bin/borgmatic --config {} create --progress check", pathToConfig_.string());
  }
}
