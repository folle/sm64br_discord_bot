#pragma once

#include <dpp/dpp.h>

#include "database/database.h"
#include "google_sheets/google_sheets.h"
#include "logger/logger.h"


class MessageHandler final {
public:
  MessageHandler() = delete;
  ~MessageHandler() = default;

  MessageHandler(std::shared_ptr<Database> database, std::shared_ptr<dpp::cluster> bot) noexcept;

  void Process(const dpp::message& message) noexcept;
  void ProcessAnnouncementMessage(dpp::snowflake channel_id, const std::string& message) const noexcept;
  void ProcessGeneralMessage(dpp::snowflake channel_id, const std::string& message) const noexcept;
  void ProcessStreamingMessage(dpp::snowflake user_id, dpp::snowflake message_id, const std::string& message) noexcept;
  void ProcessPbSubmissionMessage(dpp::snowflake user_id, const std::string& username, dpp::snowflake message_id, const std::string& message) const noexcept;

private:
  const std::shared_ptr<spdlog::async_logger> logger_ = Logger::Get().Create("Message Handler");

  const std::shared_ptr<Database> database_;
  const std::shared_ptr<dpp::cluster> bot_;

  GoogleSheets google_sheets_= GoogleSheets(database_);
};