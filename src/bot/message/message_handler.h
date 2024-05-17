#pragma once

#include <dpp/dpp.h>

#include "settings/settings.h"
#include "logger/logger.h"


class MessageHandler final {
public:
  MessageHandler() = delete;
  ~MessageHandler() = default;

  MessageHandler(std::shared_ptr<Settings> settings, std::shared_ptr<dpp::cluster> bot) noexcept;

  void Process(dpp::message const& message) noexcept;
  void ProcessAnnouncementMessage(dpp::snowflake channel_id, std::string const& message) const noexcept;
  void ProcessGeneralMessage(dpp::snowflake channel_id, std::string const& message) const noexcept;
  void ProcessStreamingMessage(dpp::snowflake user_id, dpp::snowflake message_id, std::string const& message) noexcept;

private:
  std::shared_ptr<spdlog::async_logger> const logger_ = Logger::Get().Create("Message Handler");

  std::shared_ptr<Settings> const settings_;
  std::shared_ptr<dpp::cluster> const bot_;
};