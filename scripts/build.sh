#!/bin/bash
# ============================================================================
# Sophon-Stream Web Management System Build Script
# Supports: BM1684/BM1684X/BM1688 (PCIe/SoC)
# ============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

# ── Defaults ────────────────────────────────────────────────────────────────
TARGET_ARCH="$(uname -m)"        # x86_64 / aarch64
BUILD_TYPE="Release"
CLEAN_BUILD=false
FRONTEND_ONLY=false
BACKEND_ONLY=false
JOBS="$(nproc 2>/dev/null || echo 4)"
TOOLCHAIN_FILE=""
VERSION="$(cat "${PROJECT_DIR}/VERSION" 2>/dev/null || echo "1.0.0")"

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

# ── Usage ───────────────────────────────────────────────────────────────────
usage() {
    cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Sophon-Stream Web Management System Build Script
Version: ${VERSION}

Options:
  --target <arch>       Target architecture: x86_64 (default), aarch64
  --type <type>         Build type: Release (default), Debug
  --clean               Clean build directory before building
  --frontend-only       Build frontend only
  --backend-only        Build backend only
  --toolchain <file>    CMake toolchain file for cross-compilation
  -j <jobs>             Number of parallel jobs (default: ${JOBS})
  -h, --help            Show this help message

Cross-compilation examples:
  # Build for BM1684X SoC (aarch64)
  $(basename "$0") --target aarch64 --toolchain cmake/BM1684X-toolchain.cmake

  # Build for BM1688 SoC (aarch64)
  $(basename "$0") --target aarch64 --toolchain cmake/BM1688-toolchain.cmake

  # Debug build with clean
  $(basename "$0") --type Debug --clean

EOF
    exit 0
}

# ── Parse arguments ─────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --target)
            TARGET_ARCH="$2"
            shift 2
            ;;
        --type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --frontend-only)
            FRONTEND_ONLY=true
            shift
            ;;
        --backend-only)
            BACKEND_ONLY=true
            shift
            ;;
        --toolchain)
            TOOLCHAIN_FILE="$2"
            shift 2
            ;;
        -j)
            JOBS="$2"
            shift 2
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

# ── Detect host architecture ────────────────────────────────────────────────
HOST_ARCH="$(uname -m)"
log_info "Host architecture: ${HOST_ARCH}"
log_info "Target architecture: ${TARGET_ARCH}"
log_info "Build type: ${BUILD_TYPE}"
log_info "Version: ${VERSION}"

# ── Resolve toolchain ──────────────────────────────────────────────────────
if [[ -n "${TOOLCHAIN_FILE}" ]]; then
    if [[ ! -f "${TOOLCHAIN_FILE}" ]]; then
        # Try relative to project dir
        TOOLCHAIN_FILE="${PROJECT_DIR}/${TOOLCHAIN_FILE}"
    fi
    if [[ ! -f "${TOOLCHAIN_FILE}" ]]; then
        log_error "Toolchain file not found: ${TOOLCHAIN_FILE}"
        exit 1
    fi
    log_info "Using toolchain: ${TOOLCHAIN_FILE}"
fi

# ── Build directories ──────────────────────────────────────────────────────
BUILD_DIR="${PROJECT_DIR}/build"
DIST_DIR="${PROJECT_DIR}/dist"
FRONTEND_DIR="${PROJECT_DIR}/frontend"
BACKEND_DIR="${PROJECT_DIR}/backend"

# ── Clean ──────────────────────────────────────────────────────────────────
if [[ "${CLEAN_BUILD}" == true ]]; then
    log_info "Cleaning build directories..."
    rm -rf "${BUILD_DIR}" "${DIST_DIR}"
    log_ok "Clean completed"
fi

mkdir -p "${BUILD_DIR}" "${DIST_DIR}"

