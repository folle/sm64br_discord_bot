#include "payload_parser.h"

#include <chrono>
#include <cstdlib>
#include <ranges>
#include <fmt/format.h>


namespace {
  struct SplitTime {
    std::chrono::milliseconds milliseconds;
    std::chrono::seconds seconds;
    std::chrono::minutes minutes;
    std::chrono::hours hours;
    bool positive = true;
  };

  SplitTime SplitMillisecondsToSplitTime(long long const split_milliseconds) {
    SplitTime split_time{};
    split_time.milliseconds = std::chrono::milliseconds(std::abs(split_milliseconds));
    split_time.seconds = std::chrono::duration_cast<std::chrono::seconds>(split_time.milliseconds);
    split_time.milliseconds -= std::chrono::duration_cast<std::chrono::milliseconds>(split_time.seconds);
    split_time.minutes = std::chrono::duration_cast<std::chrono::minutes>(split_time.seconds);
    split_time.seconds -= std::chrono::duration_cast<std::chrono::seconds>(split_time.minutes);
    split_time.hours = std::chrono::duration_cast<std::chrono::hours>(split_time.minutes);
    split_time.minutes -= std::chrono::duration_cast<std::chrono::minutes>(split_time.hours);
    split_time.positive = 0 <= split_milliseconds;
    return split_time;
  }

  std::string SplitTimeToString(SplitTime const& split_time, bool const include_signal) {
    auto get_signal_string = [&include_signal](bool const positive) {
      return include_signal ? (positive ? "+" : "-") : "";
    };

    if (0 != split_time.hours.count()) {
      return fmt::format("{}{:02}:{:02}:{:02}.{:03}",
                         get_signal_string(split_time.positive),
                         split_time.hours, split_time.minutes, split_time.seconds, split_time.milliseconds);
    }

    if (0 != split_time.minutes.count()) {
      return fmt::format("{}{:02}:{:02}.{:03}",
                         get_signal_string(split_time.positive),
                         split_time.minutes, split_time.seconds, split_time.milliseconds);
    }

    if (0 != split_time.seconds.count()) {
      return fmt::format("{}{:02}.{:03}", get_signal_string(split_time.positive), split_time.seconds, split_time.milliseconds);
    }

    return fmt::format("{}0.{:03}", get_signal_string(split_time.positive), split_time.milliseconds);
  }

  std::string SplitMillisecondsToString(long long const split_milliseconds, bool const include_signal) {
    auto const split_time = SplitMillisecondsToSplitTime(split_milliseconds);
    return SplitTimeToString(split_time, include_signal);
  }
}


PayloadParser::PayloadParser(std::string const& payload) noexcept {
  Parse(payload);
}

void PayloadParser::Parse(std::string const& payload) noexcept {
 try {
    auto const payload_json = nlohmann::json::parse(payload);

    user_ = payload_json["user"].get<std::string>();

    auto const& run_data = payload_json["run"];
    if (!ParseRunData(run_data)) {
      return;
    }

    auto const& splits_data = run_data["splits"];
    ParseSplitsData(splits_data);
  }
  catch (std::exception const& exception) {
    logger_->error("Failed to parse The Run payload '{}'. Error '{}'", payload, exception.what());
    return;
  }

  successfully_parsed_ = true;
}

bool PayloadParser::ParseRunData(nlohmann::json const& run_data) {
  auto const game = run_data["game"].get<std::string>();
  if (0 != game.rfind("Super Mario 64")) {
    return false;
  }

  auto const run_percentage = run_data["runPercentage"].get<double>();
  if (run_percentage < 0.85) {
    return false;
  }

  above_threshold_ = true;

  streaming_ = run_data["currentlyStreaming"].get<bool>();

  auto const pb_milliseconds = run_data["pb"].get<long long>();
  auto const bpt_milliseconds = run_data["bestPossible"].get<long long>();
  if (pb_milliseconds < bpt_milliseconds) {
    return false;
  }

  pacing_ = true;

  pb_ = ::SplitMillisecondsToString(pb_milliseconds, false);
  bpt_ = ::SplitMillisecondsToString(bpt_milliseconds, false);

  category_ = run_data["category"].get<std::string>();

  emulator_ = run_data["emulator"].get<bool>();

  auto const& game_data = run_data["gameData"];

  attempt_count_ = game_data["attemptCount"].get<size_t>();

  url_ = fmt::format("https://therun.gg/{}", game_data["url"].get<std::string>());

  return true;
}

void PayloadParser::ParseSplitsData(nlohmann::json const& splits_data) {
  for (auto const& split_data : splits_data) {
    auto const split_index = static_cast<size_t>(std::stoll(split_data["index"].get<std::string>()));

    SplitInfo split_info{};
    split_info.name = split_data["name"].get<std::string>();

    try {
      auto const split_time = split_data["splitTime"].get<long long>();
      split_info.time = SplitMillisecondsToString(split_time, false);

      auto const split_pb = split_data["pbSplitTime"].get<long long>();
      split_info.pb_difference = SplitMillisecondsToString(split_time - split_pb, true);
    } catch (std::exception const& exception) {
      // Do nothing
    }

    splits_[split_index] = split_info;
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

bool PayloadParser::IsStreaming() const noexcept {
  return streaming_;
}

bool PayloadParser::IsSuccessfullyParsed() const noexcept {
  return successfully_parsed_;
}

std::string const& PayloadParser::GetUser() const noexcept {
  return user_;
}

std::string PayloadParser::GetString() const noexcept {
  constexpr size_t kDiscordMaximumMessageSize = 2000;

  std::string run_info;
  run_info.reserve(kDiscordMaximumMessageSize);

  run_info.append("```");

  size_t biggest_split_name_length{};
  size_t biggest_split_pb_difference_length{};
  size_t biggest_split_time_length{};
  std::ranges::for_each(splits_, [&biggest_split_name_length, &biggest_split_pb_difference_length, &biggest_split_time_length](auto const& split) {
    auto const& split_info = split.second;

    if (split_info.name.size() > biggest_split_name_length) {
      biggest_split_name_length = split_info.name.size();
    }

    if (split_info.pb_difference.size() > biggest_split_pb_difference_length) {
      biggest_split_pb_difference_length = split_info.pb_difference.size();
    }

    if (split_info.time.size() > biggest_split_time_length) {
      biggest_split_time_length = split_info.time.size();
    }
  });

  std::ranges::for_each(splits_, [&run_info, &biggest_split_name_length, &biggest_split_pb_difference_length, &biggest_split_time_length](auto const& split) {
    constexpr size_t kFieldSpacing = 4;

    auto const& split_info = split.second;
    run_info.append(fmt::format("\n{:<{}}", split_info.name, biggest_split_name_length - split_info.name.size() + kFieldSpacing));
    run_info.append(fmt::format("{:>{}}", split_info.pb_difference, biggest_split_pb_difference_length - split_info.pb_difference.size()));
    run_info.append(fmt::format("{:>{}}", split_info.time, biggest_split_time_length - split_info.time.size() + kFieldSpacing));
  });

  run_info.append("```");

  return run_info;
}