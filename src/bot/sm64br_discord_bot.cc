#include "sm64br_discord_bot.h"

#include <algorithm>
#include <chrono>
#include <exception>
#include <functional>
#include <limits>
#include <regex>
#include <sstream>
#include <fmt/format.h>


namespace {
  template<class Container>
  void ClearFinishedFutures(Container& futures) {
    auto finished_futures_begin_ = std::remove_if(futures.begin(), futures.end(), [](const auto& future) {
      return (std::future_status::ready == future.wait_for(std::chrono::milliseconds(0)));
     });
    futures.erase(finished_futures_begin_, futures.end());
  }
}


Sm64brDiscordBot::Sm64brDiscordBot() :
  server_database_("database/server_database.json") {
  bot_.on_log(std::bind(&Sm64brDiscordBot::OnLog, this, std::placeholders::_1));
  bot_.on_ready(std::bind(&Sm64brDiscordBot::OnReady, this, std::placeholders::_1));
  bot_.on_message_create(std::bind(&Sm64brDiscordBot::OnMessageCreate, this, std::placeholders::_1));
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
                                 [this](const auto& role) { return (role == server_database_.GetRoleId(ServerDatabase::Roles::kModerator)); });
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
    std::string user_name;
    try {
      auto const streaming_user = bot_.guild_get_member_sync(presence_update.rich_presence.guild_id, presence_update.rich_presence.user_id).get_user();
      user_name = ((streaming_user == nullptr) ? "" : streaming_user->format_username());
    } catch (const dpp::rest_exception& rest_exception) {
      logger_->error("Failed to get user name. Exception description: {}", rest_exception.what());
      //TODO
    }

    logger_->info("Starting to process presence update for user '{}'", user_name);

    const auto& activies = presence_update.rich_presence.activities;
    const auto streaming_activity = std::find_if(activies.cbegin(), activies.cend(), [](const dpp::activity& activity) {
      return (activity.type == dpp::activity_type::at_streaming) && (activity.name == "Twitch") && (activity.state == "Super Mario 64");
    });

    const std::lock_guard<std::mutex> lock(on_presence_update_mutex_);

    auto it_user_id = std::find(streaming_users_ids_.begin(), streaming_users_ids_.end(), presence_update.rich_presence.user_id);
    const auto ping_sent_ = (streaming_users_ids_.end() != it_user_id);

    if (activies.cend() == streaming_activity) {
      logger_->info("No Twitch streaming presence found for user '{}'", user_name);
      
      if (ping_sent_) {
        streaming_users_ids_.erase(it_user_id);
      }

      return;
    }

    if (ping_sent_) {
      logger_->info("Found Twitch streaming presence update for user '{}', but ping was already sent", user_name);
      return;
    }
    
    logger_->info("Found Twitch streaming presence update for user '{}'. Sending ping", user_name);
    const auto ping_message = dpp::message(server_database_.GetChannelId(ServerDatabase::Channels::kStreams),
                                           fmt::format("**@{}** está ap vivo jogando Super Mario 64! Assista em: **{}**", user_name, streaming_activity->url));
    bot_.message_create(ping_message);
      
    logger_->info("Finished processing presence update for user '{}'", user_name);
  }));
}

void Sm64brDiscordBot::OnGuildMemberAdd(const dpp::guild_member_add_t& guild_member_add) noexcept {
  const auto join_message = dpp::message(server_database_.GetChannelId(ServerDatabase::Channels::kUpdates),
                                         fmt::format("{} acabou de entrar no servidor.", guild_member_add.added.get_user()->format_username()));
  bot_.message_create(join_message);
}

void Sm64brDiscordBot::OnGuildMemberRemove(const dpp::guild_member_remove_t& guild_member_remove) noexcept {
  const auto leave_message = dpp::message(server_database_.GetChannelId(ServerDatabase::Channels::kUpdates),
                                          fmt::format("{} acabou de sair no servidor.", guild_member_remove.removed.format_username()));
  bot_.message_create(leave_message);
}

void Sm64brDiscordBot::OnReady(const dpp::ready_t& ready) {
  logger_->info("Bot event handler loop is running and ready to start processing requests");
}

void Sm64brDiscordBot::ClearStreamingStatus() noexcept {
  dpp::guild_member_map guild_members;
  do {
    guild_members = bot_.guild_get_members_sync(server_database_.GetGuildId(), 1000, dpp::snowflake());
    for (auto& member : guild_members) {
      const auto& roles = member.second.get_roles();
      const auto it_streaming_role =  std::find(roles.cbegin(), roles.cend(), server_database_.GetRoleId(ServerDatabase::Roles::kStreaming));
      if (it_streaming_role != roles.cend()) {
        member.second.remove_role(*it_streaming_role);
      }
    }
  } while (!guild_members.empty());

  const auto streams_channel_id = server_database_.GetChannelId(ServerDatabase::Channels::kStreams);

  dpp::channel streams_channel = bot_.channel_get_sync(streams_channel_id);
  while (streams_channel.last_message_id) {
    bot_.message_delete_sync(streams_channel.last_message_id, streams_channel_id);
    streams_channel = bot_.channel_get_sync(streams_channel_id);
  } 
}
