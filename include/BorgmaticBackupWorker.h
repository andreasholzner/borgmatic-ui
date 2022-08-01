#ifndef BORGMATIC_UI_INCLUDE_BORGMATICBACKUPWORKER_H_
#define BORGMATIC_UI_INCLUDE_BORGMATICBACKUPWORKER_H_

#include <spdlog/spdlog.h>

#include <QObject>
#include <QtConcurrent/QtConcurrent>
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <concepts>
#include <filesystem>
#include <memory>

namespace backup::helper {
void readOutput(std::shared_ptr<boost::process::async_pipe> pipe, boost::asio::streambuf &buf,
                std::function<void(std::string)> const &outputHandler);
}  // namespace backup::helper

class BorgmaticBackupWorker {
 public:
  void configure(std::filesystem::path const &pathToConfig, bool purgeFlag);
  void start(
      std::invocable auto onFinished, std::invocable<std::string> auto outputHandler = [](std::string const &) {}) {
    backupWatcher.disconnect();
    QObject::connect(&backupWatcher, &QFutureWatcher<void>::finished, onFinished);

    auto ioService = std::make_shared<boost::asio::io_service>();
    auto ioPipe = std::make_shared<boost::process::async_pipe>(*ioService);
    createChildProcess(ioPipe);
    backup::helper::readOutput(ioPipe, buffer, outputHandler);

    backupFuture = QtConcurrent::run([ioService] { ioService->run(); });
    backupWatcher.setFuture(backupFuture);
  }
  bool isRunning();
  void cancel();
  std::filesystem::path executable() const { return "/usr/bin/borgmatic"; };

 private:
  void createChildProcess(std::shared_ptr<boost::process::async_pipe> ioPipe);

  boost::asio::streambuf buffer;
  boost::process::child backupProcess;
  QFuture<void> backupFuture;
  QFutureWatcher<void> backupWatcher;
  std::filesystem::path pathToConfig_;
  bool purgeFlag_;
};

#endif  // BORGMATIC_UI_INCLUDE_BORGMATICBACKUPWORKER_H_
