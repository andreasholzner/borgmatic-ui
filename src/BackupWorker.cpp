#include "BackupWorker.h"

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

bool BackupWorker::isRunning() { return backupFuture.isRunning(); }

void BackupWorker::cancel() {
  if (isRunning()) {
    auto pid = backupProcess.id();
    kill(pid, SIGINT);
  }
}
