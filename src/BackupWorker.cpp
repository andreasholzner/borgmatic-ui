#include "BackupWorker.h"

#include <spdlog/spdlog.h>

#include <QObject>
#include <QtConcurrent/QtConcurrent>
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <memory>

void readOutput(std::shared_ptr<boost::process::async_pipe> pipe, boost::asio::streambuf &buf,
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

void BackupWorker::start(std::function<void()> onFinished, std::function<void(std::string)> const &outputHandler) {
  backupWatcher.disconnect();
  QObject::connect(&backupWatcher, &QFutureWatcher<void>::finished, onFinished);
  auto ioService = std::make_shared<boost::asio::io_service>();
  auto ioPipe = std::make_shared<boost::process::async_pipe>(*ioService);
  backupProcess =
      boost::process::child("/usr/bin/tail", "-f", "/var/log/Xorg.0.log", boost::process::std_out > *ioPipe);
  readOutput(ioPipe, buffer, outputHandler);
  backupFuture = QtConcurrent::run([ioService] { ioService->run(); });
  backupWatcher.setFuture(backupFuture);
}

bool BackupWorker::isRunning() { return backupFuture.isRunning(); }

void BackupWorker::cancel() {
  if (isRunning()) {
    auto pid = backupProcess.id();
    kill(pid, SIGINT);
  }
}
