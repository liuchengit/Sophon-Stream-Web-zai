#pragma once

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>
#include <sqlite3.h>

namespace sophon_stream {
namespace web {
namespace database {

/**
 * Generic query result row - maps column name to value string.
 */
using Row = std::unordered_map<std::string, std::string>;

/**
 * Query result set.
 */
struct QueryResult {
    bool success = false;
    std::string errorMessage;
    std::vector<Row> rows;
    int64_t lastInsertId = 0;
    int64_t affectedRows = 0;
};

/**
 * Thread-safe SQLite database manager singleton.
 * Handles initialization, migrations, and CRUD operations.
 */
class DbManager {
public:
    /**
     * Get the singleton instance.
     */
    static DbManager& getInstance();

    /**
     * Initialize the database connection and create tables.
     * @param dbPath Path to the SQLite database file.
     * @return true on success.
     */
    bool init(const std::string& dbPath);

    /**
     * Close the database connection.
     */
    void close();

    /**
     * Check if the database is initialized and connected.
     */
    bool isConnected() const;

    /**
     * Execute a raw SQL statement (no results expected).
     * @param sql The SQL statement.
     * @return QueryResult with success status.
     */
    QueryResult execute(const std::string& sql);

    /**
     * Execute a parameterized SQL statement.
     * @param sql The SQL statement with ? placeholders.
     * @param params The parameter values.
     * @return QueryResult with success status.
     */
    QueryResult execute(const std::string& sql, const std::vector<std::string>& params);

    /**
     * Execute a raw SQL query and return results.
     * @param sql The SELECT SQL statement.
     * @return QueryResult with rows.
     */
    QueryResult query(const std::string& sql);

    /**
     * Execute a parameterized SQL query and return results.
     * @param sql The SELECT SQL statement with ? placeholders.
     * @param params The parameter values.
     * @return QueryResult with rows.
     */
    QueryResult query(const std::string& sql, const std::vector<std::string>& params);

    /**
     * Insert a record into a table.
     * @param table Table name.
     * @param data Map of column names to values.
     * @return QueryResult with lastInsertId.
     */
    QueryResult insert(const std::string& table, const std::unordered_map<std::string, std::string>& data);

    /**
     * Update records in a table.
     * @param table Table name.
     * @param data Map of column names to new values.
     * @param whereClause WHERE clause (without "WHERE").
     * @param params WHERE clause parameters.
     * @return QueryResult with affectedRows.
     */
    QueryResult update(const std::string& table,
                       const std::unordered_map<std::string, std::string>& data,
                       const std::string& whereClause,
                       const std::vector<std::string>& params = {});

    /**
     * Delete records from a table.
     * @param table Table name.
     * @param whereClause WHERE clause (without "WHERE").
     * @param params WHERE clause parameters.
     * @return QueryResult with affectedRows.
     */
    QueryResult remove(const std::string& table,
                       const std::string& whereClause,
                       const std::vector<std::string>& params = {});

    /**
     * Find a single record by ID.
     * @param table Table name.
     * @param id Record ID.
     * @return QueryResult with at most one row.
     */
    QueryResult findById(const std::string& table, int64_t id);

    /**
     * Find records with optional filtering and pagination.
     * @param table Table name.
     * @param whereClause Optional WHERE clause.
     * @param params WHERE clause parameters.
     * @param orderBy Optional ORDER BY clause.
     * @param limit Optional limit.
     * @param offset Optional offset.
     * @return QueryResult with rows.
     */
    QueryResult find(const std::string& table,
                     const std::string& whereClause = "",
                     const std::vector<std::string>& params = {},
                     const std::string& orderBy = "",
                     int limit = 0,
                     int offset = 0);

    /**
     * Count records in a table.
     * @param table Table name.
     * @param whereClause Optional WHERE clause.
     * @param params WHERE clause parameters.
     * @return Number of matching records, or -1 on error.
     */
    int64_t count(const std::string& table,
                  const std::string& whereClause = "",
                  const std::vector<std::string>& params = {});

    /**
     * Check if a record exists.
     * @param table Table name.
     * @param whereClause WHERE clause.
     * @param params WHERE clause parameters.
     * @return true if at least one matching record exists.
     */
    bool exists(const std::string& table,
                const std::string& whereClause,
                const std::vector<std::string>& params = {});

    /**
     * Execute a transaction.
     * @param func Function containing transaction operations.
     * @return true if transaction committed successfully.
     */
    bool transaction(const std::function<bool()>& func);

    /**
     * Create all application tables (migrations).
     * @return true on success.
     */
    bool createTables();

    /**
     * Seed initial data (default admin user, system config, etc.).
     * @return true on success.
     */
    bool seedData();

    /**
     * Get the raw SQLite database handle (use with caution).
     * @return Pointer to sqlite3 handle.
     */
    sqlite3* getHandle() const { return db_; }

private:
    DbManager() = default;
    ~DbManager();
    DbManager(const DbManager&) = delete;
    DbManager& operator=(const DbManager&) = delete;

    /**
     * Run all migration SQL statements.
     */
    bool runMigrations();

    /**
     * Create users table.
     */
    bool createUsersTable();

    /**
     * Create devices table.
     */
    bool createDevicesTable();

    /**
     * Create algorithms table.
     */
    bool createAlgorithmsTable();

    /**
     * Create tasks table.
     */
    bool createTasksTable();

    /**
     * Create alerts table.
     */
    bool createAlertsTable();

    /**
     * Create alert_rules table.
     */
    bool createAlertRulesTable();

    /**
     * Create plugins table.
     */
    bool createPluginsTable();

    /**
     * Create system_config table.
     */
    bool createSystemConfigTable();

    /**
     * Create audit_logs table.
     */
    bool createAuditLogsTable();

    /**
     * Create firmware_versions table.
     */
    bool createFirmwareVersionsTable();

    mutable std::mutex dbMutex_;
    sqlite3* db_ = nullptr;
    std::string dbPath_;
    bool initialized_ = false;
};

}  // namespace database
}  // namespace web
}  // namespace sophon_stream
