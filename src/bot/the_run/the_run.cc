#include "the_run.h"

#include <exception>
#include <print>
#include <utility>

#include "payload_parser.h"
#include "settings/settings.h"

TheRun::TheRun(std::shared_ptr<dpp::cluster> bot) noexcept :
  bot_(std::move(bot)) {
  client_.clear_access_channels(websocketpp::log::alevel::all);
  client_.clear_error_channels(websocketpp::log::elevel::all);

  client_.init_asio();

  client_.set_tls_init_handler(websocketpp::lib::bind(&TheRun::OnTlsInit, this));

  client_.start_perpetual();

  thread_ = websocketpp::lib::make_shared<websocketpp::lib::thread>(&websocketpp::client<websocketpp::config::asio_tls_client>::run, &client_);

  Connect();
}

TheRun::~TheRun() {
  client_.stop_perpetual();

  thread_->join();
}

std::shared_ptr<boost::asio::ssl::context> TheRun::OnTlsInit() noexcept {
  auto const ssl_context = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
  try {
    ssl_context->set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::no_sslv3 | boost::asio::ssl::context::single_dh_use);
  } catch (std::exception const& exception) {
    logger_.Error("Failed to set The Run SSL context. Error '{}'", exception.what());
  }

  return ssl_context;
}

void TheRun::Connect() noexcept {
  websocketpp::lib::error_code error_code;
  auto const connection = client_.get_connection(Settings::Get().GetTheRunEndpoint(), error_code);
  if (error_code) {
    logger_.Error("Failed to connect to The Run endpoint '{}'. Error '{}'", Settings::Get().GetTheRunEndpoint(), error_code.message());
    return;
  }

  connection_handle_ = connection->get_handle();

  connection->set_open_handler(websocketpp::lib::bind(
    &TheRun::OnOpen,
    this,
    &client_,
    websocketpp::lib::placeholders::_1
  ));
  connection->set_close_handler(websocketpp::lib::bind(
    &TheRun::OnClose,
    this,
    &client_,
    websocketpp::lib::placeholders::_1
  ));
  connection->set_fail_handler(websocketpp::lib::bind(
    &TheRun::OnFail,
    this,
    &client_,
    websocketpp::lib::placeholders::_1
  ));
  connection->set_message_handler(websocketpp::lib::bind(
    &TheRun::OnMessage,
    this,
    websocketpp::lib::placeholders::_1,
    websocketpp::lib::placeholders::_2
  ));

  client_.connect(connection);
}

void TheRun::Disconnect() noexcept {
  websocketpp::lib::error_code error_code;
  client_.close(connection_handle_, websocketpp::close::status::going_away, "", error_code);
  if (error_code) {
    logger_.Error("Failed to disconnect to The Run endpoint '{}'. Error '{}'", Settings::Get().GetTheRunEndpoint(), error_code.message());
  }
}

void TheRun::OnOpen(websocketpp::client<websocketpp::config::asio_tls_client> *const client, websocketpp::connection_hdl const connection_handle) noexcept {
  logger_.Info("Connection to The Run endpoint opened");
}

void TheRun::OnClose(websocketpp::client<websocketpp::config::asio_tls_client> *const client, websocketpp::connection_hdl const connection_handle) noexcept {
  logger_.Info("Connection to The Run endpoint closed");
}

void TheRun::OnFail(websocketpp::client<websocketpp::config::asio_tls_client> *const client, websocketpp::connection_hdl const connection_handle) noexcept {
  auto const connection = client->get_con_from_hdl(connection_handle);
  logger_.Error("Connection to The Run endpoint failed. Error '{}'", connection->get_ec().message());
}

void TheRun::OnMessage(websocketpp::connection_hdl const handler, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr const message) noexcept {
  if (message->get_opcode() != websocketpp::frame::opcode::text) {
    return;
  }
 
  auto const payload_parser = PayloadParser(message->get_payload());
  if (!payload_parser.IsPingable()) {
    announced_users_.erase(payload_parser.GetUser());
    return;
  }

  if (announced_users_.contains(payload_parser.GetUser())) {
    return;
  }

  logger_.Info("Payload triggered a ping '{}'", message->get_payload());

  auto const pacepals_message = std::format("{}\n{}", dpp::role::get_mention(Settings::Get().GetRoleId(Settings::Roles::kPacepals)),  payload_parser.GetString());
  bot_->message_create(dpp::message(Settings::Get().GetChannelId(Settings::Channels::kGeneral), pacepals_message));
  announced_users_.insert(payload_parser.GetUser());
}