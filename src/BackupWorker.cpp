#include "BackupWorker.h"

#include <spdlog/spdlog.h>

#include <QObject>
#include <QtConcurrent/QtConcurrent>
#include <boost/asio.hpp>
#include <boost/process.hpp>

void readOutput(boost::process::async_pipe &pipe, boost::asio::streambuf &buf,
                std::function<void(std::string)> outputHandler) {
  boost::asio::async_read_until(
      pipe, buf, '\n', [&pipe, &buf, outputHandler](boost::system::error_code const &ec, std::size_t size) {
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
  backupProcess = boost::process::child("/usr/bin/tail", "-f", "/var/log/Xorg.0.log", boost::process::std_out > ioPipe);
  readOutput(ioPipe, buffer, outputHandler);
  backupFuture = QtConcurrent::run([this] {
    spdlog::debug("Starting async work");
    ioService.run();
    ioPipe.close();
    ioService.reset();
    spdlog::debug("Async work done");
  });
  backupWatcher.setFuture(backupFuture);
}

bool BackupWorker::isRunning() { return backupFuture.isRunning(); }

void BackupWorker::cancel() {
  if (isRunning()) {
    auto pid = backupProcess.id();
    kill(pid, SIGINT);
  }
}
