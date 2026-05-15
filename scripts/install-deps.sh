#!/bin/bash
# ============================================================================
# Sophon-Stream Web Management System Dependency Installation Script
# Supports: Ubuntu 20.04/22.04, Debian 10/11, BM1684/BM1684X/BM1688 SoC
# ============================================================================
set -e

# ── Color output ────────────────────────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info()  { echo -e "${BLUE}[INFO]${NC}  $*"; }
log_ok()    { echo -e "${GREEN}[OK]${NC}    $*"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC}  $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*"; }

# ── Defaults ────────────────────────────────────────────────────────────────
INSTALL_BUILD_DEPS=true
INSTALL_RUNTIME_DEPS=true
INSTALL_SDK=false
INSTALL_CROSS_COMPILE=false
SOC_TYPE=""
DROGON_FROM_SOURCE=false

# ── Usage ───────────────────────────────────────────────────────────────────
usage() {
    cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Install dependencies for Sophon-Stream Web Management System

Options:
  --build-only           Install build dependencies only
  --runtime-only         Install runtime dependencies only
  --sdk                  Also install sophon-stream SDK
  --cross-compile <soc>  Install cross-compilation toolchain for SOC type
                         Supported: BM1684X, BM1688
  --drogon-source        Build drogon from source (if not available via apt)
  -h, --help             Show this help message

Examples:
  # Full development setup
  $(basename "$0")

  # With sophon-stream SDK
  $(basename "$0") --sdk

  # Cross-compilation for BM1684X
  $(basename "$0") --cross-compile BM1684X

EOF
    exit 0
}

# ── Parse arguments ─────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-only)
            INSTALL_RUNTIME_DEPS=false
            shift
            ;;
        --runtime-only)
            INSTALL_BUILD_DEPS=false
            shift
            ;;
        --sdk)
            INSTALL_SDK=true
            shift
            ;;
        --cross-compile)
            INSTALL_CROSS_COMPILE=true
            SOC_TYPE="$2"
            shift 2
            ;;
        --drogon-source)
            DROGON_FROM_SOURCE=true
            shift
            ;;
        -h|--help)
            usage
            ;;
        *)
            log_error "Unknown option: $1"
            usage
            ;;
    esac
done

# ── Detect OS ──────────────────────────────────────────────────────────────
detect_os() {
    if [[ -f /etc/os-release ]]; then
        . /etc/os-release
        echo "${ID} ${VERSION_ID}"
    elif [[ -f /etc/lsb-release ]]; then
        . /etc/lsb-release
        echo "${DISTRIB_ID} ${DISTRIB_RELEASE}"
    else
        echo "unknown"
    fi
}

# ── Detect SoC type ────────────────────────────────────────────────────────
detect_soc() {
    # Check if running on a Sophon SoC device
    if [[ -f /opt/sophon/conf/bmu.conf ]]; then
        local soc_info
        soc_info=$(cat /opt/sophon/conf/bmu.conf 2>/dev/null | grep -i "chip" || echo "")
        if echo "${soc_info}" | grep -qi "bm1684x"; then
            echo "BM1684X"
        elif echo "${soc_info}" | grep -qi "bm1688"; then
            echo "BM1688"
        elif echo "${soc_info}" | grep -qi "bm1684"; then
            echo "BM1684"
        else
            echo "unknown"
        fi
    elif [[ -d /opt/sophon ]]; then
        # Try detecting from installed SDK
        if ls /opt/sophon/libsophon-stream*bm1684x* &>/dev/null; then
            echo "BM1684X"
        elif ls /opt/sophon/libsophon-stream*bm1688* &>/dev/null; then
            echo "BM1688"
        else
            echo "unknown"
        fi
    else
        echo "x86"
    fi
}

# ── Install system packages ────────────────────────────────────────────────
install_system_packages() {
    local os_id
    os_id=$(detect_os | awk '{print $1}')

    log_info "Detected OS: $(detect_os)"
    log_info "Detected SoC: $(detect_soc)"

    case "${os_id}" in
        ubuntu|debian)
            install_debian_packages
            ;;
        centos|rhel|rocky)
            install_rhel_packages
            ;;
        *)
            log_error "Unsupported OS: ${os_id}"
            log_info "Please install dependencies manually"
            return 1
            ;;
    esac
}

