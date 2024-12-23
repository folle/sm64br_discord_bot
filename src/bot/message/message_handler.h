#pragma once

#include <memory>
#include <string>

#include <dpp/dpp.h>

#include "logger/logger_factory.h"

class MessageHandler final {
public:
  MessageHandler() = delete;
  ~MessageHandler() = default;

  MessageHandler(std::shared_ptr<dpp::cluster> bot) noexcept;

  void Process(dpp::message const& message) noexcept;
  void ProcessAnnouncementMessage(dpp::snowflake channel_id, std::string const& message) const noexcept;
  void ProcessGeneralMessage(dpp::snowflake channel_id, std::string const& message) const noexcept;
  void ProcessStreamingMessage(dpp::snowflake user_id, dpp::snowflake message_id, std::string const& message) noexcept;
  void ProcessAwardsMessage(dpp::snowflake user_id, dpp::snowflake message_id, std::string const& message) noexcept;

private:
  Logger const logger_ = LoggerFactory::Get().Create("Message Handler");

  std::shared_ptr<dpp::cluster> const bot_;
};