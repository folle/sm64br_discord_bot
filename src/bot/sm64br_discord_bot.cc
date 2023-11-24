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
  bot_.on_log(std::bind(&Sm64brDiscordBot::OnLog, this, std::placeholders::_1));
  bot_.on_ready(std::bind(&Sm64brDiscordBot::OnReady, this, std::placeholders::_1));
  //bot_.on_message_create(std::bind(&Sm64brDiscordBot::OnMessageCreate, this, std::placeholders::_1));
  bot_.on_presence_update(std::bind(&Sm64brDiscordBot::OnPresenceUpdate, this, std::placeholders::_1));
  bot_.on_guild_member_add(std::bind(&Sm64brDiscordBot::OnGuildMemberAdd, this, std::placeholders::_1));
  bot_.on_guild_member_remove(std::bind(&Sm64brDiscordBot::OnGuildMemberRemove, this, std::placeholders::_1));

  ClearStreamingStatus();
}

Sm64brDiscordBot::~Sm64brDiscordBot() {
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
    std::string streaming_user_name;
    try {
      auto const streaming_user = bot_.guild_get_member_sync(presence_update.rich_presence.guild_id, presence_update.rich_presence.user_id).get_user();
      streaming_user_name = ((streaming_user == nullptr) ? "" : streaming_user->format_username());
    } catch (const dpp::rest_exception& rest_exception) {
      logger_->error("Failed to get user name. Exception description: {}", rest_exception.what());
      //TODO
    }

    const auto& activies = presence_update.rich_presence.activities;
    const auto streaming_activity = std::find_if(activies.cbegin(), activies.cend(), [](const dpp::activity& activity) {
      return (activity.type == dpp::activity_type::at_streaming) && (activity.name == "Twitch") && (activity.state == "Super Mario 64");
    });

    const std::lock_guard<std::mutex> lock(on_presence_update_mutex_);

    const auto it_user_id_and_message_id = streaming_users_ids_and_messages_ids_.find(presence_update.rich_presence.user_id);
    const auto ping_sent_ = (streaming_users_ids_and_messages_ids_.cend() != it_user_id_and_message_id);
    if (activies.cend() == streaming_activity) {
      if (ping_sent_) {
        logger_->info("User '{}', but ping was already sent", streaming_user_name);

        bot_.message_delete_sync(it_user_id_and_message_id->second, database_.GetServerChannelId(Database::ServerChannels::kStreams));
        streaming_users_ids_and_messages_ids_.erase(it_user_id_and_message_id);
      }

      return;
    }

    if (ping_sent_) {
      logger_->info("Found Twitch streaming presence update for user '{}', but ping was already sent", streaming_user_name);
      return;
    }
    
    logger_->info("Found Twitch streaming presence update for user '{}'. Sending ping", streaming_user_name);

    const auto ping_message = dpp::message(database_.GetServerChannelId(Database::ServerChannels::kStreams),
                                           fmt::format("**{}** estÃ¡ ao vivo jogando Super Mario 64! Assista em: **{}**", streaming_user_name, streaming_activity->url));
    try {
      const auto ping_message_id = bot_.message_create_sync(ping_message).id;
      streaming_users_ids_and_messages_ids_[presence_update.rich_presence.user_id] = ping_message_id;
    }
    catch (const dpp::rest_exception& rest_exception) {
      // TODO
    }

    logger_->info("Finished processing presence update for user '{}'", streaming_user_name);
  }));
}

void Sm64brDiscordBot::OnGuildMemberAdd(const dpp::guild_member_add_t& guild_member_add) noexcept {
  const auto join_message = dpp::message(database_.GetServerChannelId(Database::ServerChannels::kUpdates),
                                         fmt::format("{} acabou de entrar no servidor.", guild_member_add.added.get_user()->format_username()));
  bot_.message_create(join_message);
}

void Sm64brDiscordBot::OnGuildMemberRemove(const dpp::guild_member_remove_t& guild_member_remove) noexcept {
  const auto leave_message = dpp::message(database_.GetServerChannelId(Database::ServerChannels::kUpdates),
                                          fmt::format("{} acabou de sair no servidor.", guild_member_remove.removed.format_username()));
  bot_.message_create(leave_message);
}

void Sm64brDiscordBot::OnReady(const dpp::ready_t& ready) {
  logger_->info("Bot event handler loop is running and ready to start processing requests");
}

void Sm64brDiscordBot::ClearStreamingStatus() noexcept {
  dpp::snowflake highest_member_id = {};
  dpp::guild_member_map guild_members;
  do {
    guild_members = bot_.guild_get_members_sync(database_.GetServerGuildId(), 1000, highest_member_id);
    for (auto& member : guild_members) {
      if (highest_member_id < member.first) {
        highest_member_id = member.first;
      }

      const auto& roles = member.second.get_roles();
      const auto it_streaming_role =  std::find(roles.cbegin(), roles.cend(), database_.GetServerRoleId(Database::ServerRoles::kStreaming));
      if (it_streaming_role != roles.cend()) {
        bot_.guild_member_remove_role_sync(database_.GetServerGuildId(), member.second.user_id, *it_streaming_role);
      }
    }
  } while (!guild_members.empty());

  dpp::snowflake earliest_streaming_message_id = std::numeric_limits<dpp::snowflake>::max();
  const auto streams_channel_id = database_.GetServerChannelId(Database::ServerChannels::kStreams);
  dpp::message_map streaming_messages = bot_.messages_get_sync(streams_channel_id, {}, {}, {}, 100);
  do {
    for (const auto& message : streaming_messages) {
      if (message.first < earliest_streaming_message_id) {
        earliest_streaming_message_id = message.first;
      }

      bot_.message_delete_sync(message.first, streams_channel_id);
    }

    streaming_messages = bot_.messages_get_sync(streams_channel_id, {}, earliest_streaming_message_id, {}, 100);
  } while (!streaming_messages.empty());
}
