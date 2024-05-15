#include <exception>
#include <iostream>

#include "bot/sm64br_discord_bot.h"


int main(int argc, char** argv) {
  try {
    Sm64brDiscordBot bot;
    bot.Start();
  }
  catch (std::exception const& exception) {
    std::cerr << exception.what();
    return -1;
  }

  return 0;
}