#include "settings.h"

#include <fstream>

#include <nlohmann/json.hpp>


namespace {
  Settings::Channels ServerChannelStringToEnum(std::string const& channel) {
    if (channel == "updates") {
      return Settings::Channels::kUpdates;
    }

    if (channel == "streams") {
      return Settings::Channels::kStreams;
    }

    if (channel == "pb_submission") {
      return Settings::Channels::kPbSubmission;
    }

    return Settings::Channels::kNone;
  }

  Settings::Roles ServerRoleStringToEnum(std::string const& role) {
    if (role == "moderator") {
      return Settings::Roles::kModerator;
    }

    if (role == "streaming") {
      return Settings::Roles::kStreaming;
    }

    return Settings::Roles::kNone;
  }
}


Settings::Settings(std::string const& settings_file_path) {
  std::ifstream settings_file(settings_file_path);
  auto const settings_json = nlohmann::json::parse(settings_file);

  auto const& bot_data = settings_json["bot"];
  bot_token_ = bot_data["token"].get<std::string>();

  auto const& server_data = settings_json["server"];
  guild_id_ = server_data["guild"].get<dpp::snowflake>();

  auto const& roles_json = server_data["roles"];
  for (auto it_role = roles_json.begin(); it_role != roles_json.end(); ++it_role) {
    roles_ids_[::ServerRoleStringToEnum(it_role.key())] = it_role.value().get<dpp::snowflake>();
  }

  auto const& channels_json = server_data["channels"];
  for (auto it_channel = channels_json.begin(); it_channel != channels_json.end(); ++it_channel) {
    channels_ids_[::ServerChannelStringToEnum(it_channel.key())] = it_channel.value().get<dpp::snowflake>();
  }

  auto const& the_run_data = settings_json["the_run"];
  the_run_endpoint_ = the_run_data["end_point"].get<std::string>();
}

std::string const& Settings::GetBotToken() const noexcept {
  return bot_token_;
}

dpp::snowflake Settings::GetGuildId() const noexcept {
  return guild_id_;
}

dpp::snowflake Settings::GetRoleId(Roles const role) const noexcept {
  return roles_ids_.at(role);
}

dpp::snowflake Settings::GetChannelId(Channels const channel) const noexcept  {
  return channels_ids_.at(channel);
}

std::string const& Settings::GetTheRunEndpoint() const noexcept {
  return the_run_endpoint_;
}