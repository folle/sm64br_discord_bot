#pragma once

#include <map>
#include <dpp/dpp.h>

#include "logger/logger.h"


class Database final {
public:
  enum class ServerChannels {
    kNone,
    kUpdates,
    kStreams,
  };

  enum class ServerRoles {
    kNone,
    kModerator,
    kStreaming
  };

  Database() = delete;
  ~Database() = default;

  Database(const std::string& database_file_path);

  std::string GetBotToken() const noexcept;

  dpp::snowflake GetServerGuildId() const noexcept;
  dpp::snowflake GetServerRoleId(const ServerRoles role) const noexcept;
  dpp::snowflake GetServerChannelId(const ServerChannels channel) const noexcept;

private:
  const std::shared_ptr<spdlog::async_logger> logger_ = Logger::Get().Create("Database");

  std::string bot_token_;

  dpp::snowflake server_guild_id_;
  std::map<ServerRoles, dpp::snowflake> server_roles_ids_;
  std::map<ServerChannels, dpp::snowflake> server_channels_ids_;
};