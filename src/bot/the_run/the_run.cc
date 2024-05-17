#include "the_run.h"


TheRun::TheRun(std::shared_ptr<Settings> settings, std::shared_ptr<dpp::cluster> bot) noexcept :
  settings_s(std::move(settings)), bot_(std::move(bot)) {
  client_.clear_access_channels(websocketpp::log::alevel::all);
  client_.clear_error_channels(websocketpp::log::elevel::all);

  thread_ = websocketpp::lib::make_shared<websocketpp::lib::thread>(&websocketpp::config::core_client::run, &client_);
}

TheRun::~TheRun() {
  client_.
}