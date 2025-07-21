#pragma once

#include <future>
#include <list>
#include <map>
#include <memory>
#include <mutex>

#include <dpp/dpp.h>

#include "logger/logger_factory.h"
#include "message/message_handler.h"
#include "settings/settings.h"
//#include "the_run/the_run.h"

class Sm64brDiscordBot final {
public:
  Sm64brDiscordBot();
  ~Sm64brDiscordBot();

  void Start() const noexcept;

private:
  void OnLog(dpp::log_t const& log) const noexcept;
  void OnMessageCreate(dpp::message_create_t const& message_create) noexcept;
  void OnMessageReactionAdd(dpp::message_reaction_add_t const& message_reaction_add) noexcept;
  void OnPresenceUpdate(dpp::presence_update_t const& presence_update) noexcept;
  void OnReady(dpp::ready_t const& ready) const noexcept;
  void OnGuildMemberAdd(dpp::guild_member_add_t const& guild_member_add) const noexcept;
  void OnGuildMemberRemove(dpp::guild_member_remove_t const& guild_member_remove) const noexcept;

  void ClearStreamingRoles() const;
  void ClearStreamingMessages() const;

private:
  Logger const logger_ = LoggerFactory::Get().Create("SM64BR Discord Bot");

  std:: shared_ptr<dpp::cluster> const bot_ = std::make_shared<dpp::cluster>(Settings::Get().GetBotToken(), dpp::i_all_intents);
  
  MessageHandler message_handler_ = MessageHandler(bot_);

  //TheRun the_run = TheRun(bot_);

  std::map<dpp::snowflake, dpp::snowflake> streaming_users_ids_and_messages_ids_;
  std::mutex on_presence_update_mutex_;

  std::list<std::future<void>> message_create_futures_;
  std::list<std::future<void>> message_reaction_futures_;
  std::list<std::future<void>> presence_update_futures_;
};