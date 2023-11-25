#include "message_handler.h"


namespace{
  std::string RemoveCommandHeader(const std::string& message) {
    constexpr std::size_t kCommandLength = 3;
    return message.substr(kCommandLength, message.size() - kCommandLength);
  }
}


MessageHandler::MessageHandler(const std::shared_ptr<Database> database, const std::shared_ptr<dpp::cluster> bot) noexcept :
  database_(database), bot_(bot) {

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

  // Announcement message
  const auto is_announcement_message = !message.content.rfind("!a ", 0);
  if (is_announcement_message && from_moderator && !from_bot) {
    ProcessAnnouncementMessage(message.channel_id, message.content);
    return;
  }

  // General Message
  const auto is_general_message = !message.content.rfind("!m ", 0);
  if (is_general_message && from_moderator && !from_bot) {
    ProcessGeneralMessage(message.channel_id, message.content);
    return;
  }

  const auto is_streaming_message = (message.channel_id == database_->GetChannelId(Database::Channels::kStreams));
  if (is_streaming_message && !from_bot) {
    ProcessStreamingMessage(message.id);
    return;
  }

  const auto is_pb_submission_message = (message.channel_id == database_->GetChannelId(Database::Channels::kPbSubmission));
  if (is_pb_submission_message && !from_bot) {
    ProcessPbSubmissionMessage(message.id, message.content);
    return;
  }


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

void MessageHandler::ProcessStreamingMessage(const dpp::snowflake message_id) noexcept {
  streaming_message_futures_.remove_if([](const auto& future) { return (std::future_status::ready == future.wait_for(std::chrono::milliseconds(0))); });

  logger_->info("Received streaming message with id '{}'", message_id);

  streaming_message_futures_.push_back(std::async(std::launch::async, [this, message_id]() {
    constexpr auto kStreamingMessageDeleteDelay = std::chrono::hours(6);
    std::this_thread::sleep_for(kStreamingMessageDeleteDelay);

    bot_->message_delete(message_id, database_->GetChannelId(Database::Channels::kStreams));

    logger_->info("Deleted streaming message with id '{}'", message_id);
  }));
}

void MessageHandler::ProcessPbSubmissionMessage(const dpp::snowflake message_id, const std::string& message) const noexcept {

}