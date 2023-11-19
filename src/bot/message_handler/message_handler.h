#pragma once

#include <dpp/dpp.h>

#include "logger/logger.h"


class MessageHandler final {
public:
  MessageHandler() = default;
  ~MessageHandler() = default;

  void Process(const dpp::message_create_t& message_create);

private:
  const std::shared_ptr<spdlog::async_logger> logger_ = Logger::Get().Create("Message Handler");
};