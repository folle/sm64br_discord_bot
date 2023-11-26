#pragma once


#include "message/message_handler.h"


class Sm64brDiscordBot final {
public:
  Sm64brDiscordBot();
  ~Sm64brDiscordBot();

  void Start() const noexcept;

private:
  void OnLog(const dpp::log_t& event) const noexcept;
  void OnMessageCreate(const dpp::message_create_t& message_create) noexcept;
  void OnPresenceUpdate(const dpp::presence_update_t& presence_update) noexcept;
  void OnReady(const dpp::ready_t& ready) const noexcept;
  void OnGuildMemberAdd(const dpp::guild_member_add_t& guild_member_add) const noexcept;
  void OnGuildMemberRemove(const dpp::guild_member_remove_t& guild_member_remove) const noexcept;

  void ClearStreamingStatus() const noexcept;

private:
  const std::shared_ptr<spdlog::async_logger> logger_ = Logger::Get().Create("SM64BR Discord Bot");

  const std::shared_ptr<Database> database_ = std::make_shared<Database>("database/discord_server.json");

  const std:: shared_ptr<dpp::cluster> bot_ = std::make_shared<dpp::cluster>(database_->GetBotToken(), dpp::i_all_intents);
  
  MessageHandler message_handler_ = MessageHandler(database_, bot_);

  std::map<dpp::snowflake, dpp::snowflake> streaming_users_ids_and_messages_ids_;
  std::mutex on_presence_update_mutex_;

  std::list<std::future<void>> message_create_futures_;
  std::list<std::future<void>> presence_update_futures_;
};