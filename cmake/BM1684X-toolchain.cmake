# ============================================================================
# CMake Toolchain File for BM1684X SoC (aarch64)
# Sophon-Stream Web Management System
# ============================================================================
#
# Usage:
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/BM1684X-toolchain.cmake ..
#
# Prerequisites:
#   - aarch64-linux-gnu-gcc / aarch64-linux-gnu-g++ cross-compiler
#   - Sophon-Stream SDK for BM1684X installed in /opt/sophon-stream
#   - Cross-compiled sysroot with required libraries
#
# Install cross-compiler on Ubuntu:
#   sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
# ============================================================================

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# ── Cross-compiler ─────────────────────────────────────────────────────────
set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# ── Target environment ─────────────────────────────────────────────────────
set(CMAKE_FIND_ROOT_PATH
    /opt/sophon-stream/sysroot
    /opt/sophon-stream
)

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# ── Compiler flags ─────────────────────────────────────────────────────────
set(CMAKE_C_FLAGS_INIT             "-march=armv8-a -mtune=cortex-a72")
set(CMAKE_CXX_FLAGS_INIT           "-march=armv8-a -mtune=cortex-a72")
set(CMAKE_C_FLAGS_RELEASE_INIT     "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE_INIT   "-O2 -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG_INIT       "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG_INIT     "-g -O0 -DDEBUG")

# ── Sophon-Stream SDK paths for BM1684X ────────────────────────────────────
set(SOPHON_STREAM_INCLUDE_DIR "/opt/sophon-stream/include" CACHE PATH
    "Path to sophon-stream SDK headers for BM1684X")
set(SOPHON_STREAM_LIB_DIR "/opt/sophon-stream/lib/bm1684x" CACHE PATH
    "Path to sophon-stream SDK libraries for BM1684X")

# ── Platform definition ────────────────────────────────────────────────────
add_definitions(-DTARGET_BM1684X)
add_definitions(-D__aarch64__)

# ── Sysroot for cross-compilation ─────────────────────────────────────────
# If you have a custom sysroot, set it here:
# set(CMAKE_SYSROOT "/opt/sophon-stream/sysroot")

message(STATUS "Cross-compiling for BM1684X (aarch64)")
message(STATUS "C Compiler:   ${CMAKE_C_COMPILER}")
message(STATUS "CXX Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "SDK Include:  ${SOPHON_STREAM_INCLUDE_DIR}")
message(STATUS "SDK Lib:      ${SOPHON_STREAM_LIB_DIR}")
