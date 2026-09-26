#include "ProcessRunner.h"

#include <boost/asio.hpp>
#include <boost/process.hpp>

backup::helper::ProcessResult backup::helper::runProcess(std::filesystem::path const& executable,
                                                         std::vector<std::string> const& args) {
  namespace asio = boost::asio;
  namespace bp = boost::process;

  asio::io_context ioContext;
  asio::readable_pipe outPipe{ioContext}, errPipe{ioContext};
  bp::process process{ioContext, executable.string(), args, bp::process_stdio{{}, outPipe, errPipe}};

  // Both pipes are drained concurrently, so the child can't block on a full pipe buffer.
  ProcessResult result{};
  auto ignoreEof = [](boost::system::error_code const&, std::size_t) {};
  asio::async_read(outPipe, asio::dynamic_buffer(result.stdOut), ignoreEof);
  asio::async_read(errPipe, asio::dynamic_buffer(result.stdErr), ignoreEof);
  ioContext.run();

  result.exitCode = process.wait();
  return result;
}
