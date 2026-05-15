#include "database/DbManager.h"
#include "utils/CryptoUtils.h"
#include <spdlog/spdlog.h>
#include <sstream>
#include <fstream>
#include <filesystem>

namespace sophon_stream {
namespace web {
namespace database {

DbManager& DbManager::getInstance() {
    static DbManager instance;
    return instance;
}

DbManager::~DbManager() {
    close();
}

bool DbManager::init(const std::string& dbPath) {
    std::unique_lock<std::mutex> lock(dbMutex_);

    if (initialized_ && db_ != nullptr) {
        spdlog::warn("DbManager: Already initialized");
        return true;
    }

    // Ensure the directory exists
    std::filesystem::path p(dbPath);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
        if (ec) {
            spdlog::error("DbManager: Failed to create database directory: {}", ec.message());
            return false;
        }
    }

    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) {
        spdlog::error("DbManager: Failed to open database: {}", sqlite3_errmsg(db_));
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }

    // Enable WAL mode for better concurrent performance
    char* errMsg = nullptr;
    rc = sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        spdlog::warn("DbManager: Failed to set WAL mode: {}", errMsg);
        sqlite3_free(errMsg);
    }

    // Enable foreign keys
    rc = sqlite3_exec(db_, "PRAGMA foreign_keys=ON;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        spdlog::warn("DbManager: Failed to enable foreign keys: {}", errMsg);
        sqlite3_free(errMsg);
    }

    // Set busy timeout to 5 seconds
    sqlite3_busy_timeout(db_, 5000);

    dbPath_ = dbPath;
    initialized_ = true;

    spdlog::info("DbManager: Database opened at {}", dbPath);

    // Create tables and seed data (release lock so inner methods can acquire it)
    lock.unlock();
    bool tablesOk = createTables();
    bool seedOk = seedData();

    if (!tablesOk) {
        spdlog::error("DbManager: Failed to create tables");
        return false;
    }

    if (!seedOk) {
        spdlog::warn("DbManager: Seed data insertion had issues (may already exist)");
    }

    return true;
}

void DbManager::close() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (db_ != nullptr) {
        sqlite3_close(db_);
        db_ = nullptr;
        initialized_ = false;
        spdlog::info("DbManager: Database connection closed");
    }
}

bool DbManager::isConnected() const {
    std::lock_guard<std::mutex> lock(dbMutex_);
    return initialized_ && db_ != nullptr;
}

QueryResult DbManager::execute(const std::string& sql) {
    return execute(sql, {});
}

QueryResult DbManager::execute(const std::string& sql, const std::vector<std::string>& params) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    QueryResult result;

    if (!initialized_ || db_ == nullptr) {
        result.success = false;
        result.errorMessage = "Database not initialized";
        spdlog::error("DbManager: Execute called on uninitialized database");
        return result;
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        result.success = false;
        result.errorMessage = sqlite3_errmsg(db_);
        spdlog::error("DbManager: SQL prepare error: {} | SQL: {}", result.errorMessage, sql);
        return result;
    }

    // Bind parameters
    for (size_t i = 0; i < params.size(); ++i) {
        rc = sqlite3_bind_text(stmt, static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            result.success = false;
            result.errorMessage = sqlite3_errmsg(db_);
            spdlog::error("DbManager: SQL bind error at param {}: {}", i + 1, result.errorMessage);
            sqlite3_finalize(stmt);
            return result;
        }
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        result.success = false;
        result.errorMessage = sqlite3_errmsg(db_);
        spdlog::error("DbManager: SQL execute error: {} | SQL: {}", result.errorMessage, sql);
        sqlite3_finalize(stmt);
        return result;
    }

    result.success = true;
    result.lastInsertId = sqlite3_last_insert_rowid(db_);
    result.affectedRows = sqlite3_changes(db_);

    sqlite3_finalize(stmt);
    return result;
}

QueryResult DbManager::query(const std::string& sql) {
    return query(sql, {});
}

