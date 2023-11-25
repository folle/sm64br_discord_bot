#include "sm64br_discord_bot.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <regex>
#include <sstream>
#include <fmt/format.h>


namespace {
  template<class Container> void ClearFinishedFutures(Container& futures) {
    auto finished_futures_begin_ = std::remove_if(futures.begin(), futures.end(), [](const auto& future) {
      return (std::future_status::ready == future.wait_for(std::chrono::milliseconds(0)));
     });
    futures.erase(finished_futures_begin_, futures.end());
  }
}


Sm64brDiscordBot::Sm64brDiscordBot() {
  logger_->info("Initializing bot");

  bot_.on_log(std::bind(&Sm64brDiscordBot::OnLog, this, std::placeholders::_1));
  bot_.on_ready(std::bind(&Sm64brDiscordBot::OnReady, this, std::placeholders::_1));
  //bot_.on_message_create(std::bind(&Sm64brDiscordBot::OnMessageCreate, this, std::placeholders::_1));
  bot_.on_presence_update(std::bind(&Sm64brDiscordBot::OnPresenceUpdate, this, std::placeholders::_1));
  bot_.on_guild_member_add(std::bind(&Sm64brDiscordBot::OnGuildMemberAdd, this, std::placeholders::_1));
  bot_.on_guild_member_remove(std::bind(&Sm64brDiscordBot::OnGuildMemberRemove, this, std::placeholders::_1));

  ClearStreamingStatus();
}

Sm64brDiscordBot::~Sm64brDiscordBot() {
  logger_->info("Bot terminated");
  logger_->flush();
}

void Sm64brDiscordBot::Start() noexcept {
  logger_->info("Starting bot event handler loop");
  bot_.start(dpp::st_wait);
}

void Sm64brDiscordBot::OnLog(const dpp::log_t& event) const noexcept {
  switch (event.severity) {
    case dpp::ll_trace:
      logger_->trace("{}", event.message);
      break;
    case dpp::ll_debug:
      logger_->debug("{}", event.message);
      break;
    case dpp::ll_info:
      logger_->info("{}", event.message);
      break;
    case dpp::ll_warning:
      logger_->warn("{}", event.message);
      break;
    case dpp::ll_error:
      logger_->error("{}", event.message);
      break;
    case dpp::ll_critical:
    default:
      logger_->critical("{}", event.message);
    break;
  }
}

void Sm64brDiscordBot::OnMessageCreate(const dpp::message_create_t& message_create) noexcept {
  ::ClearFinishedFutures(message_create_futures_);

  message_create_futures_.push_back(std::async(std::launch::async, [this, message_create]() {
    const auto is_bot = message_create.msg.author.is_bot();
    const auto& message = message_create.msg;

    bool is_moderator = false;
    try {
      const auto message_guild_member = bot_.guild_get_member_sync(message.guild_id, message.author.id);
      is_moderator = std::any_of(message_guild_member.get_roles().begin(),
                                 message_guild_member.get_roles().end(),
                                 [this](const auto& role) { return (role == database_.GetServerRoleId(Database::ServerRoles::kModerator)); });
    }
    catch (const dpp::rest_exception& rest_exception) {
      //TODO
    }


    // Announcement message
    const auto is_announcement_message = message.content.rfind("!a ", 0);

    // General Message
    const auto is_general_message = message.content.rfind("!m ", 0);

    message_handler_.Process(message_create);
  }));
}

