#include <exception>
#include <iostream>

#include "bot/sm64br_discord_bot.h"


int main(int argc, char** argv) {
  try {
    Sm64BrDiscordBot bot;
    bot.Start();
  }
  catch (const std::exception& exception) {
    std::cerr << exception.what();
    return -1;
  }

  return 0;
}