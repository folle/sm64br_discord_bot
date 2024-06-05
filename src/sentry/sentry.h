#pragma once

#include <memory>
#include <mutex>
#include <string>

class Sentry final {
public:
  static Sentry& Get() noexcept;

  void CaptureEventInfo(std::string const& name, std::string const& message) noexcept;
  void CaptureEventWarn(std::string const& name, std::string const& message) noexcept;
  void CaptureEventError(std::string const& name, std::string const& message) noexcept;

private:
  Sentry() noexcept;
  ~Sentry();

  Sentry(Sentry const&) = delete;
  void operator=(Sentry const&) = delete;

private:
  std::mutex capture_event_mutex_;
};