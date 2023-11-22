#pragma once

#include <map>
#include <dpp/dpp.h>

#include "logger/logger.h"


class ServerDatabase final {
public:
  enum class Channels {
    kNone,
    kUpdates,
    kStreams,
  };

  enum class Roles {
    kNone,
    kModerator,
    kStreaming
  };

  ServerDatabase() = delete;
  ~ServerDatabase() = default;

  ServerDatabase(const std::string& database_file_path);

  dpp::snowflake GetGuildId() const noexcept;
  dpp::snowflake GetRoleId(const Roles role) const noexcept;
  dpp::snowflake GetChannelId(const Channels channel) const noexcept;

private:
  const std::shared_ptr<spdlog::async_logger> logger_ = Logger::Get().Create("Server Database");

  dpp::snowflake guild_id_;
  std::map<Roles, dpp::snowflake> roles_ids_;
  std::map<Channels, dpp::snowflake> channels_ids_;
};