void Sm64brDiscordBot::OnPresenceUpdate(const dpp::presence_update_t& presence_update) noexcept {
  ::ClearFinishedFutures(presence_update_futures_);

  presence_update_futures_.push_back(std::async(std::launch::async, [this, presence_update]() {
    dpp::user* streaming_user = {};
    try {
      streaming_user = bot_.guild_get_member_sync(database_.GetServerGuildId(), presence_update.rich_presence.user_id).get_user();
    } catch (const dpp::rest_exception& rest_exception) {
      logger_->error("Failed to get member while processing presence update. Exception: '{}'", rest_exception.what());
      return;
    }

    if (!streaming_user) {
      logger_->error("Streaming user wasn't found in the cache while processing presence update");
      return;
    }

    const auto& streaming_user_name = streaming_user->format_username();
    const auto& activies = presence_update.rich_presence.activities;
    const auto streaming_activity = std::find_if(activies.cbegin(), activies.cend(), [](const auto& activity) {
      return (activity.type == dpp::activity_type::at_streaming) && (activity.name == "Twitch") && (activity.state == "Super Mario 64");
    });
    const auto is_streaming_sm64 = activies.cend() != streaming_activity;

    const auto& streaming_user_id = presence_update.rich_presence.user_id;

    const std::lock_guard<std::mutex> lock(on_presence_update_mutex_);

    const auto it_user_id_and_message_id = streaming_users_ids_and_messages_ids_.find(streaming_user_id);
    const auto streaming_message_sent_ = (streaming_users_ids_and_messages_ids_.cend() != it_user_id_and_message_id);
    if (is_streaming_sm64) {
      if (!streaming_message_sent_) {
        logger_->info("User '{}' started streaming Super Mario 64", streaming_user_name);

        const auto streaming_message = dpp::message(database_.GetServerChannelId(Database::ServerChannels::kStreams),
          fmt::format("**{}** está ao vivo jogando Super Mario 64! Assista em: **{}**", streaming_user_name, streaming_activity->url));
        dpp::snowflake streaming_message_id = {};
        try {
          streaming_message_id = bot_.message_create_sync(streaming_message).id;
        }
        catch (const dpp::rest_exception& rest_exception) {
          logger_->error("Failed to create streaming message while processing presence update. Exception: '{}'", rest_exception.what());
          return;
        }

        bot_.guild_member_add_role(database_.GetServerGuildId(), streaming_user_id, database_.GetServerRoleId(Database::ServerRoles::kStreaming));

        streaming_users_ids_and_messages_ids_[streaming_user_id] = streaming_message_id;
      }
    } else {
      if (streaming_message_sent_) {
        logger_->info("User '{}' finished streaming Super Mario 64", streaming_user_name);

        bot_.message_delete(it_user_id_and_message_id->second, database_.GetServerChannelId(Database::ServerChannels::kStreams));
        bot_.guild_member_remove_role(database_.GetServerGuildId(), streaming_user_id, database_.GetServerRoleId(Database::ServerRoles::kStreaming));

        streaming_users_ids_and_messages_ids_.erase(it_user_id_and_message_id);
      }
    }
  }));
}

void Sm64brDiscordBot::OnGuildMemberAdd(const dpp::guild_member_add_t& guild_member_add) noexcept {
  const auto join_message = dpp::message(database_.GetServerChannelId(Database::ServerChannels::kUpdates),
                                         fmt::format("**@{}** acabou de entrar no servidor.", guild_member_add.added.get_user()->format_username()));
  bot_.message_create(join_message);
}

void Sm64brDiscordBot::OnGuildMemberRemove(const dpp::guild_member_remove_t& guild_member_remove) noexcept {
  const auto leave_message = dpp::message(database_.GetServerChannelId(Database::ServerChannels::kUpdates),
                                          fmt::format("**@{}** acabou de sair no servidor.", guild_member_remove.removed.format_username()));
  bot_.message_create(leave_message);
}

void Sm64brDiscordBot::OnReady(const dpp::ready_t& ready) {
  logger_->info("Bot event handler loop is running and ready to start processing requests");
}

void Sm64brDiscordBot::ClearStreamingStatus() noexcept {
  // Clear streaming roles
  dpp::snowflake highest_member_id = {};
  dpp::guild_member_map members;
  do {
    try {
      constexpr uint16_t kMaxMembersPerCall = 1000;
      members = bot_.guild_get_members_sync(database_.GetServerGuildId(), kMaxMembersPerCall, highest_member_id);
    } catch (const dpp::rest_exception& rest_exception) {
      logger_->error("Failed to get members while clearing streaming status. Exception: '{}'", rest_exception.what());
      return;
    }

    for (auto& member : members) {
      if (highest_member_id < member.first) {
        highest_member_id = member.first;
      }

      const auto& roles = member.second.get_roles();
      const auto it_streaming_role =  std::find(roles.cbegin(), roles.cend(), database_.GetServerRoleId(Database::ServerRoles::kStreaming));
      if (it_streaming_role != roles.cend()) {
        bot_.guild_member_remove_role(database_.GetServerGuildId(), member.second.user_id, database_.GetServerRoleId(Database::ServerRoles::kStreaming));
      }
    }
  } while (!members.empty());

  // Clear streaming messages
  auto earliest_streaming_message_id = std::numeric_limits<dpp::snowflake>::max();
  dpp::message_map streaming_messages;
  do {
    try {
      constexpr uint64_t kMaxMessagesPerCall = 100;
      streaming_messages = bot_.messages_get_sync(database_.GetServerChannelId(Database::ServerChannels::kStreams), {}, {}, {}, kMaxMessagesPerCall);
    } catch (const dpp::rest_exception& rest_exception) {
      logger_->error("Failed to get streaming messages while clearing streaming status. Exception: '{}'", rest_exception.what());
      return;
    }

    for (const auto& streaming_message : streaming_messages) {
      if (!earliest_streaming_message_id || (streaming_message.first < earliest_streaming_message_id)) {
        earliest_streaming_message_id = streaming_message.first;
      }

      bot_.message_delete(streaming_message.first, database_.GetServerChannelId(Database::ServerChannels::kStreams));
    }
  } while (!streaming_messages.empty());
}
