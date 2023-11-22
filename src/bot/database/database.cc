#include "database.h"

#include <fstream>
#include <nlohmann/json.hpp>


namespace {
  Database::ServerChannels ServerChannelStringToEnum(const std::string& channel) {
    if (channel == "updates") {
      return Database::ServerChannels::kUpdates;
    }

    if (channel == "streams") {
      return Database::ServerChannels::kStreams;
    }

    return Database::ServerChannels::kNone;
  }

  Database::ServerRoles ServerRoleStringToEnum(const std::string& role) {
    if (role == "moderator") {
      return Database::ServerRoles::kModerator;
    }

    if (role == "streaming") {
      return Database::ServerRoles::kStreaming;
    }

    return Database::ServerRoles::kNone;
  }
}


Database::Database(const std::string& database_file_path) {
  std::ifstream database_file(database_file_path);
  const auto database_json = nlohmann::json::parse(database_file);

  const auto bot_data = database_json["bot"];
  bot_token_ = bot_data["token"];

  const auto server_data = database_json["server"];
  server_guild_id_ = server_data["guild"].template get<dpp::snowflake>();

  const auto& roles_json = server_data["roles"];
  for (auto role = roles_json.begin(); role != roles_json.end(); ++role) {
    server_roles_ids_[::ServerRoleStringToEnum(role.key())] = role.value().template get<dpp::snowflake>();
  }

  const auto& channels_json = server_data["channels"];
  for (auto channel = channels_json.begin(); channel != channels_json.end(); ++channel) {
    server_channels_ids_[::ServerChannelStringToEnum(channel.key())] = channel.value().template get<dpp::snowflake>();
  }
}

std::string Database::GetBotToken() const noexcept {
  return bot_token_;
}

dpp::snowflake Database::GetServerGuildId() const noexcept {
  return server_guild_id_;
}

dpp::snowflake Database::GetServerRoleId(const ServerRoles role) const noexcept {
  return server_roles_ids_.at(role);
}

dpp::snowflake Database::GetServerChannelId(const ServerChannels channel) const noexcept  {
  return server_channels_ids_.at(channel);
}
