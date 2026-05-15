cmake_minimum_required(VERSION 3.14)
project(sophon-stream-web VERSION 1.0.0 LANGUAGES CXX)

# ──────────────────────────────────────────────
# Build options
# ──────────────────────────────────────────────
option(BUILD_TYPE_RELEASE "Build in Release mode" ON)
option(ENABLE_SSL "Enable SSL support" OFF)
option(ENABLE_TESTS "Build unit tests" OFF)

if(BUILD_TYPE_RELEASE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
else()
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ──────────────────────────────────────────────
# C++ standard
# ──────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ──────────────────────────────────────────────
# Compiler flags
# ──────────────────────────────────────────────
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

# ──────────────────────────────────────────────
# Find required packages
# ──────────────────────────────────────────────
find_package(drogon CONFIG REQUIRED)
find_package(sqlite3 REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

# ──────────────────────────────────────────────
# sophon-stream SDK paths
# ──────────────────────────────────────────────
set(SOPHON_STREAM_INCLUDE_DIR "/opt/sophon-stream/include" CACHE PATH
    "Path to sophon-stream SDK headers")
set(SOPHON_STREAM_LIB_DIR "/opt/sophon-stream/lib" CACHE PATH
    "Path to sophon-stream SDK libraries")

# ──────────────────────────────────────────────
# Source files
# ──────────────────────────────────────────────
set(SOURCES
    src/main.cc
    src/database/DbManager.cc
    src/utils/JwtUtils.cc
    src/utils/CryptoUtils.cc
    src/utils/ConfigUtils.cc
    src/middleware/AuthMiddleware.cc
    src/middleware/CORSMiddleware.cc
    src/websocket/MonitorWsController.cc
    src/sophon_bridge/EngineBridge.cc
    # Services
    src/services/AuthService.cc
    src/services/DeviceService.cc
    src/services/TaskService.cc
    src/services/AlgorithmService.cc
    src/services/MonitorService.cc
    src/services/AlertService.cc
    src/services/PluginService.cc
    src/services/SystemService.cc
    # Controllers
    src/controllers/AuthController.cc
    src/controllers/DeviceController.cc
    src/controllers/TaskController.cc
    src/controllers/AlgorithmController.cc
    src/controllers/MonitorController.cc
    src/controllers/AlertController.cc
    src/controllers/PluginController.cc
    src/controllers/SystemController.cc
    src/controllers/ControllerRegistry.cc
)

# ──────────────────────────────────────────────
# Executable target
# ──────────────────────────────────────────────
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SOPHON_STREAM_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    drogon::drogon
    SQLite::SQLite3
    jwt-cpp::jwt-cpp
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
    ${CMAKE_DL_LIBS}
)

# Link sophon-stream libraries if available
if(EXISTS "${SOPHON_STREAM_LIB_DIR}")
    file(GLOB SOPHON_STREAM_LIBS "${SOPHON_STREAM_LIB_DIR}/libsophon-stream*.so")
    if(SOPHON_STREAM_LIBS)
        target_link_directories(${PROJECT_NAME} PRIVATE ${SOPHON_STREAM_LIB_DIR})
        target_link_libraries(${PROJECT_NAME} PRIVATE
            sophon-stream-framework
            sophon-stream-element
        )
        target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_SOPHON_STREAM_SDK)
        message(STATUS "Found sophon-stream SDK: ${SOPHON_STREAM_LIB_DIR}")
    else()
        message(WARNING "sophon-stream SDK libraries not found in ${SOPHON_STREAM_LIB_DIR}, building without SDK")
    endif()
else()
    message(WARNING "sophon-stream SDK directory not found at ${SOPHON_STREAM_LIB_DIR}, building without SDK")
endif()

# ──────────────────────────────────────────────
# Compile definitions
# ──────────────────────────────────────────────
target_compile_definitions(${PROJECT_NAME} PRIVATE
    PROJECT_VERSION="${PROJECT_VERSION}"
    PROJECT_NAME="${PROJECT_NAME}"
)

if(ENABLE_SSL)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_SSL)
endif()

# ──────────────────────────────────────────────
# Install rules
# ──────────────────────────────────────────────
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(DIRECTORY config/
    DESTINATION etc/${PROJECT_NAME}
    FILES_MATCHING PATTERN "*.json"
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/config/config.json
    DESTINATION etc/${PROJECT_NAME}
)

# ──────────────────────────────────────────────
# Tests
# ──────────────────────────────────────────────
if(ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ──────────────────────────────────────────────
# Status messages
# ──────────────────────────────────────────────
message(STATUS "")
message(STATUS "=== ${PROJECT_NAME} ${PROJECT_VERSION} ===")
message(STATUS "Build type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard:     C++${CMAKE_CXX_STANDARD}")
message(STATUS "Install prefix:   ${CMAKE_INSTALL_PREFIX}")
message(STATUS "SSL support:      ${ENABLE_SSL}")
message(STATUS "Tests:            ${ENABLE_TESTS}")
message(STATUS "Sophon-Stream SDK: ${SOPHON_STREAM_INCLUDE_DIR}")
message(STATUS "")
