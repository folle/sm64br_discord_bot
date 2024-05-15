#pragma once

#include "message/message_handler.h"


class Sm64brDiscordBot final {
public:
  Sm64brDiscordBot();
  ~Sm64brDiscordBot();

  void Start() const noexcept;

private:
  void OnLog(dpp::log_t const& event) const noexcept;
  void OnMessageCreate(dpp::message_create_t const& message_create) noexcept;
  void OnPresenceUpdate(dpp::presence_update_t const& presence_update) noexcept;
  void OnReady(dpp::ready_t const& ready) const noexcept;
  void OnGuildMemberAdd(dpp::guild_member_add_t const& guild_member_add) const noexcept;
  void OnGuildMemberRemove(dpp::guild_member_remove_t const& guild_member_remove) const noexcept;

  void ClearStreamingStatus() const noexcept;

private:
  std::shared_ptr<spdlog::async_logger> const logger_ = Logger::Get().Create("SM64BR Discord Bot");

  std::shared_ptr<Database> const database_ = std::make_shared<Database>("database/database.json");

  std:: shared_ptr<dpp::cluster> const bot_ = std::make_shared<dpp::cluster>(database_->GetBotToken(), dpp::i_all_intents);
  
  MessageHandler message_handler_ = MessageHandler(database_, bot_);

  std::map<dpp::snowflake, dpp::snowflake> streaming_users_ids_and_messages_ids_;
  std::mutex on_presence_update_mutex_;

  std::list<std::future<void>> message_create_futures_;
  std::list<std::future<void>> presence_update_futures_;
};