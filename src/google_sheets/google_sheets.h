#pragma once

#include <fmt/format.h>
#include <cpprest/http_client.h>

#include "logger/logger.h"


class GoogleSheets final {
public:
  GoogleSheets() = delete;
  ~GoogleSheets() = default;

  GoogleSheets(const std::string& client_secret_file_path);

private:
  const std::shared_ptr<spdlog::async_logger> logger_ = Logger::Get().Create("Google Sheets");

  web::http::client::http_client_config http_config_;
  std::unique_ptr<web::http::oauth2::experimental::oauth2_config> oauth2_config_;
};