QueryResult DbManager::query(const std::string& sql, const std::vector<std::string>& params) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    QueryResult result;

    if (!initialized_ || db_ == nullptr) {
        result.success = false;
        result.errorMessage = "Database not initialized";
        return result;
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        result.success = false;
        result.errorMessage = sqlite3_errmsg(db_);
        spdlog::error("DbManager: Query prepare error: {} | SQL: {}", result.errorMessage, sql);
        return result;
    }

    // Bind parameters
    for (size_t i = 0; i < params.size(); ++i) {
        rc = sqlite3_bind_text(stmt, static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            result.success = false;
            result.errorMessage = sqlite3_errmsg(db_);
            spdlog::error("DbManager: Query bind error at param {}: {}", i + 1, result.errorMessage);
            sqlite3_finalize(stmt);
            return result;
        }
    }

    // Get column names
    int colCount = sqlite3_column_count(stmt);
    std::vector<std::string> colNames(colCount);
    for (int i = 0; i < colCount; ++i) {
        colNames[i] = sqlite3_column_name(stmt, i);
    }

    // Fetch all rows
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Row row;
        for (int i = 0; i < colCount; ++i) {
            const char* val = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
            row[colNames[i]] = val ? val : "";
        }
        result.rows.push_back(std::move(row));
    }

    if (rc != SQLITE_DONE) {
        result.success = false;
        result.errorMessage = sqlite3_errmsg(db_);
        spdlog::error("DbManager: Query step error: {}", result.errorMessage);
        sqlite3_finalize(stmt);
        return result;
    }

    result.success = true;
    result.lastInsertId = sqlite3_last_insert_rowid(db_);
    result.affectedRows = sqlite3_changes(db_);

    sqlite3_finalize(stmt);
    return result;
}

QueryResult DbManager::insert(const std::string& table,
                               const std::unordered_map<std::string, std::string>& data) {
    if (data.empty()) {
        QueryResult result;
        result.success = false;
        result.errorMessage = "No data provided for insert";
        return result;
    }

    std::ostringstream cols, vals;
    std::vector<std::string> params;

    cols << "INSERT INTO " << table << " (";
    vals << "VALUES (";

    bool first = true;
    for (const auto& [col, val] : data) {
        if (!first) {
            cols << ", ";
            vals << ", ";
        }
        cols << col;
        vals << "?";
        params.push_back(val);
        first = false;
    }

    cols << ") ";
    vals << ")";

    std::string sql = cols.str() + vals.str();
    return execute(sql, params);
}

QueryResult DbManager::update(const std::string& table,
                               const std::unordered_map<std::string, std::string>& data,
                               const std::string& whereClause,
                               const std::vector<std::string>& params) {
    if (data.empty()) {
        QueryResult result;
        result.success = false;
        result.errorMessage = "No data provided for update";
        return result;
    }

    std::ostringstream sql;
    std::vector<std::string> allParams;

    sql << "UPDATE " << table << " SET ";

    bool first = true;
    for (const auto& [col, val] : data) {
        if (!first) sql << ", ";
        sql << col << " = ?";
        allParams.push_back(val);
        first = false;
    }

    // Always update the updated_at field
    sql << ", updated_at = datetime('now','localtime')";

    if (!whereClause.empty()) {
        sql << " WHERE " << whereClause;
        allParams.insert(allParams.end(), params.begin(), params.end());
    }

    return execute(sql.str(), allParams);
}

QueryResult DbManager::remove(const std::string& table,
                               const std::string& whereClause,
                               const std::vector<std::string>& params) {
    std::ostringstream sql;
    sql << "DELETE FROM " << table;

    if (!whereClause.empty()) {
        sql << " WHERE " << whereClause;
    }

    return execute(sql.str(), params);
}

QueryResult DbManager::findById(const std::string& table, int64_t id) {
    std::string sql = "SELECT * FROM " + table + " WHERE id = ?";
    return query(sql, {std::to_string(id)});
}

QueryResult DbManager::find(const std::string& table,
                             const std::string& whereClause,
                             const std::vector<std::string>& params,
                             const std::string& orderBy,
                             int limit,
                             int offset) {
    std::ostringstream sql;
    sql << "SELECT * FROM " << table;

    if (!whereClause.empty()) {
        sql << " WHERE " << whereClause;
    }

    if (!orderBy.empty()) {
        sql << " ORDER BY " << orderBy;
    }

    if (limit > 0) {
        sql << " LIMIT " << limit;
    }

    if (offset > 0) {
        sql << " OFFSET " << offset;
    }

    return query(sql.str(), params);
}

int64_t DbManager::count(const std::string& table,
                          const std::string& whereClause,
                          const std::vector<std::string>& params) {
    std::ostringstream sql;
    sql << "SELECT COUNT(*) AS cnt FROM " << table;

    if (!whereClause.empty()) {
        sql << " WHERE " << whereClause;
    }

    auto result = query(sql.str(), params);
    if (!result.success || result.rows.empty()) {
        return -1;
    }

    try {
        return std::stoll(result.rows[0].at("cnt"));
    } catch (...) {
        return -1;
    }
}

