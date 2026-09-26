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

namespace backup::helper {
void readOutput(std::shared_ptr<boost::asio::readable_pipe> pipe, boost::asio::streambuf &buf,
                std::function<void(std::string)> const &outputHandler);
}  // namespace backup::helper

class BorgmaticBackupWorker {
 public:
  void configure(std::filesystem::path const &pathToConfig, bool purgeFlag);
  void start(
      std::invocable auto onFinished, std::invocable<std::string> auto outputHandler = [](std::string const &) {}) {
    backupWatcher.disconnect();
    QObject::connect(&backupWatcher, &QFutureWatcher<void>::finished, onFinished);

    // The previous process is bound to the previous io_context, so it has to go first.
    backupProcess.reset();
    ioContext = std::make_shared<boost::asio::io_context>();
    auto ioPipe = std::make_shared<boost::asio::readable_pipe>(*ioContext);
    createChildProcess(*ioPipe);
    backup::helper::readOutput(ioPipe, buffer, outputHandler);

    backupFuture = QtConcurrent::run([ioContext = ioContext] { ioContext->run(); });
    backupWatcher.setFuture(backupFuture);
  }
  bool isRunning();
  void cancel();
  std::filesystem::path executable() const { return "/usr/bin/borgmatic"; };

 private:
  void createChildProcess(boost::asio::readable_pipe &ioPipe);

  boost::asio::streambuf buffer;
  // Declared before backupProcess, so the process is destroyed before its io_context.
  std::shared_ptr<boost::asio::io_context> ioContext;
  std::optional<boost::process::process> backupProcess;
  QFuture<void> backupFuture;
  QFutureWatcher<void> backupWatcher;
  std::filesystem::path pathToConfig_;
  bool purgeFlag_;
};

#endif  // BORGMATIC_UI_INCLUDE_BORGMATICBACKUPWORKER_H_
