#pragma once

#include <dpp/dpp.h>
#include <websocketpp/config/core_client.hpp>
#include <websocketpp/client.hpp>

#include "settings/settings.h"
#include "logger/logger.h"


class TheRun final {
public:
  TheRun() = delete;
  ~TheRun();

  TheRun(std::shared_ptr<Settings> settings, std::shared_ptr<dpp::cluster> bot) noexcept;

private:
  std::shared_ptr<spdlog::async_logger> const logger_ = Logger::Get().Create("The Run");

  std::shared_ptr<Settings> const settings_;
  std::shared_ptr<dpp::cluster> const bot_;

  websocketpp::client<websocketpp::config::core_client> client_;
  websocketpp::lib::shared_ptr<websocketpp::lib::thread> thread_;
};