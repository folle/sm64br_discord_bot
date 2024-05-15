#include "database.h"

#include <fstream>

#include <nlohmann/json.hpp>


namespace {
  Database::Channels ServerChannelStringToEnum(std::string const& channel) {
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

  Database::Roles ServerRoleStringToEnum(std::string const& role) {
    if (role == "moderator") {
      return Database::Roles::kModerator;
    }

    if (role == "streaming") {
      return Database::Roles::kStreaming;
    }

    return Database::Roles::kNone;
  }
}


Database::Database(std::string const& database_file_path) {
  std::ifstream database_file(database_file_path);
  auto const database_json = nlohmann::json::parse(database_file);

  auto const& bot_data = database_json["bot"];
  bot_token_ = bot_data["token"].get<std::string>();

  auto const& server_data = database_json["server"];
  guild_id_ = server_data["guild"].get<dpp::snowflake>();

  auto const& roles_json = server_data["roles"];
  for (auto it_role = roles_json.begin(); it_role != roles_json.end(); ++it_role) {
    roles_ids_[::ServerRoleStringToEnum(it_role.key())] = it_role.value().get<dpp::snowflake>();
  }

  auto const& channels_json = server_data["channels"];
  for (auto it_channel = channels_json.begin(); it_channel != channels_json.end(); ++it_channel) {
    channels_ids_[::ServerChannelStringToEnum(it_channel.key())] = it_channel.value().get<dpp::snowflake>();
  }
}

std::string Database::GetBotToken() const noexcept {
  return bot_token_;
}

dpp::snowflake Database::GetGuildId() const noexcept {
  return guild_id_;
}

dpp::snowflake Database::GetRoleId(Roles const role) const noexcept {
  return roles_ids_.at(role);
}

dpp::snowflake Database::GetChannelId(Channels const channel) const noexcept  {
  return channels_ids_.at(channel);
}