# ── Build backend ──────────────────────────────────────────────────────────
build_backend() {
    log_info "=========================================="
    log_info "Building backend (C++ / CMake)"
    log_info "=========================================="

    local backend_build_dir="${BUILD_DIR}/backend"
    mkdir -p "${backend_build_dir}"

    local cmake_args=(
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
        -DSOPHON_STREAM_INCLUDE_DIR="/opt/sophon-stream/include"
        -DSOPHON_STREAM_LIB_DIR="/opt/sophon-stream/lib"
    )

    if [[ -n "${TOOLCHAIN_FILE}" ]]; then
        cmake_args+=("-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE}")
    fi

    if [[ "${BUILD_TYPE}" == "Debug" ]]; then
        cmake_args+=("-DBUILD_TYPE_RELEASE=OFF")
    fi

    log_info "Running CMake configure..."
    cmake -S "${BACKEND_DIR}" -B "${backend_build_dir}" "${cmake_args[@]}"

    log_info "Building backend (${JOBS} jobs)..."
    cmake --build "${backend_build_dir}" -j"${JOBS}"

    # Copy binary to dist
    mkdir -p "${DIST_DIR}/bin"
    if [[ -f "${backend_build_dir}/sophon-stream-web" ]]; then
        cp "${backend_build_dir}/sophon-stream-web" "${DIST_DIR}/bin/"
        chmod +x "${DIST_DIR}/bin/sophon-stream-web"
        log_ok "Backend binary copied to ${DIST_DIR}/bin/"
    fi

    # Copy config
    mkdir -p "${DIST_DIR}/config"
    if [[ -f "${BACKEND_DIR}/config/config.json" ]]; then
        cp "${BACKEND_DIR}/config/config.json" "${DIST_DIR}/config/"
        log_ok "Config copied to ${DIST_DIR}/config/"
    fi

    log_ok "Backend build completed"
}

# ── Build frontend ─────────────────────────────────────────────────────────
build_frontend() {
    log_info "=========================================="
    log_info "Building frontend (Vue3 / Vite)"
    log_info "=========================================="

    if [[ ! -f "${FRONTEND_DIR}/package.json" ]]; then
        log_error "Frontend package.json not found at ${FRONTEND_DIR}"
        exit 1
    fi

    cd "${FRONTEND_DIR}"

    # Install dependencies
    if [[ ! -d "node_modules" ]]; then
        log_info "Installing frontend dependencies..."
        npm install
    else
        log_info "Checking for dependency updates..."
        npm ci --prefer-offline 2>/dev/null || npm install
    fi

    # Build
    log_info "Building frontend..."
    npm run build

    # Copy to dist
    if [[ -d "${FRONTEND_DIR}/dist" ]]; then
        mkdir -p "${DIST_DIR}/frontend"
        cp -r "${FRONTEND_DIR}/dist/"* "${DIST_DIR}/frontend/"
        log_ok "Frontend assets copied to ${DIST_DIR}/frontend/"
    else
        log_error "Frontend build output not found"
        exit 1
    fi

    log_ok "Frontend build completed"
    cd "${PROJECT_DIR}"
}

# ── Package ────────────────────────────────────────────────────────────────
package_output() {
    log_info "=========================================="
    log_info "Packaging distribution"
    log_info "=========================================="

    # Create data and logs directories
    mkdir -p "${DIST_DIR}/data" "${DIST_DIR}/logs" "${DIST_DIR}/plugins"

    # Copy scripts
    mkdir -p "${DIST_DIR}/scripts"
    cp "${SCRIPT_DIR}/deploy.sh" "${DIST_DIR}/scripts/"
    chmod +x "${DIST_DIR}/scripts/deploy.sh"

    # Copy service file
    mkdir -p "${DIST_DIR}/scripts"
    cp "${SCRIPT_DIR}/sophon-stream-web.service" "${DIST_DIR}/scripts/"

    # Copy migrations
    mkdir -p "${DIST_DIR}/migrations"
    if [[ -d "${BACKEND_DIR}/src/database/migrations" ]]; then
        cp "${BACKEND_DIR}/src/database/migrations/"*.sql "${DIST_DIR}/migrations/"
    fi

    # Copy nginx config
    if [[ -f "${PROJECT_DIR}/config/nginx.conf" ]]; then
        mkdir -p "${DIST_DIR}/nginx"
        cp "${PROJECT_DIR}/config/nginx.conf" "${DIST_DIR}/nginx/"
    fi

    # Create version file
    echo "${VERSION}" > "${DIST_DIR}/VERSION"

    # Create tarball
    local pkg_name="sophon-stream-web-${VERSION}-${TARGET_ARCH}"
    local tar_file="${PROJECT_DIR}/${pkg_name}.tar.gz"

    cd "${PROJECT_DIR}"
    tar -czf "${tar_file}" -C dist .

    log_ok "Package created: ${tar_file}"
    log_ok "Distribution contents:"
    ls -la "${DIST_DIR}/"
}

# ── Main ───────────────────────────────────────────────────────────────────
main() {
    log_info "Sophon-Stream Web Management System Build"
    log_info "=========================================="

    local start_time
    start_time=$(date +%s)

    if [[ "${FRONTEND_ONLY}" == true ]]; then
        build_frontend
    elif [[ "${BACKEND_ONLY}" == true ]]; then
        build_backend
    else
        build_backend
        build_frontend
    fi

    package_output

    local end_time
    end_time=$(date +%s)
    local duration=$((end_time - start_time))

    log_info "=========================================="
    log_ok "Build completed in ${duration}s"
    log_info "=========================================="
}

main "$@"