bool DbManager::exists(const std::string& table,
                        const std::string& whereClause,
                        const std::vector<std::string>& params) {
    return count(table, whereClause, params) > 0;
}

bool DbManager::transaction(const std::function<bool()>& func) {
    std::lock_guard<std::mutex> lock(dbMutex_);

    if (!initialized_ || db_ == nullptr) {
        spdlog::error("DbManager: Transaction called on uninitialized database");
        return false;
    }

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        spdlog::error("DbManager: Failed to begin transaction: {}", errMsg);
        sqlite3_free(errMsg);
        return false;
    }

    bool success = false;
    try {
        success = func();
    } catch (const std::exception& e) {
        spdlog::error("DbManager: Transaction function threw exception: {}", e.what());
        success = false;
    }

    if (success) {
        rc = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            spdlog::error("DbManager: Failed to commit transaction: {}", errMsg);
            sqlite3_free(errMsg);
            sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
            return false;
        }
        return true;
    } else {
        rc = sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            spdlog::error("DbManager: Failed to rollback transaction: {}", errMsg);
            sqlite3_free(errMsg);
        }
        return false;
    }
}

bool DbManager::createTables() {
    spdlog::info("DbManager: Creating database tables...");

    if (!createUsersTable()) return false;
    if (!createDevicesTable()) return false;
    if (!createAlgorithmsTable()) return false;
    if (!createTasksTable()) return false;
    if (!createAlertsTable()) return false;
    if (!createAlertRulesTable()) return false;
    if (!createPluginsTable()) return false;
    if (!createSystemConfigTable()) return false;
    if (!createAuditLogsTable()) return false;
    if (!createFirmwareVersionsTable()) return false;

    spdlog::info("DbManager: All tables created successfully");
    return true;
}

bool DbManager::createUsersTable() {
    const char* createSql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL UNIQUE,
            password_hash TEXT NOT NULL,
            salt TEXT NOT NULL,
            role TEXT NOT NULL DEFAULT 'operator',
            real_name TEXT,
            email TEXT,
            phone TEXT,
            status INTEGER NOT NULL DEFAULT 1,
            last_login_at TEXT,
            created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
            updated_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
        )
    )";

    auto result = execute(createSql);
    if (!result.success) {
        spdlog::error("DbManager: Failed to create users table: {}", result.errorMessage);
        return false;
    }
    execute("CREATE INDEX IF NOT EXISTS idx_users_username ON users(username)");
    execute("CREATE INDEX IF NOT EXISTS idx_users_role ON users(role)");
    execute("CREATE INDEX IF NOT EXISTS idx_users_status ON users(status)");
    return true;
}

bool DbManager::createDevicesTable() {
    const char* createSql = R"(
        CREATE TABLE IF NOT EXISTS devices (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            type TEXT NOT NULL,
            stream_url TEXT NOT NULL,
            protocol TEXT,
            resolution TEXT,
            fps REAL DEFAULT 25.0,
            codec TEXT DEFAULT 'H.264',
            status INTEGER NOT NULL DEFAULT 0,
            location TEXT,
            description TEXT,
            device_id TEXT,
            last_heartbeat TEXT,
            created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
            updated_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
        )
    )";

    auto result = execute(createSql);
    if (!result.success) {
        spdlog::error("DbManager: Failed to create devices table: {}", result.errorMessage);
        return false;
    }
    execute("CREATE INDEX IF NOT EXISTS idx_devices_type ON devices(type)");
    execute("CREATE INDEX IF NOT EXISTS idx_devices_status ON devices(status)");
    execute("CREATE INDEX IF NOT EXISTS idx_devices_device_id ON devices(device_id)");
    return true;
}

bool DbManager::createAlgorithmsTable() {
    const char* createSql = R"(
        CREATE TABLE IF NOT EXISTS algorithms (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            type TEXT NOT NULL,
            model_path TEXT NOT NULL,
            config_json TEXT,
            version TEXT,
            description TEXT,
            input_size TEXT,
            classes TEXT,
            status INTEGER NOT NULL DEFAULT 1,
            created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
            updated_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
        )
    )";

    auto result = execute(createSql);
    if (!result.success) {
        spdlog::error("DbManager: Failed to create algorithms table: {}", result.errorMessage);
        return false;
    }
    execute("CREATE INDEX IF NOT EXISTS idx_algorithms_type ON algorithms(type)");
    execute("CREATE INDEX IF NOT EXISTS idx_algorithms_status ON algorithms(status)");
    return true;
}

