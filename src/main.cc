#include <cstdlib>
#include <exception>
#include <iostream>

#include "bot/sm64br_discord_bot.h"

int main(const int argc, char const *const *const argv) {
  try {
    Sm64brDiscordBot bot;
    bot.Start();
  } catch (std::exception const& exception) {
    std::cerr << exception.what();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}