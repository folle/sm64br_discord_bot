cmake_minimum_required(VERSION 3.25.1)


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
               src/bot/database/database.cc
               src/bot/database/database.h
               src/bot/message/message_handler.cc
               src/bot/message/message_handler.h
               src/bot/the_run/the_run.cc
               src/bot/the_run/the_run.h
               src/logger/logger.cc
               src/logger/logger.h)


target_include_directories(${PROJECT_NAME} PRIVATE
                           src
                           src/bot)


find_package(dpp CONFIG REQUIRED)                                                   # Interfacing with Discord
find_package(fmt CONFIG REQUIRED)                                                   # Format logging
find_package(nlohmann_json CONFIG REQUIRED)                                         # Database storage
find_package(Opus CONFIG REQUIRED)                                                  # DPP dependency
find_package(OpenSSL REQUIRED)                                                      # DPP dependency
find_package(spdlog CONFIG REQUIRED)                                                # Logging
find_package(websocketpp CONFIG REQUIRED)                                           # therun.gg integration
find_package(ZLIB REQUIRED)                                                         # DPP dependency

if(UNIX)
find_package(unofficial-sodium CONFIG REQUIRED)                                     # DPP dependency
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
                      dpp::dpp
                      fmt::fmt-header-only
                      nlohmann_json::nlohmann_json
                      OpenSSL::SSL
                      Opus::opus
                      spdlog::spdlog_header_only
                      websocketpp::websocketpp
                      ZLIB::ZLIB)

if(UNIX)
target_link_libraries(${PROJECT_NAME} PRIVATE unofficial-sodium::sodium)
endif()


set_target_properties(${PROJECT_NAME} PROPERTIES
                      CXX_STANDARD 23
                      CXX_STANDARD_REQUIRED ON)


if(NOT EXISTS "${CMAKE_BINARY_DIR}/database/database.json")
  configure_file(sample/database.json "${CMAKE_BINARY_DIR}/database/database.json" COPYONLY)
endif()