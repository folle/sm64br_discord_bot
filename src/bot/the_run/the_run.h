#pragma once

#include <memory>
#include <set>
#include <string>

#include <dpp/dpp.h>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

#include "logger/logger_factory.h"

class TheRun final {
public:
  TheRun() = delete;
  ~TheRun();

  TheRun(std::shared_ptr<dpp::cluster> bot) noexcept;

private:
  void Connect() noexcept;
  void Disconnect() noexcept;

  std::shared_ptr<boost::asio::ssl::context> OnTlsInit() noexcept;

  void OnOpen(websocketpp::client<websocketpp::config::asio_tls_client> *const client, websocketpp::connection_hdl const connection_handle) noexcept;
  void OnClose(websocketpp::client<websocketpp::config::asio_tls_client> *const client, websocketpp::connection_hdl const connection_handle) noexcept;
  void OnFail(websocketpp::client<websocketpp::config::asio_tls_client> *const client, websocketpp::connection_hdl const connection_handle) noexcept;
  void OnMessage(websocketpp::connection_hdl const handler, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr const message) noexcept;

private:
  Logger const logger_ = LoggerFactory::Get().Create("The Run");

  std::shared_ptr<dpp::cluster> const bot_;

  websocketpp::client<websocketpp::config::asio_tls_client> client_;
  websocketpp::lib::shared_ptr<websocketpp::lib::thread> thread_;
  websocketpp::connection_hdl connection_handle_;

  std::set<std::string> announced_users_;
};