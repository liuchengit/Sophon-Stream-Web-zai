# SOPHON-STREAM-WEB Backend CMake Configuration
cmake_minimum_required(VERSION 3.16)
project(sophon-stream-web VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Find required packages
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(OpenSSL REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    /usr/local/include
    /usr/local/include/drogon
    /usr/include/jsoncpp
)

# Link directories
link_directories(/usr/local/lib)

# Source files
set(SOURCES
    src/main.cc
    src/controllers/ApiController.cc
    src/controllers/MonitorWsCtrl.cc
    src/services/AuthService.cc
    src/services/DeviceService.cc
    src/services/TaskService.cc
    src/services/AlgorithmService.cc
    src/services/AlertService.cc
    src/services/MonitorService.cc
    src/services/PluginService.cc
    src/services/SystemService.cc
    src/database/DbManager.cc
    src/utils/JwtUtils.cc
    src/utils/CryptoUtils.cc
    src/utils/ConfigUtils.cc
    src/websocket/MonitorWsHandler.cc
    src/sophon_bridge/EngineBridge.cc
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    drogon
    trantor
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    OpenSSL::SSL
    OpenSSL::Crypto
    yaml-cpp
    jsoncpp
    hiredis
    z
    uuid
    pthread
    sqlite3
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
)

# Install targets
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/config/ DESTINATION ${CMAKE_INSTALL_PREFIX}/config)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/data/ DESTINATION ${CMAKE_INSTALL_PREFIX}/data)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
