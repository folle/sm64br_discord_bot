#include "sm64br_discord_bot.h"

#include <algorithm>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

Sm64brDiscordBot::Sm64brDiscordBot() {
  bot_->on_log([this](dpp::log_t const& event) { OnLog(event); });
  bot_->on_ready([this](dpp::ready_t const& ready) { OnReady(ready); });
  bot_->on_message_create([this](dpp::message_create_t const& message_create) { OnMessageCreate(message_create); });
  bot_->on_message_reaction_add([this](dpp::message_reaction_add_t const& message_reaction_add) { OnMessageReactionAdd(message_reaction_add); });
  bot_->on_presence_update([this](dpp::presence_update_t const& presence_update) { OnPresenceUpdate(presence_update); });
  bot_->on_guild_member_add([this](dpp::guild_member_add_t const& guild_member_add) { OnGuildMemberAdd(guild_member_add); });
  bot_->on_guild_member_remove([this](dpp::guild_member_remove_t const& guild_member_remove) { OnGuildMemberRemove(guild_member_remove); });

  ClearStreamingRoles();
  ClearStreamingMessages();

  logger_.Info("Initialized bot");
}

Sm64brDiscordBot::~Sm64brDiscordBot() {
  logger_.Info("Bot terminated");
}

void Sm64brDiscordBot::Start() const noexcept {
  logger_.Info("Starting bot event handler loop");
  bot_->start(dpp::st_wait);
}

void Sm64brDiscordBot::OnLog(dpp::log_t const& log) const noexcept {
  switch (log.severity) {
    case dpp::ll_trace: {
      logger_.Trace("{}", log.message);
      break;
    }
    case dpp::ll_debug: {
      logger_.Debug("{}", log.message);
      break;
    }
    case dpp::ll_info: {
      logger_.Info("{}", log.message);
      break;
    }
    case dpp::ll_warning: {
      logger_.Warn("{}", log.message);
      break;
    }
    case dpp::ll_error: {
      logger_.Error("{}", log.message);
      break;
    }
    case dpp::ll_critical: {
      [[fallthrough]];
    }
    default: {
      logger_.Critical("{}", log.message);
      break;
    }
  }
}

void Sm64brDiscordBot::OnMessageCreate(dpp::message_create_t const& message_create) noexcept {
  message_create_futures_.remove_if([](auto const& future) { return std::future_status::ready == future.wait_for(std::chrono::milliseconds(0)); });

  message_create_futures_.push_back(std::async(std::launch::async, [this, message_create]() {
    message_handler_.Process(message_create.msg);
  }));
}

void Sm64brDiscordBot::OnMessageReactionAdd(dpp::message_reaction_add_t const& message_reaction_add) noexcept {
  message_reaction_futures_.remove_if([](auto const& future) { return std::future_status::ready == future.wait_for(std::chrono::milliseconds(0)); });

  if (message_reaction_add.message_author_id != bot_->me.id) {
    return;
  }

  auto const& message_id = message_reaction_add.message_id;
  auto const& channel_id = message_reaction_add.channel_id;

  try {
    auto const raw_event_json = nlohmann::json::parse(message_reaction_add.raw_event);
    if (dpp::snowflake(raw_event_json["d"]["user_id"].get<std::string>()) == bot_->me.id) {
      return;
    }
  } catch (nlohmann::json::exception const& json_exception) {
    logger_.Error("Failed to get user id from nomination message '{}' in channel '{}'. Exception: '{}'", message_id, channel_id, json_exception.what());
    return;
  }
  
  message_reaction_futures_.push_back(std::async(std::launch::async, [this, message_id, channel_id]() {
    dpp::message nomination_message;
    try {
      nomination_message = bot_->message_get_sync(message_id, channel_id);
    } catch (dpp::exception const& rest_exception) {
      logger_.Error("Failed to get nomination message '{}' in channel '{}'. Exception: '{}'", message_id, channel_id, rest_exception.what());
      return;
    }

    auto const& reactions = nomination_message.reactions;
    auto const user_reaction = std::find_if(reactions.cbegin(), reactions.cend(), [](auto const& reaction) {
      return reaction.count > 1;
    });

    if (user_reaction == reactions.end()) {
      return;
    }

    auto const& content = nomination_message.content;

    auto const awards_reactions_and_categories = Settings::Get().GetAwardsReactionsAndCategories();
    if (!awards_reactions_and_categories.contains(user_reaction->emoji_name)) {
      logger_.Error("Received an invalid awards reaction '{}' in message '{}'", user_reaction->emoji_name, content);
      return;
    }
    auto const nominated_category = awards_reactions_and_categories.at(user_reaction->emoji_name);

    auto const clip_url = content.substr(content.rfind('\n') + 1);
    if (clip_url.empty()) {
      logger_.Error("Received an invalid awards message '{}' without a video", content);
      return;
    }

    auto const petalite_user_id = Settings::Get().GetUserId(Settings::Users::kPetalite);
    auto const petalite_content = fmt::format("Clipe: {}\nCategoria: {}", clip_url, nominated_category);
    bot_->direct_message_create(petalite_user_id, dpp::message(petalite_content));
  }));
}

