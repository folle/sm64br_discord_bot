#include "the_run.h"


TheRun::TheRun(std::shared_ptr<Database> database, std::shared_ptr<dpp::cluster> bot) noexcept :
  database_(std::move(database)), bot_(std::move(bot)) {
  client_.clear_access_channels(websocketpp::log::alevel::all);
  client_.clear_error_channels(websocketpp::log::elevel::all);

  thread_ = websocketpp::lib::make_shared<websocketpp::lib::thread>(&websocketpp::config::core_client::run, &client_);
}

TheRun::~TheRun() {
  client_.
}