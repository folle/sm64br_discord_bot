cmake_minimum_required(VERSION 3.27.2)


find_package(Git QUIET)

if(GIT_FOUND)
  message(STATUS "Updating VCPKG submodule")
  execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                  RESULT_VARIABLE GIT_SUBMOD_RESULT)

  if(GIT_SUBMOD_RESULT EQUAL "0")
    message(STATUS "Successfully updated VCPKG submodule")
  else()
    message(FATAL_ERROR "Failed to update VCPKG submodule: '${GIT_SUBMOD_RESULT}'")
  endif()
endif()


project(sm64br_discord_bot VERSION 1.0 DESCRIPTION "SM64BR Discord Bot")  


# Bot project
add_executable(${PROJECT_NAME}
  src/main.cc
  src/bot/defs.h
  src/bot/sm64br_discord_bot.cc
  src/bot/sm64br_discord_bot.h
  src/bot/database/server_database.cc
  src/bot/database/server_database.h
  src/bot/message_handler/message_handler.cc
  src/bot/message_handler/message_handler.h
  src/logger/logger.cc
  src/logger/logger.h
  src/utils/file_utils.cc
  src/utils/file_utils.h)


target_include_directories(${PROJECT_NAME} PRIVATE
  src
  src/bot)


find_package(dpp CONFIG REQUIRED)                 # Interfacing with Discord
find_package(fmt CONFIG REQUIRED)                 # Exceptions/Logging
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)              # Logging

if(UNIX)
find_package(Opus CONFIG REQUIRED)                # DPP dependency
find_package(unofficial-sodium CONFIG REQUIRED)   # DPP dependency
endif(UNIX)

if(WIN32)
target_link_libraries(${PROJECT_NAME} PRIVATE
  dpp::dpp
  fmt::fmt-header-only
  nlohmann_json::nlohmann_json
  spdlog::spdlog_header_only)
elseif(UNIX)
target_link_libraries(${PROJECT_NAME} PRIVATE
  dpp::dpp
  fmt::fmt-header-only
  Opus::opus
  spdlog::spdlog_header_only
  unofficial-sodium::sodium unofficial-sodium::sodium_config_public)
endif()


set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED ON)


if(NOT EXISTS "${CMAKE_BINARY_DIR}/database/server_database.json")
  configure_file(samples/server_database.json "${CMAKE_BINARY_DIR}/database/server_database.json", COPYONLY)
endif()