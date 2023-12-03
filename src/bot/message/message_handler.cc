#include "message_handler.h"

#include <boost/algorithm/string.hpp>


namespace{
  std::string RemoveCommandHeader(const std::string& message) {
    constexpr std::size_t kCommandLength = 3;
    return message.substr(kCommandLength, message.size() - kCommandLength);
  }

  bool GetCategory(const std::string& data, std::string& category) {
    return true;
  }

  bool GetPlatform(const std::string& data, std::string& category) {
    return true;
  }

  bool GetTime(const std::string& data, std::string& category) {
    return true;
  }

  bool GetVod(const std::string& data, std::string& category) {
    return true;
  }


  //Categoria: [0 Star / 1 Star / 16 Star(No LBLJ) / 16 Star(LBLJ) / 70 Star / 120 Star / All Signs / 31 Star]
  //Plataforma : [N64 / EMU / VC / CELL / PC]
  //Controle : [controle]
  //Tempo : [1:23 : 45 / 12 : 34 / 1 : 23]
  //VOD : [link]
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
    ProcessStreamingMessage(message.id);
    return;
  }

  const auto is_pb_submission_message = (message.channel_id == database_->GetChannelId(Database::Channels::kPbSubmission));
  if (is_pb_submission_message && !from_bot) {
    ProcessPbSubmissionMessage(message.author.id, message.author.format_username(), message.id, message.content);
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

void MessageHandler::ProcessStreamingMessage(const dpp::snowflake message_id) noexcept {
  logger_->info("Received streaming message with id '{}'", message_id);

  constexpr auto kStreamingMessageDeleteDelay = std::chrono::hours(6);
  std::this_thread::sleep_for(kStreamingMessageDeleteDelay);

  bot_->message_delete(message_id, database_->GetChannelId(Database::Channels::kStreams));

  logger_->info("Deleted streaming message with id '{}'", message_id);
}

void MessageHandler::ProcessPbSubmissionMessage(const dpp::snowflake user_id, const std::string& username, const dpp::snowflake message_id, const std::string& message) const noexcept {
  logger_->info("Received PB submission message '{}'", message);

  std::vector<std::string> pb_submission_lines;
  boost::split(pb_submission_lines, message, boost::is_any_of("\n"));

  std::string category;
  std::string platform;
  std::string controller;
  std::string time;
  std::string vod;
  for (auto& line : pb_submission_lines) {
    std::vector<std::string> key_and_data;
    boost::split(key_and_data, line, boost::is_any_of(":"));

    constexpr auto kKeyAndDataSize = 2;
    if (kKeyAndDataSize != key_and_data.size()) {
      logger_->error("Invalid line in PB submission message '{}'", line);
      continue;
    }

    constexpr auto kKeyIndex = 0;
    constexpr auto kDataIndex = 1;
    auto& key = key_and_data[kKeyIndex];
    auto& data = key_and_data[kDataIndex];

    boost::algorithm::to_lower(key);
    boost::algorithm::trim(data);

    if (key.contains("categoria")) {
      if (!::GetCategory(data, category)) {
        logger_->error("Invalid category in PB submission message '{}'", line);
      }
      continue;
    }

    if (key.contains("plataforma")) {
      if (!::GetPlatform(data, platform)) {
        logger_->error("Invalid platform in PB submission message '{}'", line);
      }
      continue;
    }

    if (key.contains("controle")) {
      controller = data;
      continue;
    }

    if (key.contains("tempo")) {
      if (!::GetTime(data, time)) {
        logger_->error("Invalid time in PB submission message '{}'", line);
      }
      continue;
    }

    if (key.contains("vod")) {
      if (!::GetVod(data, vod)) {
        logger_->error("Invalid VOD in PB submission message '{}'", line);
      }
      continue;
    }
  }

  if (controller.empty()) {
    logger_->error("No controller specified in PB submission message");
  }

  // send dm
//  google_sheets_.AddPbToLeaderboard(user_id, username, );

  //bot_->message_delete(message_id, database_->GetChannelId(Database::Channels::kPbSubmission));
}