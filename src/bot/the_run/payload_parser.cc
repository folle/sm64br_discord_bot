#include "payload_parser.h"

#include <chrono>
#include <fmt/format.h>
#include <nlohmann/json.hpp>


namespace {
  std::string SplitMillisecondsToString(long long const split_milliseconds) {
    auto milliseconds = std::chrono::milliseconds(split_milliseconds);
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(milliseconds);
    milliseconds -= std::chrono::duration_cast<std::chrono::milliseconds>(seconds);
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(seconds);
    seconds -= std::chrono::duration_cast<std::chrono::seconds>(minutes);
    auto const hours = std::chrono::duration_cast<std::chrono::hours>(minutes);
    minutes -= std::chrono::duration_cast<std::chrono::minutes>(hours);

    if (0 != hours.count()) {
      return fmt::format("{:02}:{:02}:{:02}.{:03}", hours, minutes, seconds, milliseconds);
    }

    if (0 != minutes.count()) {
      return fmt::format("{:02}:{:02}.{:03}", minutes, seconds, milliseconds);
    }

    if (0 != seconds.count()) {
      return fmt::format("{:02}.{:03}", seconds, milliseconds);
    }

    return fmt::format("0.{:03}", milliseconds);
  }
}


PayloadParser::PayloadParser(std::string const& payload) noexcept {
  Process(payload);
}

void PayloadParser::Process(std::string const& payload) noexcept {
 try {
    auto const payload_json = nlohmann::json::parse(payload);

    user_ = payload_json["user"].get<std::string>();

    auto const& run_data = payload_json["run"];

    auto const game = run_data["game"].get<std::string>();
    if (0 != game.rfind("Super Mario 64")) {
      return;
    }

    auto const run_percentage = run_data["runPercentage"].get<double>();
    if (run_percentage < 0.85) {
      return;
    }

    above_threshold_ = true;

    auto const pb_milliseconds = run_data["pb"].get<long long>();
    auto const bpt_milliseconds = run_data["bestPossible"].get<long long>();
    if (pb_milliseconds < bpt_milliseconds) {
      return;
    }

    pacing_ = true;

    //auto const current_time = run_data["currentTime"].get<double>();
    category_ = run_data["category"].get<std::string>();

    pb_ = ::SplitMillisecondsToString(pb_milliseconds);
    bpt_ = ::SplitMillisecondsToString(bpt_milliseconds);
  }
  catch (std::exception const& exception) {
    logger_->error("Failed to parse The Run payload '{}'. Error '{}'", payload, exception.what());
    return;
  }
}

bool PayloadParser::IsValidGame() const noexcept {
  return valid_game_;
}

bool PayloadParser::IsValidCategory() const noexcept {
  return valid_category_;
}

bool PayloadParser::IsAboveThreshold() const noexcept {
  return above_threshold_;
}

bool PayloadParser::IsPacing() const noexcept {
  return pacing_;
}

std::string const& PayloadParser::GetUser() const noexcept {
  return user_;
}

std::string PayloadParser::GetString() const noexcept {
  return {};
}


// {
//     "user": "Simply",
//     "run": {
//         "sob": 5658528.517,
//         "runPercentage": 0,
//         "currentTime": 1360.1797000000001,
//         "emulator": false,
//         "game": "Super Mario 64",
//         "bestPossible": 5658528.517,
//         "currentlyStreaming": true,
//         "pb": 5832271.292,
//         "category": "120 Star",
//         "user": "Simply",
//         "gameData": {
//              "attemptCount": 6806,
//              "url": "Simply/Super%20Mario%2064/120%20Star",
//         },
//         "splits": [
//             {
//                 "pbSplitTime": 169160,
//                 "bestPossible": 164300,
//                 "name": "DW (1)",
//                 "splitTime": null,
//                 "index": "0"
//             }
//         ]
//     },
// }