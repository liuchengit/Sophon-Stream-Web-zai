# ============================================================================
# CMake Toolchain File for BM1688 SoC (aarch64)
# Sophon-Stream Web Management System
# ============================================================================
#
# Usage:
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/BM1688-toolchain.cmake ..
#
# Prerequisites:
#   - aarch64-linux-gnu-gcc / aarch64-linux-gnu-g++ cross-compiler
#   - Sophon-Stream SDK for BM1688 installed in /opt/sophon-stream
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
# BM1688 uses newer ARM core with different optimization targets
set(CMAKE_C_FLAGS_INIT             "-march=armv8.2-a -mtune=cortex-a76")
set(CMAKE_CXX_FLAGS_INIT           "-march=armv8.2-a -mtune=cortex-a76")
set(CMAKE_C_FLAGS_RELEASE_INIT     "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE_INIT   "-O2 -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG_INIT       "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG_INIT     "-g -O0 -DDEBUG")

# ── Sophon-Stream SDK paths for BM1688 ─────────────────────────────────────
set(SOPHON_STREAM_INCLUDE_DIR "/opt/sophon-stream/include" CACHE PATH
    "Path to sophon-stream SDK headers for BM1688")
set(SOPHON_STREAM_LIB_DIR "/opt/sophon-stream/lib/bm1688" CACHE PATH
    "Path to sophon-stream SDK libraries for BM1688")

# ── Platform definition ────────────────────────────────────────────────────
add_definitions(-DTARGET_BM1688)
add_definitions(-D__aarch64__)

# ── Sysroot for cross-compilation ─────────────────────────────────────────
# If you have a custom sysroot, set it here:
# set(CMAKE_SYSROOT "/opt/sophon-stream/sysroot")

message(STATUS "Cross-compiling for BM1688 (aarch64)")
message(STATUS "C Compiler:   ${CMAKE_C_COMPILER}")
message(STATUS "CXX Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "SDK Include:  ${SOPHON_STREAM_INCLUDE_DIR}")
message(STATUS "SDK Lib:      ${SOPHON_STREAM_LIB_DIR}")
