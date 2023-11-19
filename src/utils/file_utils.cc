#include "file_utils.h"

#include <exception>
#include <fmt/format.h>


void FileUtils::CreateDir(const std::filesystem::path& path, const std::shared_ptr<spdlog::async_logger>& logger) {
  if (std::filesystem::exists(path)) {
    return;
  }
    
  if(!std::filesystem::create_directory(path)) {
    const std::string error_message = fmt::format("Failed to create directory: {}\n", path.string());
    logger->critical(error_message);
    throw std::runtime_error(error_message);
  }

  logger->info("Successfully created directory: {}", path.string());
}
