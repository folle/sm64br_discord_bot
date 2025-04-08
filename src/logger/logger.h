#pragma once

#include <memory>
#include <print>
#include <string>

#include <spdlog/async.h>

class Logger final {
public:
  Logger() = delete;
  ~Logger();

  Logger(std::shared_ptr<spdlog::async_logger>&& logger_) noexcept;

  template <typename... Args>
  void Trace(std::format_string<Args...> fmt, Args&&... args) const noexcept {
    auto const message = std::format(fmt, std::forward<Args>(args)...);
    logger_->trace(message);
  }

  template <typename... Args>
  void Debug(std::format_string<Args...> fmt, Args&&... args) const noexcept {
    auto const message = std::format(fmt, std::forward<Args>(args)...);
    logger_->debug(message);
  }

  template <typename... Args>
  void Info(std::format_string<Args...> fmt, Args&&... args) const noexcept {
    auto const message = std::format(fmt, std::forward<Args>(args)...);
    logger_->info(message);
  }

  template <typename... Args>
  void Warn(std::format_string<Args...> fmt, Args&&... args) const noexcept {
    auto const message = std::format(fmt, std::forward<Args>(args)...);
    logger_->warn(message);
  }

  template <typename... Args>
  void Error(std::format_string<Args...> fmt, Args&&... args) const noexcept {
    auto const message = std::format(fmt, std::forward<Args>(args)...);
    logger_->error(message);
  }

  template <typename... Args>
  void Critical(std::format_string<Args...> fmt, Args&&... args) const noexcept {
    auto const message = std::format(fmt, std::forward<Args>(args)...);
    logger_->critical(message);
  }

private:
  std::shared_ptr<spdlog::async_logger> const logger_;
};