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
#include <optional>

#include "ProcessRunner.h"

namespace backup::helper {
void readOutput(std::shared_ptr<boost::asio::readable_pipe> pipe, boost::asio::streambuf &buf,
                std::function<void(std::string)> const &outputHandler);
}  // namespace backup::helper

class BorgmaticBackupWorker {
 public:
  explicit BorgmaticBackupWorker(std::filesystem::path executable = backup::helper::borgmaticExecutable())
      : executable_(std::move(executable)) {}
  ~BorgmaticBackupWorker();
  BorgmaticBackupWorker(BorgmaticBackupWorker const &) = delete;
  BorgmaticBackupWorker &operator=(BorgmaticBackupWorker const &) = delete;

  void configure(std::filesystem::path const &pathToConfig, bool purgeFlag);
  // onFinished receives borgmatic's exit code, or -1 if borgmatic couldn't be started.
  void start(
      std::invocable<int> auto onFinished,
      std::invocable<std::string> auto outputHandler = [](std::string const &) {}) {
    backupWatcher.disconnect();
    QObject::connect(&backupWatcher, &QFutureWatcher<int>::finished,
                     [this, onFinished] { onFinished(backupFuture.result()); });

    // The previous process is bound to the previous io_context, so it has to go first.
    backupProcess.reset();
    ioContext = std::make_shared<boost::asio::io_context>();
    // Output left over from a previous, interrupted run doesn't belong to this one.
    buffer.consume(buffer.size());
    auto ioPipe = std::make_shared<boost::asio::readable_pipe>(*ioContext);
    try {
      createChildProcess(*ioPipe);
    } catch (std::exception const &e) {
      spdlog::error("Error calling borgmatic executable {}, reason: {}", executable_.string(), e.what());
      backupFuture = QtFuture::makeReadyValueFuture(-1);
      backupWatcher.setFuture(backupFuture);
      return;
    }
    auto exitCode = std::make_shared<int>(-1);
    backupProcess->async_wait([exitCode](boost::system::error_code const &ec, int code) {
      if (!ec) {
        *exitCode = code;
      }
    });
    backup::helper::readOutput(ioPipe, buffer, outputHandler);

    backupFuture = QtConcurrent::run([ioContext = ioContext, exitCode] {
      ioContext->run();
      return *exitCode;
    });
    backupWatcher.setFuture(backupFuture);
  }
  bool isRunning();
  void cancel();
  // Interrupts borgmatic and blocks until it has exited (killing it after a grace period). onFinished is not called.
  void cancelAndWait();
  std::filesystem::path executable() const { return executable_; };

 private:
  void createChildProcess(boost::asio::readable_pipe &ioPipe);

  std::filesystem::path executable_;

  boost::asio::streambuf buffer;
  // Declared before backupProcess, so the process is destroyed before its io_context.
  std::shared_ptr<boost::asio::io_context> ioContext;
  std::optional<boost::process::process> backupProcess;
  QFuture<int> backupFuture;
  QFutureWatcher<int> backupWatcher;
  std::filesystem::path pathToConfig_;
  bool purgeFlag_ = false;
};

#endif  // BORGMATIC_UI_INCLUDE_BORGMATICBACKUPWORKER_H_
