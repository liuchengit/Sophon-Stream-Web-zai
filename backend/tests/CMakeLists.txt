cmake_minimum_required(VERSION 3.14)

# ============================================================================
# Sophon-Stream Web Management System - Unit Tests
# ============================================================================

find_package(GTest CONFIG REQUIRED)

# ── Test executable ────────────────────────────────────────────────────────
set(TEST_SOURCES
    test_auth.cpp
    test_device.cpp
    test_task.cpp
)

add_executable(sophon-stream-web-tests ${TEST_SOURCES})

target_include_directories(sophon-stream-web-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${SOPHON_STREAM_INCLUDE_DIR}
)

target_link_libraries(sophon-stream-web-tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
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
        target_link_directories(sophon-stream-web-tests PRIVATE ${SOPHON_STREAM_LIB_DIR})
        target_link_libraries(sophon-stream-web-tests PRIVATE
            sophon-stream-framework
            sophon-stream-element
        )
        target_compile_definitions(sophon-stream-web-tests PRIVATE HAS_SOPHON_STREAM_SDK)
    endif()
endif()

# Compile definitions
target_compile_definitions(sophon-stream-web-tests PRIVATE
    PROJECT_VERSION="${PROJECT_VERSION}"
    PROJECT_NAME="${PROJECT_NAME}"
    TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

# C++ standard
target_compile_features(sophon-stream-web-tests PRIVATE cxx_std_17)

# ── Register tests with CTest ─────────────────────────────────────────────
include(GoogleTest)
gtest_discover_tests(sophon-stream-web-tests
    PROPERTIES
        TIMEOUT 60
    DISCOVERY_TIMEOUT 30
)
