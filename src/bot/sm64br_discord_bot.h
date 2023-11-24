#pragma once

#include <dpp/dpp.h>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "database/database.h"
#include "message_handler/message_handler.h"
#include "logger/logger.h"


class Sm64brDiscordBot final {
public:
  Sm64brDiscordBot();
  ~Sm64brDiscordBot();

  void Start() noexcept;

private:
  void OnLog(const dpp::log_t& event) const noexcept;
  void OnMessageCreate(const dpp::message_create_t& message_create) noexcept;
  void OnPresenceUpdate(const dpp::presence_update_t& presence_update) noexcept;
  void OnReady(const dpp::ready_t& ready);
  void OnGuildMemberAdd(const dpp::guild_member_add_t& guild_member_add) noexcept;
  void OnGuildMemberRemove(const dpp::guild_member_remove_t& guild_member_remove) noexcept;

  void ClearStreamingStatus() noexcept;

private:
  const std::shared_ptr<spdlog::async_logger> logger_ = Logger::Get().Create("SM64BR Discord Bot");

  const Database database_ = Database("database/database.json");

  dpp::cluster bot_ = dpp::cluster(database_.GetBotToken(), dpp::i_all_intents);
  
  MessageHandler message_handler_;

  std::map<dpp::snowflake, dpp::snowflake> streaming_users_ids_and_messages_ids_;
  std::mutex on_presence_update_mutex_;

  std::vector<std::future<void>> message_create_futures_;
  std::vector<std::future<void>> presence_update_futures_;
};