bool DbManager::createTasksTable() {
    const char* createSql = R"(
        CREATE TABLE IF NOT EXISTS tasks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            type TEXT NOT NULL,
            status INTEGER NOT NULL DEFAULT 0,
            device_id INTEGER NOT NULL,
            algorithm_ids TEXT NOT NULL,
            graph_config TEXT,
            schedule_config TEXT,
            roi_config TEXT,
            osd_enabled INTEGER DEFAULT 1,
            record_enabled INTEGER DEFAULT 0,
            output_url TEXT,
            description TEXT,
            started_at TEXT,
            stopped_at TEXT,
            created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
            updated_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
            FOREIGN KEY (device_id) REFERENCES devices(id)
        )
    )";

    auto result = execute(createSql);
    if (!result.success) {
        spdlog::error("DbManager: Failed to create tasks table: {}", result.errorMessage);
        return false;
    }
    execute("CREATE INDEX IF NOT EXISTS idx_tasks_status ON tasks(status)");
    execute("CREATE INDEX IF NOT EXISTS idx_tasks_device_id ON tasks(device_id)");
    execute("CREATE INDEX IF NOT EXISTS idx_tasks_type ON tasks(type)");
    return true;
}

bool DbManager::createAlertsTable() {
    const char* createSql = R"(
        CREATE TABLE IF NOT EXISTS alerts (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            task_id INTEGER NOT NULL,
            rule_id INTEGER,
            type TEXT NOT NULL,
            level INTEGER NOT NULL DEFAULT 1,
            message TEXT NOT NULL,
            evidence_path TEXT,
            metadata TEXT,
            acknowledged INTEGER DEFAULT 0,
            created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
            FOREIGN KEY (task_id) REFERENCES tasks(id)
        )
    )";

    auto result = execute(createSql);
    if (!result.success) {
        spdlog::error("DbManager: Failed to create alerts table: {}", result.errorMessage);
        return false;
    }
    execute("CREATE INDEX IF NOT EXISTS idx_alerts_task_id ON alerts(task_id)");
    execute("CREATE INDEX IF NOT EXISTS idx_alerts_type ON alerts(type)");
    execute("CREATE INDEX IF NOT EXISTS idx_alerts_level ON alerts(level)");
    execute("CREATE INDEX IF NOT EXISTS idx_alerts_acknowledged ON alerts(acknowledged)");
    execute("CREATE INDEX IF NOT EXISTS idx_alerts_created_at ON alerts(created_at)");
    return true;
}

bool DbManager::createAlertRulesTable() {
    const char* createSql = R"(
        CREATE TABLE IF NOT EXISTS alert_rules (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            task_id INTEGER NOT NULL,
            type TEXT NOT NULL,
            condition_json TEXT NOT NULL,
            debounce_ms INTEGER DEFAULT 3000,
            enabled INTEGER DEFAULT 1,
            notify_type TEXT DEFAULT 'webhook',
            notify_config TEXT,
            created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
            updated_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
            FOREIGN KEY (task_id) REFERENCES tasks(id)
        )
    )";

    auto result = execute(createSql);
    if (!result.success) {
        spdlog::error("DbManager: Failed to create alert_rules table: {}", result.errorMessage);
        return false;
    }
    execute("CREATE INDEX IF NOT EXISTS idx_alert_rules_task_id ON alert_rules(task_id)");
    execute("CREATE INDEX IF NOT EXISTS idx_alert_rules_enabled ON alert_rules(enabled)");
    return true;
}

bool DbManager::createPluginsTable() {
    const char* createSql = R"(
        CREATE TABLE IF NOT EXISTS plugins (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE,
            version TEXT NOT NULL,
            so_path TEXT NOT NULL,
            config_schema TEXT,
            description TEXT,
            author TEXT,
            type TEXT,
            status INTEGER DEFAULT 0,
            checksum TEXT,
            installed_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
            updated_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
        )
    )";

    auto result = execute(createSql);
    if (!result.success) {
        spdlog::error("DbManager: Failed to create plugins table: {}", result.errorMessage);
        return false;
    }
    execute("CREATE INDEX IF NOT EXISTS idx_plugins_type ON plugins(type)");
    execute("CREATE INDEX IF NOT EXISTS idx_plugins_status ON plugins(status)");
    return true;
}

