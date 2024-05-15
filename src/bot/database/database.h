#pragma once

#include <map>

#include <dpp/dpp.h>

#include "logger/logger.h"


class Database final {
public:
  enum class Channels {
    kNone,
    kUpdates,
    kStreams,
    kPbSubmission
  };

  enum class Roles {
    kNone,
    kModerator,
    kStreaming
  };

  Database() = delete;
  ~Database() = default;

  Database(std::string const&  database_file_path);

  std::string GetBotToken() const noexcept;

  dpp::snowflake GetGuildId() const noexcept;
  dpp::snowflake GetRoleId(Roles const role) const noexcept;
  dpp::snowflake GetChannelId(Channels const channel) const noexcept;

private:
  std::shared_ptr<spdlog::async_logger> const logger_ = Logger::Get().Create("Database");

  std::string bot_token_;

  dpp::snowflake guild_id_;
  std::map<Roles, dpp::snowflake> roles_ids_;
  std::map<Channels, dpp::snowflake> channels_ids_;
};