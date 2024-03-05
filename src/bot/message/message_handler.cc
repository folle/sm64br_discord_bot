#include "message_handler.h"

#include <regex>


namespace{
  std::string RemoveCommandHeader(const std::string& message) {
    constexpr std::size_t kCommandLength = 3;
    return message.substr(kCommandLength, message.size() - kCommandLength);
  }
}


MessageHandler::MessageHandler(std::shared_ptr<Database> database, std::shared_ptr<dpp::cluster> bot) noexcept :
  database_(std::move(database)), bot_(std::move(bot)) {
}

void MessageHandler::Process(const dpp::message& message) noexcept {
  dpp::guild_member member;
  try {
    member = bot_->guild_get_member_sync(message.guild_id, message.author.id);
  }
  catch (const dpp::rest_exception& rest_exception) {
    logger_->error("Failed to get member while processing message create. Exception '{}'", rest_exception.what());
    return;
  }

  const auto from_moderator = std::any_of(member.get_roles().begin(),
                                          member.get_roles().end(),
                                          [this](const auto& role) { return (role == database_->GetRoleId(Database::Roles::kModerator)); });
  const auto from_bot = message.author.is_bot();

  const auto is_announcement_message = !message.content.rfind("!a ", 0);
  if (is_announcement_message && from_moderator && !from_bot) {
    ProcessAnnouncementMessage(message.channel_id, message.content);
    return;
  }

  const auto is_general_message = !message.content.rfind("!m ", 0);
  if (is_general_message && from_moderator && !from_bot) {
    ProcessGeneralMessage(message.channel_id, message.content);
    return;
  }

  const auto is_streaming_message = (message.channel_id == database_->GetChannelId(Database::Channels::kStreams));
  if (is_streaming_message && !from_bot) {
    ProcessStreamingMessage(message.author.id, message.id, message.content);
    return;
  }

  // TODO:
  // star of the week
  // beginner dw reds 
}

void MessageHandler::ProcessAnnouncementMessage(const dpp::snowflake channel_id, const std::string& message) const noexcept {
  const auto announcement_message = "@everyone " + ::RemoveCommandHeader(message);

  logger_->info("Received announcement message '{}'", announcement_message);

  bot_->message_create(dpp::message(channel_id, announcement_message));
}

void MessageHandler::ProcessGeneralMessage(const dpp::snowflake channel_id, const std::string& message) const noexcept {
  const auto general_message = ::RemoveCommandHeader(message);
  
  logger_->info("Received general message '{}'", general_message);

  bot_->message_create(dpp::message(channel_id, general_message));
}

void MessageHandler::ProcessStreamingMessage(const dpp::snowflake user_id, const dpp::snowflake message_id, const std::string& message) noexcept {
  logger_->info("Received streaming message with id '{}'", message_id);

  const auto kUrlRegex = std::regex("((http|https)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)");
  if (std::regex_search(message, kUrlRegex)) {
    constexpr auto kStreamingMessageDeleteDelay = std::chrono::hours(6);
    std::this_thread::sleep_for(kStreamingMessageDeleteDelay);
  }
  else {
    const auto invalid_streaming_message = "Por favor, poste apenas mensagens com uma URL para uma stream de Super Mario 64 no canal **#streams**!";
    bot_->direct_message_create(user_id, dpp::message(invalid_streaming_message));
  }

  bot_->message_delete(message_id, database_->GetChannelId(Database::Channels::kStreams));

  logger_->info("Deleted streaming message with id '{}'", message_id);
}