# ── Debian/Ubuntu packages ─────────────────────────────────────────────────
install_debian_packages() {
    log_info "Installing Debian/Ubuntu packages..."

    # Update package list
    apt-get update -qq

    # Common build dependencies
    local build_packages=(
        build-essential
        cmake
        gcc
        g++
        gdb
        pkg-config
        libsqlite3-dev
        libssl-dev
        zlib1g-dev
        libboost-all-dev
        uuid-dev
        git
        curl
        wget
    )

    # Runtime dependencies
    local runtime_packages=(
        libsqlite3-0
        libssl3
        zlib1g
        curl
    )

    if [[ "${INSTALL_BUILD_DEPS}" == true ]]; then
        log_info "Installing build dependencies..."
        DEBIAN_FRONTEND=noninteractive apt-get install -y -qq "${build_packages[@]}"

        # Install Node.js 18.x
        install_nodejs

        # Install drogon
        install_drogon
    fi

    if [[ "${INSTALL_RUNTIME_DEPS}" == true ]]; then
        log_info "Installing runtime dependencies..."
        DEBIAN_FRONTEND=noninteractive apt-get install -y -qq "${runtime_packages[@]}"
    fi

    log_ok "Debian/Ubuntu packages installed"
}

# ── RHEL/CentOS packages ───────────────────────────────────────────────────
install_rhel_packages() {
    log_info "Installing RHEL/CentOS packages..."

    local build_packages=(
        gcc
        gcc-c++
        make
        cmake
        sqlite-devel
        openssl-devel
        zlib-devel
        boost-devel
        libuuid-devel
        git
        curl
        wget
    )

    if [[ "${INSTALL_BUILD_DEPS}" == true ]]; then
        yum install -y "${build_packages[@]}"
        install_nodejs
        install_drogon
    fi

    log_ok "RHEL/CentOS packages installed"
}

# ── Install Node.js ────────────────────────────────────────────────────────
install_nodejs() {
    if command -v node &>/dev/null && [[ "$(node --version | cut -d. -f1)" == "v18" || "$(node --version | cut -d. -f1)" == "v20" ]]; then
        log_ok "Node.js $(node --version) already installed"
        return 0
    fi

    log_info "Installing Node.js 18.x..."

    # Use NodeSource repository
    curl -fsSL https://deb.nodesource.com/setup_18.x | bash - 2>/dev/null || {
        # Fallback for non-Debian
        log_warn "NodeSource setup failed, trying alternative method..."
        local arch="$(uname -m)"
        local node_arch="x64"
        [[ "${arch}" == "aarch64" ]] && node_arch="arm64"

        local node_version="18.20.2"
        wget -q "https://nodejs.org/dist/v${node_version}/node-v${node_version}-linux-${node_arch}.tar.xz" -O /tmp/node.tar.xz
        tar -xf /tmp/node.tar.xz -C /usr/local --strip-components=1
        rm -f /tmp/node.tar.xz
    }

    DEBIAN_FRONTEND=noninteractive apt-get install -y -qq nodejs 2>/dev/null || true

    if command -v node &>/dev/null; then
        log_ok "Node.js $(node --version) installed"
        log_ok "npm $(npm --version) installed"
    else
        log_error "Node.js installation failed"
        return 1
    fi
}

# ── Install Drogon ─────────────────────────────────────────────────────────
install_drogon() {
    # Check if drogon is already available
    if pkg-config --exists drogon 2>/dev/null || dpkg -l | grep -q libdrogon-dev 2>/dev/null; then
        log_ok "Drogon is already installed"
        return 0
    fi

    if [[ "${DROGON_FROM_SOURCE}" != true ]]; then
        # Try to install from package first
        log_info "Attempting to install drogon from package..."
        add-apt-repository -y ppa:drogon-team/drogon 2>/dev/null && \
            apt-get update -qq && \
            DEBIAN_FRONTEND=noninteractive apt-get install -y -qq libdrogon-dev libtrantor-dev 2>/dev/null

        if pkg-config --exists drogon 2>/dev/null; then
            log_ok "Drogon installed from package"
            return 0
        fi

        log_warn "Drogon package not available, building from source..."
    fi

    log_info "Building drogon from source..."
    local drogon_dir="/tmp/drogon-build"
    rm -rf "${drogon_dir}"
    mkdir -p "${drogon_dir}"

    git clone --depth 1 https://github.com/drogonframework/drogon.git "${drogon_dir}/drogon"
    git clone --depth 1 https://github.com/an-tao/trantor.git "${drogon_dir}/trantor"

    # Build trantor first
    mkdir -p "${drogon_dir}/trantor/build"
    cmake -S "${drogon_dir}/trantor" -B "${drogon_dir}/trantor/build" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local
    cmake --build "${drogon_dir}/trantor/build" -j"$(nproc)"
    cmake --install "${drogon_dir}/trantor/build"

    # Build drogon
    mkdir -p "${drogon_dir}/drogon/build"
    cmake -S "${drogon_dir}/drogon" -B "${drogon_dir}/drogon/build" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DBUILD_TESTING=OFF \
        -DBUILD_EXAMPLES=OFF
    cmake --build "${drogon_dir}/drogon/build" -j"$(nproc)"
    cmake --install "${drogon_dir}/drogon/build"

    ldconfig

    # Cleanup
    rm -rf "${drogon_dir}"

    if pkg-config --exists drogon 2>/dev/null; then
        log_ok "Drogon built and installed from source"
    else
        log_error "Drogon installation failed"
        return 1
    fi
}

