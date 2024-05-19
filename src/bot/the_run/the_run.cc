#include "the_run.h"

#include <fmt/format.h>
#include <nlohmann/json.hpp>


namespace {
  struct SplitTime {
    std::chrono::milliseconds milliseconds;
    std::chrono::seconds seconds;
    std::chrono::minutes minutes;
    std::chrono::hours hours;
  };

  SplitTime MillisecondsToSplitTime(double const milliseconds) {
    SplitTime split_time;
    split_time.milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<double>(milliseconds));
    split_time.seconds = std::chrono::duration_cast<std::chrono::seconds>(split_time.milliseconds);
    split_time.milliseconds -= std::chrono::duration_cast<std::chrono::milliseconds>(split_time.seconds);
    split_time.minutes = std::chrono::duration_cast<std::chrono::minutes>(split_time.seconds);
    split_time.seconds -= std::chrono::duration_cast<std::chrono::seconds>(split_time.minutes);
    split_time.hours = std::chrono::duration_cast<std::chrono::hours>(split_time.minutes);
    split_time.minutes -= std::chrono::duration_cast<std::chrono::minutes>(split_time.hours);
    return split_time;
  }
}


TheRun::TheRun(std::shared_ptr<Settings> settings, std::shared_ptr<dpp::cluster> bot) noexcept :
  settings_(std::move(settings)), bot_(std::move(bot)) {
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
  }
  catch (std::exception const& exception) {
    logger_->error("Failed to set The Run SSL context. Error '{}'", exception.what());
  }

  return ssl_context;
}

void TheRun::Connect() noexcept {
  websocketpp::lib::error_code error_code;
  auto const connection = client_.get_connection(settings_->GetTheRunEndpoint(), error_code);
  if (error_code) {
    logger_->error("Failed to connect to The Run endpoint '{}'. Error '{}'", settings_->GetTheRunEndpoint(), error_code.message());
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
    logger_->error("Failed to disconnect to The Run endpoint '{}'. Error '{}'", settings_->GetTheRunEndpoint(), error_code.message());
  }
}

void TheRun::OnOpen(websocketpp::client<websocketpp::config::asio_tls_client> *const client, websocketpp::connection_hdl const connection_handle) noexcept {
  logger_->info("Connection to The Run endpoint opened");
}

void TheRun::OnClose(websocketpp::client<websocketpp::config::asio_tls_client> *const client, websocketpp::connection_hdl const connection_handle) noexcept {
  logger_->info("Connection to The Run endpoint closed");
}

void TheRun::OnFail(websocketpp::client<websocketpp::config::asio_tls_client> *const client, websocketpp::connection_hdl const connection_handle) noexcept {
  auto const connection = client->get_con_from_hdl(connection_handle);
  logger_->error("Connection to The Run endpoint failed. Error '{}'", connection->get_ec().message());
}

void TheRun::OnMessage(websocketpp::connection_hdl const handler, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr const message) noexcept {
  if (message->get_opcode() != websocketpp::frame::opcode::text) {
    return;
  }
 
  ProcessRunPayload(message->get_payload());
}

void TheRun::ProcessRunPayload(std::string const& run_payload) noexcept {
  auto const payload_json = nlohmann::json::parse(run_payload);

  auto const user = payload_json["user"].get<std::string>();

  auto const& run_data = payload_json["run"];
  auto const game = run_data["game"].get<std::string>();
  if (0 != game.rfind("Super Mario 64")) {
    return;
  }

  auto const run_percentage = run_data["runPercentage"].get<double>();
  if (run_percentage < 0.8) {
    return;
  }

  auto const pb = run_data["pb"].get<double>();
  auto const bpt = run_data["bestPossible"].get<double>();
  if (pb < bpt) {
    return;
  }

  auto const pb_split_time = ::MillisecondsToSplitTime(pb);
  auto const bpt_split_time = ::MillisecondsToSplitTime(bpt);

  auto const current_time = run_data["currentTime"].get<double>();
  auto const category = run_data["category"].get<std::string>();

  auto const pacepals_message = fmt::format("Runner: {}\nCategoria: {} - {}\nPB: {}\nBPT: {}\n", user, game, category, pb, bpt);
  bot_->message_create(dpp::message(settings_->GetChannelId(Settings::Channels::kGeneral), pacepals_message));
}
