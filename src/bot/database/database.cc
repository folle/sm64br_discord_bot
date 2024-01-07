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

  const auto& bot_data = database_json["bot"];
  bot_token_ = bot_data["token"].get<std::string>();

  const auto& server_data = database_json["server"];
  guild_id_ = server_data["guild"].get<dpp::snowflake>();

  const auto& roles_json = server_data["roles"];
  for (auto it_role = roles_json.begin(); it_role != roles_json.end(); ++it_role) {
    roles_ids_[::ServerRoleStringToEnum(it_role.key())] = it_role.value().get<dpp::snowflake>();
  }

  const auto& channels_json = server_data["channels"];
  for (auto it_channel = channels_json.begin(); it_channel != channels_json.end(); ++it_channel) {
    channels_ids_[::ServerChannelStringToEnum(it_channel.key())] = it_channel.value().get<dpp::snowflake>();
  }

  const auto& google_service_account_data = database_json["google_service_account"];
  google_client_email_ = google_service_account_data["client_email"].get<std::string>();
  google_private_key_id_ = google_service_account_data["private_key_id"].get<std::string>();
  google_private_key_ = google_service_account_data["private_key"].get<std::string>();
}

std::string Database::GetBotToken() const noexcept {
  return bot_token_;
}

dpp::snowflake Database::GetGuildId() const noexcept {
  return guild_id_;
}

dpp::snowflake Database::GetRoleId(const Roles role) const noexcept {
  return roles_ids_.at(role);
}

dpp::snowflake Database::GetChannelId(const Channels channel) const noexcept  {
  return channels_ids_.at(channel);
}

std::string Database::GetGoogleClientEmail() const noexcept {
  return google_client_email_;
}

std::string Database::GetGooglePrivateKeyId() const noexcept {
  return google_private_key_id_;
}

std::string Database::GetGooglePrivateKey() const noexcept {
  return google_private_key_;
}