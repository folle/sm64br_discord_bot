#pragma once

#include <sentry.h>

#include "settings/settings.h"


class Sentry final {
public:
  static Sentry& Get(std::shared_ptr<Settings> settings) noexcept;

private:
  Sentry() = delete;
  ~Sentry();

  Sentry(std::shared_ptr<Settings> settings) noexcept;

  Sentry(Sentry const&) = delete;
  void operator=(Sentry const&) = delete;
};