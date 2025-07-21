#include "logger_factory.h"

#include <ranges>

#include <spdlog/async.h>

LoggerFactory& LoggerFactory::Get() noexcept {
  static LoggerFactory LoggerFactory;
  return LoggerFactory;
}

LoggerFactory::LoggerFactory() noexcept
  : sinks_{stdout_sink_, file_sink_} {
  spdlog::init_thread_pool(8192, 2);
}

LoggerFactory::~LoggerFactory() {
  std::ranges::for_each(sinks_, [](auto const& sink) { sink->flush(); });
}

Logger LoggerFactory::Create(std::string const& name) const noexcept {
  return Logger(std::make_shared<spdlog::async_logger>(name, sinks_.begin(), sinks_.end(), spdlog::thread_pool()));
}