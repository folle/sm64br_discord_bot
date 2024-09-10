#include "sentry.h"

#include <sentry.h>

#include "settings/settings.h"

Sentry& Sentry::Get() noexcept {
  static Sentry sentry;
  return sentry;
}

Sentry::Sentry() noexcept {
  sentry_options_t *options = sentry_options_new();
  sentry_options_set_dsn(options, Settings::Get().GetSentryDsn().c_str());
  sentry_options_set_database_path(options, ".sentry-native");
  sentry_options_set_release(options, "sm64br-discord-bot@1.0");
  sentry_init(options);
}

Sentry::~Sentry() {
  sentry_close();
}

void Sentry::CaptureEventInfo(std::string const& name, std::string const& message) noexcept {
  std::scoped_lock<std::mutex> const mutex_lock(capture_event_mutex_);
  sentry_capture_event(sentry_value_new_message_event(SENTRY_LEVEL_INFO, name.c_str(), message.c_str()));
}

void Sentry::CaptureEventWarn(std::string const& name, std::string const& message) noexcept {
  std::scoped_lock<std::mutex> const mutex_lock(capture_event_mutex_);
  sentry_capture_event(sentry_value_new_message_event(SENTRY_LEVEL_WARNING, name.c_str(), message.c_str()));
}

void Sentry::CaptureEventError(std::string const& name, std::string const& message) noexcept {
  std::scoped_lock<std::mutex> const mutex_lock(capture_event_mutex_);
  sentry_capture_event(sentry_value_new_message_event(SENTRY_LEVEL_ERROR, name.c_str(), message.c_str()));
}