# ── Install sophon-stream SDK ──────────────────────────────────────────────
install_sdk() {
    log_info "Installing sophon-stream SDK..."

    local soc_type
    soc_type=$(detect_soc)
    log_info "Detected SoC type: ${soc_type}"

    local sdk_url_base="https://sophon-file.sophon.cn/sophon-prod-s3/drive/23"

    if [[ "${soc_type}" == "BM1684X" ]] || [[ -n "${SOC_TYPE}" && "${SOC_TYPE}" == "BM1684X" ]]; then
        log_info "Installing sophon-stream SDK for BM1684X..."
        # Download and install BM1684X SDK
        mkdir -p /tmp/sophon-sdk
        # Note: Actual URLs should be updated with the latest SDK release
        log_warn "Please download sophon-stream SDK for BM1684X manually from:"
        log_warn "  https://sophon.ai/"
        log_warn "And install to /opt/sophon-stream/"

    elif [[ "${soc_type}" == "BM1688" ]] || [[ -n "${SOC_TYPE}" && "${SOC_TYPE}" == "BM1688" ]]; then
        log_info "Installing sophon-stream SDK for BM1688..."
        log_warn "Please download sophon-stream SDK for BM1688 manually from:"
        log_warn "  https://sophon.ai/"
        log_warn "And install to /opt/sophon-stream/"
    else
        log_info "Installing sophon-stream SDK for PCIe (x86)..."
        log_warn "Please download sophon-stream SDK for PCIe from:"
        log_warn "  https://sophon.ai/"
        log_warn "And install to /opt/sophon-stream/"
    fi

    # Create SDK directory structure if it doesn't exist
    mkdir -p /opt/sophon-stream/{include,lib,config}

    log_ok "SDK directory structure created at /opt/sophon-stream"
}

# ── Install cross-compilation toolchain ────────────────────────────────────
install_cross_compile_toolchain() {
    if [[ -z "${SOC_TYPE}" ]]; then
        log_error "Please specify SOC type: BM1684X or BM1688"
        exit 1
    fi

    log_info "Installing cross-compilation toolchain for ${SOC_TYPE}..."

    # Install aarch64 cross-compiler
    DEBIAN_FRONTEND=noninteractive apt-get install -y -qq \
        gcc-aarch64-linux-gnu \
        g++-aarch64-linux-gnu \
        binutils-aarch64-linux-gnu

    # Verify cross-compiler
    if aarch64-linux-gnu-gcc --version &>/dev/null; then
        log_ok "aarch64 cross-compiler installed: $(aarch64-linux-gnu-gcc --version | head -1)"
    else
        log_error "Failed to install aarch64 cross-compiler"
        return 1
    fi

    # Install cross-compiled libraries for target
    log_info "Setting up sysroot for ${SOC_TYPE}..."

    local sysroot_dir="/opt/sophon-stream/sysroot"
    mkdir -p "${sysroot_dir}"/{usr/include,usr/lib}

    log_ok "Cross-compilation toolchain for ${SOC_TYPE} installed"
    log_info "Toolchain file: cmake/${SOC_TYPE}-toolchain.cmake"
}

