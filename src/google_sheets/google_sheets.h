#pragma once

#include "database/database.h"
#include "logger/logger.h"


class GoogleSheets final {
public:
  GoogleSheets() = delete;
  ~GoogleSheets() = default;

  GoogleSheets(std::shared_ptr<Database> database);

private:
  [[nodiscard]] bool GetBearerAccessToken(std::wstring& access_token) const noexcept;

private:
  const std::shared_ptr<spdlog::async_logger> logger_ = Logger::Get().Create("Google Sheets");

  const std::shared_ptr<Database> database_;
};