#pragma once

#include <map>
#include <string>

#include <dpp/dpp.h>

class Settings final {
public:
  enum class Channels {
    kNone,
    kGeneral,
    kUpdates,
    kStreams,
    kClips
  };

  enum class Roles {
    kNone,
    kModerator,
    kStreaming,
    kPacepals
  };

  enum class Users {
    kNone,
    kPetalite
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
    long long bpt{};
    double percentage{};
  };

  static Settings& Get() noexcept;

  std::string const& GetBotToken() const noexcept;

  dpp::snowflake GetGuildId() const noexcept;
  dpp::snowflake GetChannelId(Channels const channel) const noexcept;
  dpp::snowflake GetRoleId(Roles const role) const noexcept;
  dpp::snowflake GetUserId(Users const user) const noexcept;
  std::map<std::string, std::string> const& GetAwardsReactionsAndCategories() const noexcept;

  std::string const& GetTheRunEndpoint() const noexcept;
  TheRunThresholds const& GetTheRunThresholds(Categories const category) const noexcept;

private:
  Settings();
  ~Settings() = default;

  Settings(Settings const&) = delete;
  void operator=(Settings const&) = delete;

private:
  std::string bot_token_;

  dpp::snowflake guild_id_;
  std::map<Channels, dpp::snowflake> channels_ids_;
  std::map<Roles, dpp::snowflake> roles_ids_;
  std::map<Users, dpp::snowflake> users_ids_;
  std::map<std::string, std::string> awards_reactions_and_categories_;

  std::string the_run_endpoint_;
  std::map<Categories, TheRunThresholds> the_run_thresholds_;
};