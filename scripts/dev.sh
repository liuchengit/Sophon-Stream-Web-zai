#!/bin/bash
# ============================================================================
# Sophon-Stream Web Management System Development Environment Script
# Starts backend in debug mode and frontend dev server with hot reload
# ============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

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
BACKEND_PORT=8080
FRONTEND_PORT=5173
NO_BACKEND=false
NO_FRONTEND=false
JOBS="$(nproc 2>/dev/null || echo 4)"

# ── Parse arguments ─────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --no-backend)
            NO_BACKEND=true
            shift
            ;;
        --no-frontend)
            NO_FRONTEND=true
            shift
            ;;
        -j)
            JOBS="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $(basename "$0") [--no-backend] [--no-frontend] [-j jobs]"
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

# ── Cleanup trap ────────────────────────────────────────────────────────────
PIDS=()
cleanup() {
    log_info "Shutting down development servers..."
    for pid in "${PIDS[@]}"; do
        if kill -0 "${pid}" 2>/dev/null; then
            kill "${pid}" 2>/dev/null || true
        fi
    done
    wait 2>/dev/null
    log_ok "Development servers stopped"
}
trap cleanup EXIT INT TERM

# ── Build backend (debug) ──────────────────────────────────────────────────
build_backend_debug() {
    local build_dir="${PROJECT_DIR}/build/debug"
    mkdir -p "${build_dir}"

    log_info "Configuring backend (Debug)..."
    cmake -S "${PROJECT_DIR}/backend" -B "${build_dir}" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DBUILD_TYPE_RELEASE=OFF

    log_info "Building backend..."
    cmake --build "${build_dir}" -j"${JOBS}"

    echo "${build_dir}"
}

# ── Start backend ──────────────────────────────────────────────────────────
start_backend() {
    log_info "Starting backend in debug mode..."

    local build_dir
    build_dir=$(build_backend_debug)

    # Ensure data directory exists
    mkdir -p "${PROJECT_DIR}/data" "${PROJECT_DIR}/logs"

    # Start backend
    "${build_dir}/sophon-stream-web" \
        --config="${PROJECT_DIR}/backend/config/config.json" &

    local backend_pid=$!
    PIDS+=("${backend_pid}")

    # Wait for backend to start
    local retries=15
    while [[ ${retries} -gt 0 ]]; do
        if curl -sf "http://localhost:${BACKEND_PORT}/api/v1/health" > /dev/null 2>&1; then
            log_ok "Backend started (PID: ${backend_pid}, port: ${BACKEND_PORT})"
            return 0
        fi
        retries=$((retries - 1))
        sleep 1
    done

    log_error "Backend failed to start within timeout"
    return 1
}

# ── Start frontend ─────────────────────────────────────────────────────────
start_frontend() {
    log_info "Starting frontend dev server..."

    local frontend_dir="${PROJECT_DIR}/frontend"

    if [[ ! -f "${frontend_dir}/package.json" ]]; then
        log_error "Frontend package.json not found"
        return 1
    fi

    cd "${frontend_dir}"

    # Install dependencies if needed
    if [[ ! -d "node_modules" ]]; then
        log_info "Installing frontend dependencies..."
        npm install
    fi

    # Start dev server
    npm run dev -- --host 0.0.0.0 --port "${FRONTEND_PORT}" &

    local frontend_pid=$!
    PIDS+=("${frontend_pid}")

    cd "${PROJECT_DIR}"

    log_ok "Frontend dev server started (PID: ${frontend_pid}, port: ${FRONTEND_PORT})"
}

# ── Watch for backend changes (optional) ───────────────────────────────────
watch_backend() {
    log_info "Watching for backend source changes..."

    local build_dir="${PROJECT_DIR}/build/debug"
    local src_dir="${PROJECT_DIR}/backend/src"

    # Use inotifywait if available
    if command -v inotifywait &>/dev/null; then
        while inotifywait -r -e modify,create,delete "${src_dir}" 2>/dev/null; do
            log_info "Backend source change detected, rebuilding..."
            if cmake --build "${build_dir}" -j"${JOBS}"; then
                log_ok "Backend rebuilt successfully"
                # Restart backend (kill old and start new)
                if [[ ${#PIDS[@]} -gt 0 ]] && kill -0 "${PIDS[0]}" 2>/dev/null; then
                    kill "${PIDS[0]}" 2>/dev/null
                    sleep 1
                    "${build_dir}/sophon-stream-web" \
                        --config="${PROJECT_DIR}/backend/config/config.json" &
                    PIDS[0]=$!
                    log_ok "Backend restarted (PID: ${PIDS[0]})"
                fi
            else
                log_error "Backend rebuild failed"
            fi
        done &
        PIDS+=($!)
    else
        log_warn "inotifywait not found, backend auto-rebuild disabled"
        log_warn "Install inotify-tools for auto-rebuild: sudo apt install inotify-tools"
    fi
}

# ── Main ───────────────────────────────────────────────────────────────────
main() {
    log_info "Sophon-Stream Web Development Environment"
    log_info "=========================================="

    if [[ "${NO_BACKEND}" == false ]]; then
        if ! start_backend; then
            log_error "Failed to start backend"
            exit 1
        fi
    fi

    if [[ "${NO_FRONTEND}" == false ]]; then
        start_frontend
    fi

    if [[ "${NO_BACKEND}" == false ]]; then
        watch_backend
    fi

    log_info "=========================================="
    log_ok "Development environment ready"
    if [[ "${NO_BACKEND}" == false ]]; then
        log_info "Backend:  http://localhost:${BACKEND_PORT}"
        log_info "API:      http://localhost:${BACKEND_PORT}/api/v1"
    fi
    if [[ "${NO_FRONTEND}" == false ]]; then
        log_info "Frontend: http://localhost:${FRONTEND_PORT}"
    fi
    log_info "Press Ctrl+C to stop all servers"
    log_info "=========================================="

    # Wait for any child process to exit
    wait
}

main "$@"
