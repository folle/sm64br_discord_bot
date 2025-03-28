cmake_minimum_required(VERSION 3.25.1)

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

if (${CMAKE_C_COMPILER} MATCHES "clang-cl")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Qunused-arguments")
endif()

if (${CMAKE_CXX_COMPILER} MATCHES "clang-cl")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments")
endif()

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

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
find_package(fmt CONFIG REQUIRED)                                                   # Format logging
find_package(nlohmann_json CONFIG REQUIRED)                                         # Settings storage, therun.gg parsing
find_package(OpenSSL REQUIRED)                                                      # DPP dependency
find_package(spdlog CONFIG REQUIRED)                                                # Logging

target_link_libraries(${PROJECT_NAME} PRIVATE
                      Boost::beast
                      dpp::dpp
                      fmt::fmt-header-only
                      nlohmann_json::nlohmann_json
                      OpenSSL::Crypto
                      spdlog::spdlog_header_only)

set_target_properties(${PROJECT_NAME} PROPERTIES
                      CXX_STANDARD 23
                      CXX_STANDARD_REQUIRED ON)

if(NOT EXISTS "${CMAKE_BINARY_DIR}/settings/settings.json")
  configure_file(sample/settings.json "${CMAKE_BINARY_DIR}/settings/settings.json" COPYONLY)
endif()