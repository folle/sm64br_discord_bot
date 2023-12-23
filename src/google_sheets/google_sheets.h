#pragma once

#include "database/database.h"
#include "logger/logger.h"


class GoogleSheets final {
public:
  GoogleSheets() = delete;
  ~GoogleSheets() = default;

  GoogleSheets(std::shared_ptr<Database> database);

  bool AddPbToLeaderboard();

private:
#ifdef _WIN32
  [[nodiscard]] bool GetBearerAccessToken(std::wstring& access_token) const noexcept;
#else
  [[nodiscard]] bool GetBearerAccessToken(std::string& access_token) const noexcept;
#endif

private:
  const std::shared_ptr<spdlog::async_logger> logger_ = Logger::Get().Create("Google Sheets");

  const std::shared_ptr<Database> database_;

  std::mutex pb_leaderboard_mutex_;
};