#pragma once

#include <map>

#include <dpp/dpp.h>

#include "logger/logger.h"


class Settings final {
public:
  enum class Channels {
    kNone,
    kGeneral,
    kUpdates,
    kStreams,
    kPbSubmission
  };

  enum class Roles {
    kNone,
    kModerator,
    kStreaming,
    kPacepals
  };

  enum class Categories {
    kNone,
    k0Star,
    k1Star,
    k16Star,
    k70Star,
    k120Star
  };

  struct TheRunThresholds {
    double percentage{};
    long long bpt{};
  };

  Settings() = delete;
  ~Settings() = default;

  Settings(std::string const& settings_file_path);

  std::string const& GetBotToken() const noexcept;

  dpp::snowflake GetGuildId() const noexcept;
  dpp::snowflake GetRoleId(Roles const role) const noexcept;
  dpp::snowflake GetChannelId(Channels const channel) const noexcept;

  std::string const& GetTheRunEndpoint() const noexcept;
  TheRunThresholds const& GetTheRunThresholds(Categories const category) const noexcept;

private:
  std::shared_ptr<spdlog::async_logger> const logger_ = Logger::Get().Create("Settings");

  std::string bot_token_;

  dpp::snowflake guild_id_;
  std::map<Roles, dpp::snowflake> roles_ids_;
  std::map<Channels, dpp::snowflake> channels_ids_;

  std::string the_run_endpoint_;
  std::map<Categories, TheRunThresholds> the_run_thresholds_;
};