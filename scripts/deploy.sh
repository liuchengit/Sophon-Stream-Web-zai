#!/bin/bash
# ============================================================================
# Sophon-Stream Web Management System Deployment Script
# ============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# ── Defaults ────────────────────────────────────────────────────────────────
INSTALL_DIR="/opt/sophon-stream-web"
SERVICE_NAME="sophon-stream-web"
SERVICE_USER="sophon"
HEALTH_CHECK_URL="http://localhost:8080/api/v1/health"
HEALTH_CHECK_RETRIES=30
HEALTH_CHECK_INTERVAL=2
BACKUP_DIR="/opt/sophon-stream-web/backups"
ROLLBACK=false

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
Usage: $(basename "$0") [OPTIONS] <package_tarball>

Sophon-Stream Web Management System Deployment Script

Options:
  --install-dir <dir>    Installation directory (default: /opt/sophon-stream-web)
  --user <user>          Service user (default: sophon)
  --rollback             Rollback to previous version
  -h, --help             Show this help message

Examples:
  # Deploy from tarball
  $(basename "$0") sophon-stream-web-1.0.0-x86_64.tar.gz

  # Rollback to previous version
  $(basename "$0") --rollback

  # Custom install directory
  $(basename "$0") --install-dir /home/sophon/sophon-stream-web package.tar.gz

EOF
    exit 0
}

# ── Parse arguments ─────────────────────────────────────────────────────────
PACKAGE_FILE=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --install-dir)
            INSTALL_DIR="$2"
            shift 2
            ;;
        --user)
            SERVICE_USER="$2"
            shift 2
            ;;
        --rollback)
            ROLLBACK=true
            shift
            ;;
        -h|--help)
            usage
            ;;
        *)
            PACKAGE_FILE="$1"
            shift
            ;;
    esac
done

# ── Check root ──────────────────────────────────────────────────────────────
check_root() {
    if [[ "$(id -u)" -ne 0 ]]; then
        log_error "This script must be run as root (or with sudo)"
        exit 1
    fi
}

# ── Ensure service user exists ─────────────────────────────────────────────
ensure_user() {
    if ! id -u "${SERVICE_USER}" &>/dev/null; then
        log_info "Creating service user: ${SERVICE_USER}"
        useradd --system --no-create-home --shell /usr/sbin/nologin "${SERVICE_USER}"
        log_ok "User ${SERVICE_USER} created"
    fi
}

# ── Stop existing service ──────────────────────────────────────────────────
stop_service() {
    log_info "Stopping existing service..."
    if systemctl is-active --quiet "${SERVICE_NAME}" 2>/dev/null; then
        systemctl stop "${SERVICE_NAME}"
        log_ok "Service stopped"
    else
        log_warn "Service is not running"
    fi
}

# ── Backup current installation ────────────────────────────────────────────
backup_current() {
    if [[ ! -d "${INSTALL_DIR}/bin" ]]; then
        log_warn "No existing installation found at ${INSTALL_DIR}, skipping backup"
        return 0
    fi

    local timestamp
    timestamp=$(date +%Y%m%d_%H%M%S)
    local backup_path="${BACKUP_DIR}/${SERVICE_NAME}_${timestamp}"

    log_info "Backing up current installation to ${backup_path}..."
    mkdir -p "${BACKUP_DIR}"
    cp -a "${INSTALL_DIR}/bin" "${backup_path}/"
    cp -a "${INSTALL_DIR}/config" "${backup_path}/"
    cp -a "${INSTALL_DIR}/frontend" "${backup_path}/" 2>/dev/null || true

    # Record backup path for potential rollback
    echo "${backup_path}" > "${BACKUP_DIR}/last_backup"

    log_ok "Backup created: ${backup_path}"

    # Keep only last 5 backups
    local backup_count
    backup_count=$(ls -1d "${BACKUP_DIR}/${SERVICE_NAME}_"* 2>/dev/null | wc -l)
    if [[ "${backup_count}" -gt 5 ]]; then
        log_info "Cleaning old backups (keeping last 5)..."
        ls -1d "${BACKUP_DIR}/${SERVICE_NAME}_"* | sort | head -n -5 | xargs rm -rf
    fi
}

# ── Deploy new files ───────────────────────────────────────────────────────
deploy_files() {
    if [[ -z "${PACKAGE_FILE}" ]]; then
        log_error "No package file specified"
        usage
    fi

    if [[ ! -f "${PACKAGE_FILE}" ]]; then
        log_error "Package file not found: ${PACKAGE_FILE}"
        exit 1
    fi

    log_info "Deploying new files from ${PACKAGE_FILE}..."

    # Create install directory structure
    mkdir -p "${INSTALL_DIR}"/{bin,config,frontend,data,logs,plugins,migrations}

    # Extract package
    tar -xzf "${PACKAGE_FILE}" -C "${INSTALL_DIR}/"

    # Ensure binary is executable
    chmod +x "${INSTALL_DIR}/bin/sophon-stream-web" 2>/dev/null || true

    # Set ownership
    chown -R "${SERVICE_USER}:${SERVICE_USER}" "${INSTALL_DIR}"

    log_ok "Files deployed to ${INSTALL_DIR}"
}

