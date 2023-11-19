#pragma once

#include <filesystem>
#include <memory>

#include "logger/logger.h"


class FileUtils final {
public:
  static void CreateDir(const std::filesystem::path& path, const std::shared_ptr<spdlog::async_logger>& logger);
};