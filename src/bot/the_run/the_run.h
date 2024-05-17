#pragma once

#include <dpp/dpp.h>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include "settings/settings.h"
#include "logger/logger.h"


class TheRun final {
public:
  TheRun() = delete;
  ~TheRun();

  TheRun(std::shared_ptr<Settings> settings, std::shared_ptr<dpp::cluster> bot) noexcept;

private:
  void OnOpen(websocketpp::client<websocketpp::config::asio_client> *const client, websocketpp::connection_hdl const handler);
  void OnClose(websocketpp::client<websocketpp::config::asio_client> *const client, websocketpp::connection_hdl const handler);
  void OnFail(websocketpp::client<websocketpp::config::asio_client> *const client, websocketpp::connection_hdl const handler);
  void OnMessage(websocketpp::connection_hdl const handler, websocketpp::client<websocketpp::config::asio_client>::message_ptr const message);

private:
  std::shared_ptr<spdlog::async_logger> const logger_ = Logger::Get().Create("The Run");

  std::shared_ptr<Settings> const settings_;
  std::shared_ptr<dpp::cluster> const bot_;

  websocketpp::client<websocketpp::config::asio_client> client_;
  websocketpp::lib::shared_ptr<websocketpp::lib::thread> thread_;
  websocketpp::connection_hdl connection_handler_;
};