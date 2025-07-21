#include "payload_parser.h"

#include <chrono>
#include <cstdlib>
#include <exception>
#include <print>
#include <ranges>

#include "settings/settings.h"

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
      return std::format("{}{}:{:02}:{:02}.{:03}",
                         get_signal_string(split_time.positive),
                         split_time.hours.count(), split_time.minutes.count(), split_time.seconds.count(), split_time.milliseconds.count());
    }

    if (0 != split_time.minutes.count()) {
      return std::format("{}{}:{:02}.{:03}",
                         get_signal_string(split_time.positive),
                         split_time.minutes.count(), split_time.seconds.count(), split_time.milliseconds.count());
    }

    if (0 != split_time.seconds.count()) {
      return std::format("{}{}.{:03}", get_signal_string(split_time.positive), split_time.seconds.count(), split_time.milliseconds.count());
    }

    return std::format("{}0.{:03}", get_signal_string(split_time.positive), split_time.milliseconds.count());
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
    if (!ParseSplitsData(splits_data)) {
      return;
    }
  } catch (std::exception const& exception) {
    logger_.Error("Failed to parse The Run payload '{}'. Error '{}'", payload, exception.what());
    return;
  }

  successfully_parsed_ = true;
}

bool PayloadParser::ParseRunData(nlohmann::json const& run_data) {
  auto const game = run_data["game"].get<std::string>();
  if (0 != game.rfind("Super Mario 64")) {
    return false;
  }

  if (!run_data["currentlyStreaming"].get<bool>()) {
    return false;
  }

  Settings::Categories category{};
  category_ = run_data["category"].get<std::string>();
  if (0 == category_.rfind("120 Star")) {
    category = Settings::Categories::k120Star;
  } else if (0 == category_.rfind("70 Star")) {
    category = Settings::Categories::k70Star;
  } else if (0 == category_.rfind("16 Star")) {
    category = Settings::Categories::k16Star;
  } else if (0 == category_.rfind("1 Star")) {
    category = Settings::Categories::k1Star;
  } else if (0 == category_.rfind("0 Star")) {
    category = Settings::Categories::k0Star;
  } else {
    return false;
  }

  auto const thresholds = Settings::Get().GetTheRunThresholds(category);

  if (run_data["runPercentage"].get<double>() < thresholds.percentage) {
    return false;
  }

  auto const bpt_milliseconds = run_data["bestPossible"].get<long long>();
  if (bpt_milliseconds > thresholds.bpt) {
    return false;
  }
  
  auto const pb_milliseconds = run_data["pb"].get<long long>();
  if (pb_milliseconds < bpt_milliseconds) {
    return false;
  }

  pb_ = ::SplitMillisecondsToString(pb_milliseconds, false);
  bpt_ = ::SplitMillisecondsToString(bpt_milliseconds, false);
  sob_ = ::SplitMillisecondsToString(run_data["sob"].get<long long>(), false);

  emulator_ = run_data["emulator"].get<bool>();

  auto const& game_data = run_data["gameData"];

  attempt_count_ = game_data["attemptCount"].get<std::size_t>();

  url_ = std::format("https://therun.gg/{}", game_data["url"].get<std::string>());

  return true;
}

bool PayloadParser::ParseSplitsData(nlohmann::json const& splits_data) {
  for (auto const& split_data : splits_data) {
    auto const split_index = static_cast<std::size_t>(std::stoll(split_data["index"].get<std::string>()));

    SplitInfo split_info{};
    split_info.name = split_data["name"].get<std::string>();

    try {
      auto const split_time = split_data["splitTime"].get<long long>();
      split_info.time = SplitMillisecondsToString(split_time, false);

      auto const split_pb = split_data["pbSplitTime"].get<long long>();
      split_info.pb_difference = SplitMillisecondsToString(split_time - split_pb, true);
    } catch (std::exception const& exception) {
      break;
    }

    splits_[split_index] = std::move(split_info);
  }

  return !splits_.empty();
}

bool PayloadParser::IsPingable() const noexcept {
  return successfully_parsed_;
}

std::string const& PayloadParser::GetUser() const noexcept {
  return user_;
}

std::string PayloadParser::GetString() const noexcept {
  constexpr std::size_t kDiscordMaximumMessageSize = 2000;
  constexpr std::size_t kFieldSpacing = 4;

  std::string run_info;
  run_info.reserve(kDiscordMaximumMessageSize);

  run_info.append(std::format("**Runner: {}**\nCategoria: {}\nPlataforma: {}\nPB: {}\nBPT: {}\nSOB: {}\nTentativa: {}\n{}\n", 
                              user_, category_, emulator_ ? "Emulador" : "Console", pb_, bpt_, sob_, attempt_count_, url_));

  std::size_t biggest_split_name_length{};
  std::size_t biggest_split_pb_difference_length{};
  std::size_t biggest_split_time_length{};
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

  run_info.append("```");

  std::ranges::for_each(splits_, [&run_info, &biggest_split_name_length, &biggest_split_pb_difference_length, &biggest_split_time_length](auto const& split) {
    auto const& split_info = split.second;
    if (split_info.pb_difference.empty()) {
      return;
    }
    
    run_info.append(std::format("{}", split_info.name));
    run_info.append(std::format("{:<{}}", "", biggest_split_name_length - split_info.name.size() + kFieldSpacing));
    run_info.append(std::format("{:<{}}", "", biggest_split_pb_difference_length - split_info.pb_difference.size()));
    run_info.append(std::format("[{}]", split_info.pb_difference));
    run_info.append(std::format("{:<{}}", "", biggest_split_time_length - split_info.time.size() + kFieldSpacing));
    run_info.append(std::format("{}\n", split_info.time));
  });

  run_info.append("```");

  return run_info;
}