# ── Main ───────────────────────────────────────────────────────────────────
main() {
    log_info "Sophon-Stream Web Dependency Installer"
    log_info "========================================"

    # Check for root
    if [[ "$(id -u)" -ne 0 ]]; then
        log_warn "Not running as root, some packages may fail to install"
        log_info "Try: sudo $0 $*"
    fi

    install_system_packages

    if [[ "${INSTALL_SDK}" == true ]]; then
        install_sdk
    fi

    if [[ "${INSTALL_CROSS_COMPILE}" == true ]]; then
        install_cross_compile_toolchain
    fi

    # Install additional C++ libraries
    if [[ "${INSTALL_BUILD_DEPS}" == true ]]; then
        log_info "Installing additional C++ libraries..."

        # jwt-cpp
        if ! pkg-config --exists jwt-cpp 2>/dev/null; then
            log_info "Installing jwt-cpp..."
            local jwt_dir="/tmp/jwt-cpp-build"
            rm -rf "${jwt_dir}"
            mkdir -p "${jwt_dir}"
            git clone --depth 1 https://github.com/Thalhammer/jwt-cpp.git "${jwt_dir}/jwt-cpp"
            mkdir -p "${jwt_dir}/jwt-cpp/build"
            cmake -S "${jwt_dir}/jwt-cpp" -B "${jwt_dir}/jwt-cpp/build" \
                -DCMAKE_INSTALL_PREFIX=/usr/local \
                -DJWT_CMAKE_FILES_INSTALL_DIR=/usr/lib/cmake/jwt-cpp
            cmake --build "${jwt_dir}/jwt-cpp/build"
            cmake --install "${jwt_dir}/jwt-cpp/build"
            rm -rf "${jwt_dir}"
            ldconfig
            log_ok "jwt-cpp installed"
        fi

        # nlohmann_json
        if ! pkg-config --exists nlohmann_json 2>/dev/null; then
            log_info "Installing nlohmann_json..."
            local json_dir="/tmp/nlohmann-json-build"
            rm -rf "${json_dir}"
            mkdir -p "${json_dir}"
            git clone --depth 1 https://github.com/nlohmann/json.git "${json_dir}/json"
            mkdir -p "${json_dir}/json/build"
            cmake -S "${json_dir}/json" -B "${json_dir}/json/build" \
                -DCMAKE_INSTALL_PREFIX=/usr/local
            cmake --build "${json_dir}/json/build"
            cmake --install "${json_dir}/json/build"
            rm -rf "${json_dir}"
            ldconfig
            log_ok "nlohmann_json installed"
        fi

        # spdlog
        if ! pkg-config --exists spdlog 2>/dev/null; then
            log_info "Installing spdlog..."
            local spdlog_dir="/tmp/spdlog-build"
            rm -rf "${spdlog_dir}"
            mkdir -p "${spdlog_dir}"
            git clone --depth 1 https://github.com/gabime/spdlog.git "${spdlog_dir}/spdlog"
            mkdir -p "${spdlog_dir}/spdlog/build"
            cmake -S "${spdlog_dir}/spdlog" -B "${spdlog_dir}/spdlog/build" \
                -DCMAKE_INSTALL_PREFIX=/usr/local \
                -DCMAKE_BUILD_TYPE=Release
            cmake --build "${spdlog_dir}/spdlog/build" -j"$(nproc)"
            cmake --install "${spdlog_dir}/spdlog/build"
            rm -rf "${spdlog_dir}"
            ldconfig
            log_ok "spdlog installed"
        fi

        # Google Test
        if ! pkg-config --exists gtest 2>/dev/null; then
            log_info "Installing Google Test..."
            local gtest_dir="/tmp/gtest-build"
            rm -rf "${gtest_dir}"
            mkdir -p "${gtest_dir}"
            git clone --depth 1 https://github.com/google/googletest.git "${gtest_dir}/googletest"
            mkdir -p "${gtest_dir}/googletest/build"
            cmake -S "${gtest_dir}/googletest" -B "${gtest_dir}/googletest/build" \
                -DCMAKE_INSTALL_PREFIX=/usr/local
            cmake --build "${gtest_dir}/googletest/build" -j"$(nproc)"
            cmake --install "${gtest_dir}/googletest/build"
            rm -rf "${gtest_dir}"
            ldconfig
            log_ok "Google Test installed"
        fi
    fi

    log_info "========================================"
    log_ok "Dependency installation completed"
    log_info "========================================"

    # Print summary
    echo ""
    log_info "Installed versions:"
    cmake --version 2>/dev/null | head -1 | xargs log_info "  "
    gcc --version 2>/dev/null | head -1 | xargs log_info "  "
    node --version 2>/dev/null | xargs log_info "  Node.js: "
    npm --version 2>/dev/null | xargs log_info "  npm: "
    pkg-config --modversion drogon 2>/dev/null | xargs log_info "  Drogon: "
}

main "$@"
