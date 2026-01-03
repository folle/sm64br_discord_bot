cmake_minimum_required(VERSION 4.2.1)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#find_program(CLANG_TIDY_EXECUTABLE NAMES "clang-tidy" REQUIRED)
#set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXECUTABLE} -checks=-*,readability-*)

find_package(Git REQUIRED)
execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                RESULT_VARIABLE GIT_SUBMODULE_RESULT)
if(GIT_SUBMODULE_RESULT)
  message(FATAL_ERROR "Failed to run 'git submodule update --init': '${GIT_SUBMODULE_RESULT}'")
endif()

project(sm64br_discord_bot VERSION 1.0 DESCRIPTION "SM64BR Discord Bot")

add_executable(${PROJECT_NAME}
               src/main.cc
               src/bot/sm64br_discord_bot.cc
               src/bot/sm64br_discord_bot.h
               src/bot/settings/settings.cc
               src/bot/settings/settings.h
               src/bot/message/message_handler.cc
               src/bot/message/message_handler.h
               #src/bot/the_run/payload_parser.cc
               #src/bot/the_run/payload_parser.h
               #src/bot/the_run/the_run.cc
               #src/bot/the_run/the_run.h
               src/logger/logger.cc
               src/logger/logger.h
               src/logger/logger_factory.cc
               src/logger/logger_factory.h)

target_include_directories(${PROJECT_NAME} PRIVATE
                           src
                           src/bot)

find_package(Boost REQUIRED COMPONENTS beast)                                       # therun.gg integration
find_package(dpp CONFIG REQUIRED)                                                   # Interfacing with Discord
find_package(nlohmann_json CONFIG REQUIRED)                                         # Settings storage, therun.gg parsing
find_package(OpenSSL REQUIRED)                                                      # DPP dependency
find_package(spdlog CONFIG REQUIRED)                                                # Logging

target_link_libraries(${PROJECT_NAME} PRIVATE
                      Boost::beast
                      dpp::dpp
                      nlohmann_json::nlohmann_json
                      OpenSSL::Crypto
                      spdlog::spdlog_header_only)

set_target_properties(${PROJECT_NAME} PROPERTIES
                      CXX_STANDARD 23
                      CXX_STANDARD_REQUIRED ON)

if(NOT EXISTS "${CMAKE_BINARY_DIR}/settings/settings.json")
  configure_file(sample/settings.json "${CMAKE_BINARY_DIR}/settings/settings.json" COPYONLY)
endif()

if (CMAKE_EXPORT_COMPILE_COMMANDS)
  add_custom_target(
      copy_compile_commands ALL
      COMMAND ${CMAKE_COMMAND} -E create_symlink
              ${CMAKE_BINARY_DIR}/compile_commands.json
              ${CMAKE_SOURCE_DIR}/compile_commands.json
      COMMENT "Symlinking build compile_commands.json to project root"
      VERBATIM
  )
endif()