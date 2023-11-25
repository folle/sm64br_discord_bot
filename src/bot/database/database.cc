#include "database.h"

#include <fstream>
#include <nlohmann/json.hpp>


namespace {
  Database::Channels ServerChannelStringToEnum(const std::string& channel) {
    if (channel == "updates") {
      return Database::Channels::kUpdates;
    }

    if (channel == "streams") {
      return Database::Channels::kStreams;
    }

    if (channel == "pb_submission") {
      return Database::Channels::kPbSubmission;
    }

    return Database::Channels::kNone;
  }

  Database::Roles ServerRoleStringToEnum(const std::string& role) {
    if (role == "moderator") {
      return Database::Roles::kModerator;
    }

    if (role == "streaming") {
      return Database::Roles::kStreaming;
    }

    return Database::Roles::kNone;
  }
}


Database::Database(const std::string& database_file_path) {
  std::ifstream database_file(database_file_path);
  const auto database_json = nlohmann::json::parse(database_file);

  const auto bot_data = database_json["bot"];
  bot_token_ = bot_data["token"];

  const auto server_data = database_json["server"];
  guild_id_ = server_data["guild"].template get<dpp::snowflake>();

  const auto& roles_json = server_data["roles"];
  for (auto role = roles_json.begin(); role != roles_json.end(); ++role) {
    server_guild_id_[::ServerRoleStringToEnum(role.key())] = role.value().template get<dpp::snowflake>();
  }

  const auto& channels_json = server_data["channels"];
  for (auto channel = channels_json.begin(); channel != channels_json.end(); ++channel) {
    channels_ids_[::ServerChannelStringToEnum(channel.key())] = channel.value().template get<dpp::snowflake>();
  }
}

std::string Database::GetBotToken() const noexcept {
  return bot_token_;
}

dpp::snowflake Database::GetGuildId() const noexcept {
  return guild_id_;
}

dpp::snowflake Database::GetRoleId(const Roles role) const noexcept {
  return server_guild_id_.at(role);
}

dpp::snowflake Database::GetChannelId(const Channels channel) const noexcept  {
  return channels_ids_.at(channel);
}