bool DbManager::createSystemConfigTable() {
    const char* createSql = R"(
        CREATE TABLE IF NOT EXISTS system_config (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL,
            description TEXT,
            updated_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
        )
    )";

    auto result = execute(createSql);
    if (!result.success) {
        spdlog::error("DbManager: Failed to create system_config table: {}", result.errorMessage);
        return false;
    }
    return true;
}

bool DbManager::createAuditLogsTable() {
    const char* createSql = R"(
        CREATE TABLE IF NOT EXISTS audit_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER,
            action TEXT NOT NULL,
            resource_type TEXT NOT NULL,
            resource_id INTEGER,
            detail TEXT,
            ip_address TEXT,
            created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
            FOREIGN KEY (user_id) REFERENCES users(id)
        )
    )";

    auto result = execute(createSql);
    if (!result.success) {
        spdlog::error("DbManager: Failed to create audit_logs table: {}", result.errorMessage);
        return false;
    }
    execute("CREATE INDEX IF NOT EXISTS idx_audit_logs_user_id ON audit_logs(user_id)");
    execute("CREATE INDEX IF NOT EXISTS idx_audit_logs_action ON audit_logs(action)");
    execute("CREATE INDEX IF NOT EXISTS idx_audit_logs_resource_type ON audit_logs(resource_type)");
    execute("CREATE INDEX IF NOT EXISTS idx_audit_logs_created_at ON audit_logs(created_at)");
    return true;
}

bool DbManager::createFirmwareVersionsTable() {
    const char* createSql = R"(
        CREATE TABLE IF NOT EXISTS firmware_versions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            version TEXT NOT NULL,
            type TEXT NOT NULL,
            package_url TEXT,
            checksum TEXT,
            size INTEGER,
            description TEXT,
            status INTEGER DEFAULT 0,
            installed_at TEXT,
            created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
        )
    )";

    auto result = execute(createSql);
    if (!result.success) {
        spdlog::error("DbManager: Failed to create firmware_versions table: {}", result.errorMessage);
        return false;
    }
    execute("CREATE INDEX IF NOT EXISTS idx_firmware_versions_type ON firmware_versions(type)");
    execute("CREATE INDEX IF NOT EXISTS idx_firmware_versions_status ON firmware_versions(status)");
    return true;
}

bool DbManager::seedData() {
    spdlog::info("DbManager: Seeding initial data...");

    // Check if admin user already exists
    if (exists("users", "username = ?", {"admin"})) {
        spdlog::info("DbManager: Admin user already exists, skipping seed");
        return true;
    }

    // Create default admin user
    // Password: admin123 (should be changed in production)
    std::string salt = utils::CryptoUtils::generateSalt(32);
    std::string passwordHash = utils::CryptoUtils::hashPassword("admin123", salt);

    auto result = insert("users", {
        {"username", "admin"},
        {"password_hash", passwordHash},
        {"salt", salt},
        {"role", "admin"},
        {"real_name", "System Administrator"},
        {"email", "admin@sophon-stream.local"},
        {"status", "1"}
    });

    if (!result.success) {
        spdlog::warn("DbManager: Failed to seed admin user: {}", result.errorMessage);
        // Not a fatal error, may already exist
        return true;
    }

    spdlog::info("DbManager: Default admin user created (username: admin, password: admin123)");

    // Seed system configuration
    std::vector<std::pair<std::string, std::string>> configItems = {
        {"system.name", "Sophon-Stream Web Management System"},
        {"system.version", "1.0.0"},
        {"system.max_tasks", "100"},
        {"system.max_devices", "500"},
        {"system.alert_retention_days", "90"},
        {"system.audit_retention_days", "180"},
        {"system.auto_backup", "true"},
        {"system.backup_path", "./backups"},
        {"monitor.tpu_enabled", "true"},
        {"monitor.broadcast_interval_ms", "2000"},
    };

    for (const auto& [key, value] : configItems) {
        if (!exists("system_config", "key = ?", {key})) {
            insert("system_config", {
                {"key", key},
                {"value", value},
                {"description", "Auto-seeded configuration"}
            });
        }
    }

    spdlog::info("DbManager: System configuration seeded");
    return true;
}

bool DbManager::runMigrations() {
    // Future migration support
    // Each migration would be versioned and applied sequentially
    spdlog::info("DbManager: Running migrations...");
    return createTables();
}

}  // namespace database
}  // namespace web
}  // namespace sophon_stream
