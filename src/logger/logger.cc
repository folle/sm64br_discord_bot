#include "logger.h"


Logger& Logger::Get() noexcept {
  static Logger logger;
  return logger;
}

Logger::Logger()
  : sinks_{stdout_sink_, file_sink_} {
  spdlog::init_thread_pool(8192, 2);

}

Logger::~Logger() {
  for (auto const& sink : sinks_) {
    sink->flush();
  }
}

std::shared_ptr<spdlog::async_logger> Logger::Create(std::string const& name) const noexcept {
  return std::make_shared<spdlog::async_logger>(name, sinks_.begin(), sinks_.end(), spdlog::thread_pool());
}