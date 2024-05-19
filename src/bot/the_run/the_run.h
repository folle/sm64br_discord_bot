#pragma once

#include <chrono>
#include <set>
#include <dpp/dpp.h>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include "settings/settings.h"
#include "logger/logger.h"


class TheRun final {
public:
  TheRun() = delete;
  ~TheRun();

  TheRun(std::shared_ptr<Settings> settings, std::shared_ptr<dpp::cluster> bot) noexcept;

private:
  void Connect() noexcept;
  void Disconnect() noexcept;

  std::shared_ptr<boost::asio::ssl::context> OnTlsInit() noexcept;

  void OnOpen(websocketpp::client<websocketpp::config::asio_tls_client> *const client, websocketpp::connection_hdl const connection_handle) noexcept;
  void OnClose(websocketpp::client<websocketpp::config::asio_tls_client> *const client, websocketpp::connection_hdl const connection_handle) noexcept;
  void OnFail(websocketpp::client<websocketpp::config::asio_tls_client> *const client, websocketpp::connection_hdl const connection_handle) noexcept;
  void OnMessage(websocketpp::connection_hdl const handler, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr const message) noexcept;

  void ProcessRunPayload(std::string const& run_payload) noexcept;

private:
  std::shared_ptr<spdlog::async_logger> const logger_ = Logger::Get().Create("The Run");

  std::shared_ptr<Settings> const settings_;
  std::shared_ptr<dpp::cluster> const bot_;

  websocketpp::client<websocketpp::config::asio_tls_client> client_;
  websocketpp::lib::shared_ptr<websocketpp::lib::thread> thread_;
  websocketpp::connection_hdl connection_handle_;

  std::set<std::string> announced_users_;
};