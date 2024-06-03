#include "sentry.h"


Sentry& Sentry::Get(std::shared_ptr<Settings> settings) noexcept {
  static Sentry sentry(settings);
  return sentry;
}

Sentry::Sentry(std::shared_ptr<Settings> settings) noexcept {
  sentry_options_t *options = sentry_options_new();
  sentry_options_set_dsn(options, settings->GetSentryDsn().c_str());
  sentry_options_set_database_path(options, ".sentry-native");
  sentry_options_set_release(options, "sm64br-discord-bot@1.0");
  sentry_init(options);
}

Sentry::~Sentry() {
  sentry_close();
}

// {
//   sentry_capture_event(sentry_value_new_message_event(
//   /*   level */ SENTRY_LEVEL_INFO,
//   /*  logger */ "custom",
//   /* message */ "It works!"
// ));
// }