#pragma once

#include <memory>
#include <regex>
#include <string>
#include <vector>

#include <dpp/dpp.h>

#include "logger/logger_factory.h"

class MessageHandler final {
public:
  MessageHandler() = delete;
  ~MessageHandler() = default;

  MessageHandler(std::shared_ptr<dpp::cluster> bot) noexcept;

  void Process(dpp::message const& message) noexcept;
  void ProcessAnnouncementMessage(dpp::snowflake channel_id, std::string const& content) const noexcept;
  void ProcessGeneralMessage(dpp::snowflake channel_id, std::string const& content) const noexcept;
  void ProcessStreamingMessage(dpp::snowflake user_id, dpp::snowflake message_id, std::string const& content) noexcept;
  void ProcessAwardsMessage(dpp::snowflake user_id, dpp::snowflake message_id, std::string const& content, std::vector<dpp::attachment> const& attachments) noexcept;

private:
  void SendNominationMessage(dpp::snowflake const user_id, std::string const& clip_url) noexcept;

private:
  Logger const logger_ = LoggerFactory::Get().Create("Message Handler");

  std::shared_ptr<dpp::cluster> const bot_;

  std::regex const url_regex_ = std::regex("((http|https)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)");

  std::string nomination_content_header_;
};