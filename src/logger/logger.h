#pragma once

#include <memory>
#include <string>

#include <fmt/format.h>
#include <spdlog/async.h>

class Logger final {
public:
  Logger() = delete;
  ~Logger();

  Logger(std::shared_ptr<spdlog::async_logger>&& logger_) noexcept;

  template <typename... Args>
  void Trace(fmt::format_string<Args...> fmt, Args&&... args) const noexcept {
    auto const message = fmt::format(fmt, std::forward<Args>(args)...);
    logger_->trace(message);
  }

  template <typename... Args>
  void Debug(fmt::format_string<Args...> fmt, Args&&... args) const noexcept {
    auto const message = fmt::format(fmt, std::forward<Args>(args)...);
    logger_->debug(message);
  }

  template <typename... Args>
  void Info(fmt::format_string<Args...> fmt, Args&&... args) const noexcept {
    auto const message = fmt::format(fmt, std::forward<Args>(args)...);
    logger_->info(message);
  }

  template <typename... Args>
  void Warn(fmt::format_string<Args...> fmt, Args&&... args) const noexcept {
    auto const message = fmt::format(fmt, std::forward<Args>(args)...);
    logger_->warn(message);
  }

  template <typename... Args>
  void Error(fmt::format_string<Args...> fmt, Args&&... args) const noexcept {
    auto const message = fmt::format(fmt, std::forward<Args>(args)...);
    logger_->error(message);
  }

  template <typename... Args>
  void Critical(fmt::format_string<Args...> fmt, Args&&... args) const noexcept {
    auto const message = fmt::format(fmt, std::forward<Args>(args)...);
    logger_->critical(message);
  }

private:
  std::shared_ptr<spdlog::async_logger> const logger_;
};