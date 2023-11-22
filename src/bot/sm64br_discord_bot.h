#pragma once

#include <dpp/dpp.h>
#include <future>
#include <memory>
#include <mutex>

#include "database/server_database.h"
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
  dpp::cluster bot_ = dpp::cluster(BOT_TOKEN, dpp::i_default_intents | dpp::i_guild_members | dpp::i_guild_presences | dpp::i_message_content);
  
  MessageHandler message_handler_;

  const std::shared_ptr<spdlog::async_logger> logger_ = Logger::Get().Create("SM64BR Discord Bot");

  std::vector<dpp::snowflake> streaming_users_ids_;
  std::mutex on_presence_update_mutex_;

  std::vector<std::future<void>> message_create_futures_;
  std::vector<std::future<void>> presence_update_futures_;

  const ServerDatabase server_database_;
};