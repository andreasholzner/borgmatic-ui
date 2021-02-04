#ifndef BORGMATIC_UI_INCLUDE_BACKUPWORKER_H_
#define BORGMATIC_UI_INCLUDE_BACKUPWORKER_H_

#include <spdlog/spdlog.h>

#include <QObject>
#include <QtConcurrent/QtConcurrent>
#include <boost/asio.hpp>
#include <boost/process.hpp>

class BackupWorker {
 public:
  BackupWorker() : ioService(), ioPipe(ioService) {}

  void start(
      std::function<void()> onFinished,
      std::function<void(std::string)> const &outputHandler = [](std::string const &) {});
  bool isRunning();
  void cancel();

 private:
  boost::asio::io_service ioService;
  boost::asio::streambuf buffer;
  boost::process::async_pipe ioPipe;
  boost::process::child backupProcess;
  QFuture<void> backupFuture;
  QFutureWatcher<void> backupWatcher;
};

#endif  // BORGMATIC_UI_INCLUDE_BACKUPWORKER_H_
