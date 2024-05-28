#pragma once

#include <map>
#include <nlohmann/json.hpp>

#include "logger/logger.h"
#include "settings/settings.h"


class PayloadParser final {
public:
  PayloadParser() = delete;
  ~PayloadParser() = default;

  PayloadParser(Settings const& settings, std::string const& payload) noexcept;

  bool IsPingable() const noexcept;
 
  std::string const& GetUser() const noexcept;
  
  std::string GetString() const noexcept;

private:
  void Parse(Settings const& settings, std::string const& payload) noexcept;
  bool ParseRunData(Settings const& settings, nlohmann::json const& run_data);
  bool ParseSplitsData(nlohmann::json const& splits_data);

private:
  std::shared_ptr<spdlog::async_logger> const logger_ = Logger::Get().Create("Payload Parser");

  bool emulator_{};
  bool successfully_parsed_{};

  std::string user_;
  std::string category_;
  std::string pb_;
  std::string bpt_;
  std::string sob_;
  std::string url_;

  size_t attempt_count_{};

  struct SplitInfo {
    std::string name;
    std::string pb_difference;
    std::string time;
  };
  std::map<size_t, SplitInfo> splits_;
};