#include "BorgmaticBackupWorker.h"

#include <spdlog/spdlog.h>

#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <memory>

void backup::helper::readOutput(std::shared_ptr<boost::process::async_pipe> pipe, boost::asio::streambuf &buf,
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
  if (isRunning()) {
    auto pid = backupProcess.id();
    kill(pid, SIGINT);
  }
}

void BorgmaticBackupWorker::createChildProcess(std::shared_ptr<boost::process::async_pipe> ioPipe) {
  if (purgeFlag_) {
    backupProcess = boost::process::child("/usr/bin/borgmatic", "--config", pathToConfig_.string(), "prune", "create",
                                          "--progress", "check", boost::process::std_out > *ioPipe);
    spdlog::info("/usr/bin/borgmatic --config {} prune create --progress check", pathToConfig_.string());
  } else {
    backupProcess = boost::process::child("/usr/bin/borgmatic", "--config", pathToConfig_.string(), "create",
                                          "--progress", "check", boost::process::std_out > *ioPipe);
    spdlog::info("/usr/bin/borgmatic --config {} create --progress check", pathToConfig_.string());
  }
}