void Sm64brDiscordBot::OnPresenceUpdate(dpp::presence_update_t const& presence_update) noexcept {
  presence_update_futures_.remove_if([](auto const& future) { return std::future_status::ready == future.wait_for(std::chrono::milliseconds(0)); });

  presence_update_futures_.push_back(std::async(std::launch::async, [this, presence_update]() {
    auto const& activies = presence_update.rich_presence.activities;
    auto const streaming_activity = std::find_if(activies.cbegin(), activies.cend(), [](auto const& activity) {
      return (activity.type == dpp::activity_type::at_streaming) &&
        (((activity.name == "Twitch") && (activity.state == "Super Mario 64")) ||
         ((activity.name == "YouTube") && (activity.details.contains("Mario 64") || activity.details.contains("SM64"))));
    });
    auto const is_streaming_sm64 = activies.cend() != streaming_activity;

    auto const& streaming_user_id = presence_update.rich_presence.user_id;

    std::scoped_lock<std::mutex> const mutex_lock(on_presence_update_mutex_);

    auto const it_user_id_and_message_id = streaming_users_ids_and_messages_ids_.find(streaming_user_id);
    auto const streaming_message_sent_ = (streaming_users_ids_and_messages_ids_.cend() != it_user_id_and_message_id);
    if (is_streaming_sm64) {
      if (!streaming_message_sent_) {
        logger_.Info("User '{}' started streaming Super Mario 64", streaming_user_id);

        auto const streaming_message = dpp::message(Settings::Get().GetChannelId(Settings::Channels::kStreams),
                                                    fmt::format("{} **{}**\n{}", 
                                                                dpp::user::get_mention(streaming_user_id),
                                                                streaming_activity->details,
                                                                streaming_activity->url));
        dpp::snowflake streaming_message_id{};
        try {
          streaming_message_id = bot_->message_create_sync(streaming_message).id;
        } catch (dpp::rest_exception const& rest_exception) {
          logger_.Error("Failed to create streaming message for user '{}' while processing presence update. Exception '{}'",
                        streaming_user_id, rest_exception.what());
          return;
        }

        bot_->guild_member_add_role(Settings::Get().GetGuildId(), streaming_user_id, Settings::Get().GetRoleId(Settings::Roles::kStreaming));

        streaming_users_ids_and_messages_ids_[streaming_user_id] = streaming_message_id;
      }
    } else {
      if (streaming_message_sent_) {
        logger_.Info("User '{}' finished streaming Super Mario 64", streaming_user_id);

        bot_->message_delete(it_user_id_and_message_id->second, Settings::Get().GetChannelId(Settings::Channels::kStreams));
        bot_->guild_member_remove_role(Settings::Get().GetGuildId(), streaming_user_id, Settings::Get().GetRoleId(Settings::Roles::kStreaming));

        streaming_users_ids_and_messages_ids_.erase(it_user_id_and_message_id);
      }
    }
  }));
}

void Sm64brDiscordBot::OnGuildMemberAdd(dpp::guild_member_add_t const& guild_member_add) const noexcept {
  auto const join_message = dpp::message(Settings::Get().GetChannelId(Settings::Channels::kUpdates),
                                         fmt::format("**{}** acabou de entrar no servidor.", guild_member_add.added.get_user()->get_mention()));
  bot_->message_create(join_message);
}

void Sm64brDiscordBot::OnGuildMemberRemove(dpp::guild_member_remove_t const& guild_member_remove) const noexcept {
  auto const leave_message = dpp::message(Settings::Get().GetChannelId(Settings::Channels::kUpdates),
                                          fmt::format("**{}** acabou de sair no servidor.", guild_member_remove.removed.get_mention()));
  bot_->message_create(leave_message);
}

void Sm64brDiscordBot::OnReady(dpp::ready_t const& ready) const noexcept {
  logger_.Info("Bot event handler loop started");
}

void Sm64brDiscordBot::ClearStreamingRoles() const noexcept {
  dpp::snowflake highest_member_id{};
  dpp::guild_member_map members;
  do {
    uint16_t constexpr kMaxMembersPerCall = 1000;
    members = bot_->guild_get_members_sync(Settings::Get().GetGuildId(), kMaxMembersPerCall, highest_member_id);
    for (auto& member : members) {
      if (highest_member_id < member.first) {
        highest_member_id = member.first;
      }

      auto const& roles = member.second.get_roles();
      auto const it_streaming_role =  std::find(roles.cbegin(), roles.cend(), Settings::Get().GetRoleId(Settings::Roles::kStreaming));
      if (it_streaming_role != roles.cend()) {
        bot_->guild_member_remove_role_sync(Settings::Get().GetGuildId(), member.second.user_id, Settings::Get().GetRoleId(Settings::Roles::kStreaming));
      }
    }
  } while (!members.empty());
}

void Sm64brDiscordBot::ClearStreamingMessages() const noexcept {
  dpp::snowflake highest_streaming_message_id = 1ULL;
  dpp::message_map streaming_messages;
  do {
    auto constexpr kMaxMessagesPerCall = 100ULL;
    streaming_messages = bot_->messages_get_sync(Settings::Get().GetChannelId(Settings::Channels::kStreams), {}, {}, 
                                                 highest_streaming_message_id, kMaxMessagesPerCall);

    for (auto const& streaming_message : streaming_messages) {
      if (highest_streaming_message_id < streaming_message.first) {
        highest_streaming_message_id = streaming_message.first;
      }

      bot_->message_delete_sync(streaming_message.first, Settings::Get().GetChannelId(Settings::Channels::kStreams));
    }
  } while (!streaming_messages.empty());
}