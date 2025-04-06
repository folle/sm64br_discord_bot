#include "message_handler.h"

#include <algorithm>
#include <utility>

#include <fmt/format.h>

#include "settings/settings.h"

namespace{
  std::string RemoveCommandHeader(std::string const& content) {
    auto constexpr kCommandLength = 3ULL;
    return content.substr(kCommandLength, content.size() - kCommandLength);
  }
}

MessageHandler::MessageHandler(std::shared_ptr<dpp::cluster> bot) noexcept :
  bot_(std::move(bot)) {
  nomination_content_header_ = std::string("Você gostaria de indicar esse vídeo para o Super Mario 64 Brasil Awards? Se sim, reaja de acordo com a categoria desejada (apenas uma reação por vídeo):\n");

  auto const awards_reactions_and_categories = Settings::Get().GetAwardsReactionsAndCategories();
  for (auto it = awards_reactions_and_categories.begin(); it != awards_reactions_and_categories.end(); ++it) {
    nomination_content_header_.append(fmt::format("{} - {}\n", it->first, it->second));
  }
}

void MessageHandler::Process(dpp::message const& message) noexcept {
  auto const member_confirmation = bot_->co_guild_get_member(message.guild_id, message.author.id).sync_wait();
  if (member_confirmation.is_error()) {
    logger_.Error("Failed to get member while processing message create. Error '{}'", member_confirmation.get_error().human_readable);
    return;
  }

  auto const member = member_confirmation.get<dpp::guild_member>();
  auto const from_moderator = std::any_of(member.get_roles().begin(),
                                          member.get_roles().end(),
                                          [this](auto const& role) { return Settings::Get().GetRoleId(Settings::Roles::kModerator) == role; });
  auto const from_bot = message.author.is_bot();

  auto const is_announcement_message = 0 == message.content.rfind("!a ", 0);
  if (is_announcement_message && from_moderator && !from_bot) {
    ProcessAnnouncementMessage(message.channel_id, message.content);
    return;
  }

  auto const is_general_message = 0 == message.content.rfind("!m ", 0);
  if (is_general_message && from_moderator && !from_bot) {
    ProcessGeneralMessage(message.channel_id, message.content);
    return;
  }

  auto const is_streaming_message = Settings::Get().GetChannelId(Settings::Channels::kStreams) == message.channel_id;
  if (is_streaming_message && !from_bot) {
    ProcessStreamingMessage(message.author.id, message.id, message.content);
    return;
  }
  
  auto const is_clip_message = Settings::Get().GetChannelId(Settings::Channels::kClips) == message.channel_id;
  if (is_clip_message && !from_bot) {
    ProcessAwardsMessage(message.author.id, message.id, message.content, message.attachments);
    return;
  }
}

void MessageHandler::ProcessAnnouncementMessage(dpp::snowflake const channel_id, std::string const& content) const noexcept {
  auto const announcement = fmt::format("@everyone {}", ::RemoveCommandHeader(content));
  logger_.Info("Received announcement message '{}'", announcement);

  auto const announcement_message = dpp::message(channel_id, announcement).set_allowed_mentions(false, false, true);
  bot_->message_create(announcement_message);
}

void MessageHandler::ProcessGeneralMessage(dpp::snowflake const channel_id, std::string const& content) const noexcept {
  auto const general_announcement = ::RemoveCommandHeader(content);
  logger_.Info("Received general message '{}'", general_announcement);

  bot_->message_create(dpp::message(channel_id, general_announcement));
}

void MessageHandler::ProcessStreamingMessage(dpp::snowflake const user_id, dpp::snowflake const message_id, std::string const& content) noexcept {
  logger_.Info("Received streaming message with id '{}'", message_id);

  if (std::regex_search(content, url_regex_)) {
    auto constexpr kStreamingMessageDeleteDelay = std::chrono::hours(6);
    std::this_thread::sleep_for(kStreamingMessageDeleteDelay);
  }
  else {
    auto const invalid_streaming_message = "Por favor, poste apenas mensagens com uma URL para uma stream de Super Mario 64 no canal **#streams**!";
    bot_->direct_message_create(user_id, dpp::message(invalid_streaming_message));
  }

  bot_->message_delete(message_id, Settings::Get().GetChannelId(Settings::Channels::kStreams));

  logger_.Info("Deleted streaming message with id '{}'", message_id);
}

void MessageHandler::ProcessAwardsMessage(dpp::snowflake const user_id, dpp::snowflake const message_id, std::string const& content, std::vector<dpp::attachment> const& attachments) noexcept {
  for (auto it = std::sregex_iterator(content.begin(), content.end(), url_regex_); it != std::sregex_iterator(); ++it) {
    SendNominationMessage(user_id, it->str());
  }

  for (auto const attachment : attachments) {
    if (attachment.content_type.rfind("video/", 0) != 0) {
      continue;
    }

    SendNominationMessage(user_id, attachment.url);
  }
}

void MessageHandler::SendNominationMessage(dpp::snowflake const user_id, std::string const& clip_url) noexcept {
  auto nomination_content = nomination_content_header_;
  nomination_content.append(clip_url);

  auto const sent_message_confirmation = bot_->co_direct_message_create(user_id, dpp::message(nomination_content)).sync_wait();
  if (sent_message_confirmation.is_error()) {
    logger_.Error("Failed to send nomination message '{}' to user '{}'. Error: '{}'", nomination_content, user_id, sent_message_confirmation.get_error().human_readable);
    return;
  }
  
  auto const sent_message = sent_message_confirmation.get<dpp::message>();
  auto const awards_reactions_and_categories = Settings::Get().GetAwardsReactionsAndCategories();
  for (auto it = awards_reactions_and_categories.begin(); it != awards_reactions_and_categories.end(); ++it) {
    auto const add_reaction_confirmation = bot_->co_message_add_reaction(sent_message, it->first).sync_wait();
    if (add_reaction_confirmation.is_error()) {
      logger_.Error("Failed to add awards reaction '{}' in nomination message '{}' to user '{}. Error: '{}'", it->first, sent_message.id, user_id, add_reaction_confirmation.get_error().human_readable);
    }
  }
}