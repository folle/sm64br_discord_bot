#include "message_handler.h"

#include <algorithm>
#include <regex>
#include <utility>

#include <boost/algorithm/string.hpp>

#include "settings/settings.h"

namespace{
  std::string RemoveCommandHeader(std::string const& message) {
    constexpr auto kCommandLength = 3ULL;
    return message.substr(kCommandLength, message.size() - kCommandLength);
  }
}

MessageHandler::MessageHandler(std::shared_ptr<dpp::cluster> bot) noexcept :
  bot_(std::move(bot)) {
    
}

void MessageHandler::Process(dpp::message const& message) noexcept {
  dpp::guild_member member;
  try {
    member = bot_->guild_get_member_sync(message.guild_id, message.author.id);
  }
  catch (dpp::rest_exception const& rest_exception) {
    logger_.Error("Failed to get member while processing message create. Exception '{}'", rest_exception.what());
    return;
  }

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
  auto const is_ooc_message = Settings::Get().GetChannelId(Settings::Channels::kOoc) == message.channel_id;
  if ((is_clip_message ||  is_ooc_message) && !from_bot) {
    ProcessAwardsMessage(message.author.id, message.id, message.content);
    return;
  }
}

void MessageHandler::ProcessAnnouncementMessage(dpp::snowflake const channel_id, std::string const& message) const noexcept {
  auto const announcement = fmt::format("@everyone {}", ::RemoveCommandHeader(message));
  logger_.Info("Received announcement message '{}'", announcement);

  auto const announcement_message = dpp::message(channel_id, announcement).set_allowed_mentions(false, false, true);
  bot_->message_create(announcement_message);
}

void MessageHandler::ProcessGeneralMessage(dpp::snowflake const channel_id, std::string const& message) const noexcept {
  auto const general_message = ::RemoveCommandHeader(message);
  logger_.Info("Received general message '{}'", general_message);

  bot_->message_create(dpp::message(channel_id, general_message));
}

void MessageHandler::ProcessStreamingMessage(dpp::snowflake const user_id, dpp::snowflake const message_id, std::string const& message) noexcept {
  logger_.Info("Received streaming message with id '{}'", message_id);

  auto const kUrlRegex = std::regex("((http|https)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)");
  if (std::regex_search(message, kUrlRegex)) {
    constexpr auto kStreamingMessageDeleteDelay = std::chrono::hours(6);
    std::this_thread::sleep_for(kStreamingMessageDeleteDelay);
  }
  else {
    auto const invalid_streaming_message = "Por favor, poste apenas mensagens com uma URL para uma stream de Super Mario 64 no canal **#streams**!";
    bot_->direct_message_create(user_id, dpp::message(invalid_streaming_message));
  }

  bot_->message_delete(message_id, Settings::Get().GetChannelId(Settings::Channels::kStreams));

  logger_.Info("Deleted streaming message with id '{}'", message_id);
}

void MessageHandler::ProcessAwardsMessage(dpp::snowflake const user_id, dpp::snowflake const message_id, std::string const& message) noexcept {

//
//boost::algorithm::split(strVec,str,is_any_of("\t "),boost::token_compress_on); 
//  melhor pop-off
//  melhor meme da comunidade
//  momento mais engraçado
//  momento mais insano,
//  melhor rage
//  melhor clutch
//  momento skill issue
//
//  estrela em ascensão
//  pb mais merecido
//  streamer do ano
//  jogador do ano
}