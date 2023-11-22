#pragma once

#include <memory>
#include <spdlog/async.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>


class Logger final {
public:
  static Logger& Get() noexcept;

  std::shared_ptr<spdlog::async_logger> Create(const std::string& name) const noexcept;

private:
  Logger();
  ~Logger();

  Logger(Logger const&) = delete;
  void operator=(Logger const&) = delete;

private:
  const std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> stdout_sink_ = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
  const std::shared_ptr<spdlog::sinks::daily_file_sink_mt> file_sink_ = std::make_shared<spdlog::sinks::daily_file_sink_mt>("logs/log.txt", 0, 0);
  const std::vector<spdlog::sink_ptr> sinks_;
};