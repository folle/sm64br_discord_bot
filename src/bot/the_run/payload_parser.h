#pragma once

#include <map>

#include "logger/logger.h"


class PayloadParser final {
public:
  PayloadParser() = delete;
  ~PayloadParser() = default;

  PayloadParser(std::string const& payload) noexcept;

  bool IsValidGame() const noexcept;
  bool IsValidCategory() const noexcept;
  bool IsAboveThreshold() const noexcept;
  bool IsPacing() const noexcept;
 
  std::string const& GetUser() const noexcept;
  
  std::string GetString() const noexcept;

private:
  void Process(std::string const& payload) noexcept;

private:
  std::shared_ptr<spdlog::async_logger> const logger_ = Logger::Get().Create("Payload Parser");

  bool valid_game_ = false;
  bool valid_category_ = false;
  bool above_threshold_ = false;
  bool pacing_ = false;

  std::string user_;
  std::string category_;
  std::string pb_;
  std::string bpt_;

  struct SplitInfo {
    std::string name;
    std::string time;
    std::string pb;
  };
  std::map<size_t, SplitInfo> splits_;
};