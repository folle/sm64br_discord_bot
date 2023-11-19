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
  const dpp::snowflake kModeratorRoleId(1018992321632686170);

  const dpp::snowflake kStreamsChannelId(1116382308245704936);
  const dpp::snowflake kDiscordUpdateChannel(1119016096826146826);
  
  template<class Container>
  void ClearFinishedFutures(Container& futures) {
    auto finished_futures_begin_ = std::remove_if(futures.begin(), futures.end(), [](const auto& future) {
      return (std::future_status::ready == future.wait_for(std::chrono::milliseconds(0)));
     });
    futures.erase(finished_futures_begin_, futures.end());
  }
}


Sm64BrDiscordBot::Sm64BrDiscordBot() {
  bot_.on_log(std::bind(&Sm64BrDiscordBot::OnLog, this, std::placeholders::_1));
  bot_.on_ready(std::bind(&Sm64BrDiscordBot::OnReady, this, std::placeholders::_1));
  bot_.on_message_create(std::bind(&Sm64BrDiscordBot::OnMessageCreate, this, std::placeholders::_1));
  bot_.on_presence_update(std::bind(&Sm64BrDiscordBot::OnPresenceUpdate, this, std::placeholders::_1));
  bot_.on_guild_member_add(std::bind(&Sm64BrDiscordBot::OnGuildMemberAdd, this, std::placeholders::_1));
  bot_.on_guild_member_remove(std::bind(&Sm64BrDiscordBot::OnGuildMemberRemove, this, std::placeholders::_1));
}

Sm64BrDiscordBot::~Sm64BrDiscordBot() {
  logger_->flush();
}

void Sm64BrDiscordBot::Start() noexcept {
  logger_->info("Starting bot event handler loop");
  bot_.start(dpp::st_wait);
}

void Sm64BrDiscordBot::OnLog(const dpp::log_t& event) const noexcept {
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

void Sm64BrDiscordBot::OnMessageCreate(const dpp::message_create_t& message_create) noexcept {
  ::ClearFinishedFutures(message_create_futures_);

  message_create_futures_.push_back(std::async(std::launch::async, [this, message_create]() {
    const auto is_bot = message_create.msg.author.is_bot();
    const auto& message = message_create.msg;

    bool is_moderator = false;
    try {
      const auto message_guild_member = bot_.guild_get_member_sync(message.guild_id, message.author.id);
      is_moderator = std::any_of(message_guild_member.get_roles().begin(), message_guild_member.get_roles().end(), [](const auto& role) { return (role == kModeratorRoleId); });
    }
    catch (const dpp::rest_exception& rest_exception) {
      //TODO
    }


    // Announcement message
    const auto is_announcement_message = message.content.substr(0, 3) == "!a ";

    // General Message
    const auto is_general_message = message.content.substr(0, 3) == "!m ";

    message_handler_.Process(message_create);
  }));
}

void Sm64BrDiscordBot::OnPresenceUpdate(const dpp::presence_update_t& presence_update) noexcept {
  ::ClearFinishedFutures(presence_update_futures_);

  presence_update_futures_.push_back(std::async(std::launch::async, [this, presence_update]() {
    std::string user_name;
    try {
      auto const streaming_user = bot_.guild_get_member_sync(presence_update.rich_presence.guild_id, presence_update.rich_presence.user_id).get_user();
      user_name = ((streaming_user == nullptr) ? "UNKNOWN" : streaming_user->format_username());
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

    auto user_id_it = std::find(streaming_users_ids_.begin(), streaming_users_ids_.end(), presence_update.rich_presence.user_id);
    const auto ping_sent_ = (streaming_users_ids_.end() != user_id_it);

    if (activies.cend() == streaming_activity) {
      logger_->info("No Twitch streaming presence found for user '{}'", user_name);
      
      if (ping_sent_) {
        streaming_users_ids_.erase(user_id_it);
      }

      return;
    }

    if (ping_sent_) {
      logger_->info("Found Twitch streaming presence update for user '{}', but ping was already sent", user_name);
      return;
    }
    
    logger_->info("Found Twitch streaming presence update for user '{}'. Sending ping", user_name);
    const auto ping_message = dpp::message(kStreamsChannelId, fmt::format("**@{}** está ap vivo jogando Super Mario 64! Assista em: **{}**", user_name, streaming_activity->url));
    bot_.message_create(ping_message);
      
    logger_->info("Finished processing presence update for user '{}'", user_name);
  }));
}

void Sm64BrDiscordBot::OnGuildMemberAdd(const dpp::guild_member_add_t& guild_member_add) noexcept {
  const auto join_message = dpp::message(kDiscordUpdateChannel, fmt::format("{} acabou de entrar no servidor.", guild_member_add.added.get_user()->format_username()));
  bot_.message_create(join_message);
}

void Sm64BrDiscordBot::OnGuildMemberRemove(const dpp::guild_member_remove_t& guild_member_remove) noexcept {
  const auto leave_message = dpp::message(kDiscordUpdateChannel, fmt::format("{} acabou de sair no servidor.", guild_member_remove.removed.format_username()));
  bot_.message_create(leave_message);
}

void Sm64BrDiscordBot::OnReady(const dpp::ready_t& ready) {
  logger_->info("Bot event handler loop is running and ready to start processing requests");
}