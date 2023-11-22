#include "server_database.h"

#include <fstream>
#include <nlohmann/json.hpp>

#include "utils/file_utils.h"


namespace {
  ServerDatabase::Channels ChannelStringToEnum(const std::string& channel) {
    if (channel == "updates") {
      return ServerDatabase::Channels::kUpdates;
    }

    if (channel == "streams") {
      return ServerDatabase::Channels::kStreams;
    }

    return ServerDatabase::Channels::kNone;
  }

  ServerDatabase::Roles RoleStringToEnum(const std::string& role) {
    if (role == "moderator") {
      return ServerDatabase::Roles::kModerator;
    }

    if (role == "streaming") {
      return ServerDatabase::Roles::kStreaming;
    }

    return ServerDatabase::Roles::kNone;
  }
}


ServerDatabase::ServerDatabase(const std::string& database_file_path) {
  std::ifstream database_file(database_file_path);
  nlohmann::json database_json = nlohmann::json::parse(database_file);

  guild_id_ = database_json["guild"].template get<dpp::snowflake>();

  const auto& roles_json = database_json["roles"];
  for (auto role = roles_json.begin(); role != roles_json.end(); ++role) {
    roles_ids_[::RoleStringToEnum(role.key())] = role.value().template get<dpp::snowflake>();
  }

  const auto& channels_json = database_json["channels"];
  for (auto channel = channels_json.begin(); channel != channels_json.end(); ++channel) {
    channels_ids_[::ChannelStringToEnum(channel.key())] = channel.value().template get<dpp::snowflake>();
  }
}

dpp::snowflake ServerDatabase::GetGuildId() const noexcept {
  return guild_id_;
}

dpp::snowflake ServerDatabase::GetRoleId(const Roles role) const noexcept {
  return roles_ids_.at(role);
}

dpp::snowflake ServerDatabase::GetChannelId(const Channels channel) const noexcept  {
  return channels_ids_.at(channel);
}
