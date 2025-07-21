#pragma once

#include <cstddef>
#include <map>
#include <string>

#include <nlohmann/json.hpp>

#include "logger/logger_factory.h"

class PayloadParser final {
public:
  PayloadParser() = delete;
  ~PayloadParser() = default;

  PayloadParser(std::string const& payload) noexcept;

  bool IsPingable() const noexcept;
 
  std::string const& GetUser() const noexcept;
  
  std::string GetString() const noexcept;

private:
  void Parse(std::string const& payload) noexcept;
  bool ParseRunData(nlohmann::json const& run_data);
  bool ParseSplitsData(nlohmann::json const& splits_data);

private:
  Logger const logger_ = LoggerFactory::Get().Create("Payload Parser");

  bool emulator_{};
  bool successfully_parsed_{};

  std::string user_;
  std::string category_;
  std::string pb_;
  std::string bpt_;
  std::string sob_;
  std::string url_;

  std::size_t attempt_count_{};

  struct SplitInfo {
    std::string name;
    std::string pb_difference;
    std::string time;
  };
  std::map<std::size_t, SplitInfo> splits_;
};