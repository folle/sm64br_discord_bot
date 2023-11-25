#include "message_handler.h"


void MessageHandler::Process(const dpp::message_create_t& message_create) noexcept {
  //const auto is_bot = message_create.msg.author.is_bot();
  //const auto& message = message_create.msg;

  //bool is_moderator = false;
  //try {
  //  const auto message_guild_member = bot_.guild_get_member_sync(message.guild_id, message.author.id);
  //  is_moderator = std::any_of(message_guild_member.roles.begin(), message_guild_member.roles.end(), [](const auto& role) { return (role == kModeratorRoleId)}
  //}
  //catch (const dpp::rest_exception& rest_exception) {
  //  //TODO
  //}


  //// Announcement message
  //const auto is_announcement_message = message.content.substr(0, 3) == "!a ";

  //// General Message
  //const auto is_announcement_message = message.content.substr(0, 3) == "!m ";

  // PB Sheet channel

  // bot return


  // streaming channel

  // star of the week

  // beginner dw reds 
}