# ── Run database migrations ────────────────────────────────────────────────
run_migrations() {
    log_info "Running database migrations..."

    local db_path="${INSTALL_DIR}/data/sophon_stream.db"
    local migration_dir="${INSTALL_DIR}/migrations"

    if [[ ! -d "${migration_dir}" ]]; then
        log_warn "No migrations directory found, skipping"
        return 0
    fi

    # Create migrations tracking table if not exists
    if [[ -f "${db_path}" ]]; then
        sqlite3 "${db_path}" "CREATE TABLE IF NOT EXISTS schema_migrations (
            version TEXT PRIMARY KEY,
            applied_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
        );"

        # Apply each migration in order
        for migration_file in $(ls "${migration_dir}"/V*.sql 2>/dev/null | sort); do
            local version
            version=$(basename "${migration_file}" | sed 's/V\([0-9_]*\)__.*/\1/')

            # Check if migration already applied
            local already_applied
            already_applied=$(sqlite3 "${db_path}" "SELECT COUNT(*) FROM schema_migrations WHERE version = '${version}';")

            if [[ "${already_applied}" -eq 0 ]]; then
                log_info "Applying migration: $(basename "${migration_file}")"
                sqlite3 "${db_path}" < "${migration_file}"

                if [[ $? -eq 0 ]]; then
                    sqlite3 "${db_path}" "INSERT INTO schema_migrations (version) VALUES ('${version}');"
                    log_ok "Migration applied: $(basename "${migration_file}")"
                else
                    log_error "Migration failed: $(basename "${migration_file}")"
                    return 1
                fi
            else
                log_info "Migration already applied: $(basename "${migration_file}")"
            fi
        done
    else
        log_info "No existing database found, will be created on first run"
    fi

    log_ok "Database migrations completed"
}

# ── Install and start service ──────────────────────────────────────────────
start_service() {
    log_info "Installing systemd service..."

    # Install service file
    if [[ -f "${INSTALL_DIR}/scripts/sophon-stream-web.service" ]]; then
        cp "${INSTALL_DIR}/scripts/sophon-stream-web.service" /etc/systemd/system/
    elif [[ -f "${SCRIPT_DIR}/sophon-stream-web.service" ]]; then
        cp "${SCRIPT_DIR}/sophon-stream-web.service" /etc/systemd/system/
    fi

    # Update service file with actual paths
    sed -i "s|/opt/sophon-stream-web|${INSTALL_DIR}|g" /etc/systemd/system/"${SERVICE_NAME}".service
    sed -i "s|User=sophon|User=${SERVICE_USER}|g" /etc/systemd/system/"${SERVICE_NAME}".service

    # Reload systemd
    systemctl daemon-reload
    systemctl enable "${SERVICE_NAME}"

    # Start service
    log_info "Starting service..."
    systemctl start "${SERVICE_NAME}"

    log_ok "Service installed and started"
}

# ── Health check ───────────────────────────────────────────────────────────
health_check() {
    log_info "Running health check..."

    local attempt=1
    while [[ ${attempt} -le ${HEALTH_CHECK_RETRIES} ]]; do
        if curl -sf "${HEALTH_CHECK_URL}" > /dev/null 2>&1; then
            log_ok "Health check passed (attempt ${attempt}/${HEALTH_CHECK_RETRIES})"
            return 0
        fi

        log_info "Waiting for service... (attempt ${attempt}/${HEALTH_CHECK_RETRIES})"
        sleep "${HEALTH_CHECK_INTERVAL}"
        attempt=$((attempt + 1))
    done

    log_error "Health check failed after ${HEALTH_CHECK_RETRIES} attempts"
    return 1
}

# ── Rollback ───────────────────────────────────────────────────────────────
do_rollback() {
    log_warn "Rolling back to previous version..."

    if [[ ! -f "${BACKUP_DIR}/last_backup" ]]; then
        log_error "No backup found for rollback"
        exit 1
    fi

    local last_backup
    last_backup=$(cat "${BACKUP_DIR}/last_backup")

    if [[ ! -d "${last_backup}" ]]; then
        log_error "Backup directory not found: ${last_backup}"
        exit 1
    fi

    stop_service

    log_info "Restoring from ${last_backup}..."
    cp -a "${last_backup}/bin" "${INSTALL_DIR}/"
    cp -a "${last_backup}/config" "${INSTALL_DIR}/"
    cp -a "${last_backup}/frontend" "${INSTALL_DIR}/" 2>/dev/null || true

    chown -R "${SERVICE_USER}:${SERVICE_USER}" "${INSTALL_DIR}"

    start_service
    health_check

    log_ok "Rollback completed"
    exit 0
}

# ── Main ───────────────────────────────────────────────────────────────────
main() {
    log_info "Sophon-Stream Web Management System Deployment"
    log_info "================================================"

    check_root

    if [[ "${ROLLBACK}" == true ]]; then
        do_rollback
        exit 0
    fi

    ensure_user
    stop_service
    backup_current

    if ! deploy_files; then
        log_error "Deployment failed, initiating rollback..."
        do_rollback
        exit 1
    fi

    run_migrations
    start_service

    if ! health_check; then
        log_error "Health check failed, initiating rollback..."
        stop_service
        do_rollback
        exit 1
    fi

    log_info "================================================"
    log_ok "Deployment completed successfully"
    log_info "Service URL: ${HEALTH_CHECK_URL}"
    log_info "Config: ${INSTALL_DIR}/config/config.json"
    log_info "Logs: journalctl -u ${SERVICE_NAME} -f"
    log_info "================================================"
}

main "$@"
