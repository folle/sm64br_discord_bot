#include "sm64br_discord_bot.h"


Sm64brDiscordBot::Sm64brDiscordBot() {
  bot_->on_log([this](const dpp::log_t& event) { OnLog(event); });
  bot_->on_ready([this](const dpp::ready_t& ready) { OnReady(ready); });
  bot_->on_message_create([this](const dpp::message_create_t& message_create) { OnMessageCreate(message_create); });
  bot_->on_presence_update([this](const dpp::presence_update_t& presence_update) { OnPresenceUpdate(presence_update); });
  bot_->on_guild_member_add([this](const dpp::guild_member_add_t& guild_member_add) { OnGuildMemberAdd(guild_member_add); });
  bot_->on_guild_member_remove([this](const dpp::guild_member_remove_t& guild_member_remove) { OnGuildMemberRemove(guild_member_remove); });

  ClearStreamingStatus();

  logger_->info("Initialized bot");
}

Sm64brDiscordBot::~Sm64brDiscordBot() {
  logger_->info("Bot terminated");
  logger_->flush();
}

void Sm64brDiscordBot::Start() const noexcept {
  logger_->info("Starting bot event handler loop");
  bot_->start(dpp::st_wait);
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
  message_create_futures_.remove_if([](const auto& future) { return (std::future_status::ready == future.wait_for(std::chrono::milliseconds(0))); });

  message_create_futures_.push_back(std::async(std::launch::async, [this, message_create]() {
    message_handler_.Process(message_create.msg);
  }));
}

void Sm64brDiscordBot::OnPresenceUpdate(const dpp::presence_update_t& presence_update) noexcept {
  presence_update_futures_.remove_if([](const auto& future) { return (std::future_status::ready == future.wait_for(std::chrono::milliseconds(0))); });

  presence_update_futures_.push_back(std::async(std::launch::async, [this, presence_update]() {
    dpp::user* streaming_user = {};
    try {
      streaming_user = bot_->guild_get_member_sync(database_->GetGuildId(), presence_update.rich_presence.user_id).get_user();
    } catch (const dpp::rest_exception& rest_exception) {
      logger_->error("Failed to get member while processing presence update. Exception '{}'", rest_exception.what());
      return;
    }

    if (!streaming_user) {
      logger_->error("Streaming user wasn't found in the cache while processing presence update");
      return;
    }

    const auto& streaming_username = streaming_user->format_username();
    const auto& activies = presence_update.rich_presence.activities;
    const auto streaming_activity = std::find_if(activies.cbegin(), activies.cend(), [](const auto& activity) {
      return (activity.type == dpp::activity_type::at_streaming) && (activity.name == "Twitch") && (activity.state == "Super Mario 64");
    });
    const auto is_streaming_sm64 = activies.cend() != streaming_activity;

    const auto& streaming_user_id = presence_update.rich_presence.user_id;

    const std::lock_guard<std::mutex> mutex_lock(on_presence_update_mutex_);

    const auto it_user_id_and_message_id = streaming_users_ids_and_messages_ids_.find(streaming_user_id);
    const auto streaming_message_sent_ = (streaming_users_ids_and_messages_ids_.cend() != it_user_id_and_message_id);
    if (is_streaming_sm64) {
      if (!streaming_message_sent_) {
        logger_->info("User '{}' started streaming Super Mario 64", streaming_username);

        const auto streaming_message = dpp::message(database_->GetChannelId(Database::Channels::kStreams),
                                                    fmt::format("**{}** encontra-se ao vivo jogando Super Mario 64! Assista em: {}", 
                                                                streaming_username,
                                                                streaming_activity->url));
        dpp::snowflake streaming_message_id = {};
        try {
          streaming_message_id = bot_->message_create_sync(streaming_message).id;
        }
        catch (const dpp::rest_exception& rest_exception) {
          logger_->error("Failed to create streaming message while processing presence update. Exception '{}'", rest_exception.what());
          return;
        }

        bot_->guild_member_add_role(database_->GetGuildId(), streaming_user_id, database_->GetRoleId(Database::Roles::kStreaming));

        streaming_users_ids_and_messages_ids_[streaming_user_id] = streaming_message_id;
      }
    } else {
      if (streaming_message_sent_) {
        logger_->info("User '{}' finished streaming Super Mario 64", streaming_username);

        bot_->message_delete(it_user_id_and_message_id->second, database_->GetChannelId(Database::Channels::kStreams));
        bot_->guild_member_remove_role(database_->GetGuildId(), streaming_user_id, database_->GetRoleId(Database::Roles::kStreaming));

        streaming_users_ids_and_messages_ids_.erase(it_user_id_and_message_id);
      }
    }
  }));
}

void Sm64brDiscordBot::OnGuildMemberAdd(const dpp::guild_member_add_t& guild_member_add) const noexcept {
  const auto join_message = dpp::message(database_->GetChannelId(Database::Channels::kUpdates),
                                         fmt::format("**@{}** acabou de entrar no servidor.", guild_member_add.added.get_user()->format_username()));
  bot_->message_create(join_message);
}

void Sm64brDiscordBot::OnGuildMemberRemove(const dpp::guild_member_remove_t& guild_member_remove) const noexcept {
  const auto leave_message = dpp::message(database_->GetChannelId(Database::Channels::kUpdates),
                                          fmt::format("**@{}** acabou de sair no servidor.", guild_member_remove.removed.format_username()));
  bot_->message_create(leave_message);
}

void Sm64brDiscordBot::OnReady(const dpp::ready_t& ready) const noexcept {
  logger_->info("Bot event handler loop started");
}

void Sm64brDiscordBot::ClearStreamingStatus() const noexcept {
  // Clear streaming roles
  dpp::snowflake highest_member_id = {};
  dpp::guild_member_map members;
  do {
    try {
      constexpr uint16_t kMaxMembersPerCall = 1000;
      members = bot_->guild_get_members_sync(database_->GetGuildId(), kMaxMembersPerCall, highest_member_id);
    } catch (const dpp::rest_exception& rest_exception) {
      logger_->error("Failed to get members while clearing streaming status. Exception '{}'", rest_exception.what());
      return;
    }

    for (auto& member : members) {
      if (highest_member_id < member.first) {
        highest_member_id = member.first;
      }

      const auto& roles = member.second.get_roles();
      const auto it_streaming_role =  std::find(roles.cbegin(), roles.cend(), database_->GetRoleId(Database::Roles::kStreaming));
      if (it_streaming_role != roles.cend()) {
        bot_->guild_member_remove_role(database_->GetGuildId(), member.second.user_id, database_->GetRoleId(Database::Roles::kStreaming));
      }
    }
  } while (!members.empty());

  // Clear streaming messages
  auto earliest_streaming_message_id = std::numeric_limits<dpp::snowflake>::max();
  dpp::message_map streaming_messages;
  do {
    try {
      constexpr uint64_t kMaxMessagesPerCall = 100;
      streaming_messages = bot_->messages_get_sync(database_->GetChannelId(Database::Channels::kStreams), {}, {}, {}, kMaxMessagesPerCall);
    } catch (const dpp::rest_exception& rest_exception) {
      logger_->error("Failed to get streaming messages while clearing streaming status. Exception '{}'", rest_exception.what());
      return;
    }

    for (const auto& streaming_message : streaming_messages) {
      if (!earliest_streaming_message_id || (streaming_message.first < earliest_streaming_message_id)) {
        earliest_streaming_message_id = streaming_message.first;
      }

      bot_->message_delete(streaming_message.first, database_->GetChannelId(Database::Channels::kStreams));
    }
  } while (!streaming_messages.empty());
}
