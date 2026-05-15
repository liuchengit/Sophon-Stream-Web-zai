#include "services/SystemService.h"
#include "database/DbManager.h"
#include "sophon_bridge/EngineBridge.h"
#include "common/Version.h"
#include "models/Alert.h"
#include "models/AuditLog.h"
#include "models/FirmwareVersion.h"
#include "models/SystemConfig.h"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <ctime>
#include <unistd.h>
#include <sys/utsname.h>

namespace sophon_stream {
namespace web {
namespace services {

SystemService& SystemService::getInstance() {
    static SystemService instance;
    return instance;
}

// ── System info ─────────────────────────────────────────────

nlohmann::json SystemService::getSystemInfo() {
    nlohmann::json result;
    result["version"] = PROJECT_VERSION;
    result["name"] = PROJECT_NAME;

    // Uptime
    std::ifstream uptimeFile("/proc/uptime");
    double uptimeSeconds = 0;
    if (uptimeFile.is_open()) {
        uptimeFile >> uptimeSeconds;
    }
    result["uptime"] = static_cast<int64_t>(uptimeSeconds);

    // OS info
    struct utsname uts;
    if (uname(&uts) == 0) {
        result["os"]["sysname"] = uts.sysname;
        result["os"]["nodename"] = uts.nodename;
        result["os"]["release"] = uts.release;
        result["os"]["version"] = uts.version;
        result["os"]["machine"] = uts.machine;
        result["hostname"] = uts.nodename;
    }

    // CPU info
    std::ifstream cpuInfo("/proc/cpuinfo");
    int cpuCount = 0;
    std::string cpuModel;
    if (cpuInfo.is_open()) {
        std::string line;
        while (std::getline(cpuInfo, line)) {
            if (line.find("processor") == 0) ++cpuCount;
            if (line.find("model name") == 0 && cpuModel.empty()) {
                size_t pos = line.find(':');
                if (pos != std::string::npos) cpuModel = line.substr(pos + 2);
            }
        }
    }
    result["cpu"]["count"] = cpuCount;
    result["cpu"]["model"] = cpuModel;

    // Memory info
    std::ifstream memInfo("/proc/meminfo");
    long memTotal = 0, memAvailable = 0;
    if (memInfo.is_open()) {
        std::string line;
        while (std::getline(memInfo, line)) {
            if (line.find("MemTotal:") == 0) {
                std::istringstream iss(line.substr(9));
                iss >> memTotal;
            } else if (line.find("MemAvailable:") == 0) {
                std::istringstream iss(line.substr(13));
                iss >> memAvailable;
            }
            if (memTotal > 0 && memAvailable > 0) break;
        }
    }
    result["memory"]["totalMB"] = memTotal / 1024;
    result["memory"]["availableMB"] = memAvailable / 1024;
    result["memory"]["usedMB"] = (memTotal - memAvailable) / 1024;

    // TPU info
    std::ifstream tpuCheck("/dev/sophon0");
    result["tpu"]["available"] = tpuCheck.is_open();
    if (tpuCheck.is_open()) tpuCheck.close();

    // Engine info
    auto& bridge = sophon_bridge::EngineBridge::getInstance();
    auto engineStatus = bridge.getEngineStatus();
    result["engine"]["initialized"] = engineStatus.initialized;
    result["engine"]["runningGraphs"] = engineStatus.runningGraphs;
    result["engine"]["totalGraphs"] = engineStatus.totalGraphs;
    result["engine"]["version"] = engineStatus.version;

    result["pid"] = static_cast<int>(getpid());

    return result;
}

// ── Configuration ───────────────────────────────────────────

std::string SystemService::getConfig(const std::string& key) {
    auto& db = database::DbManager::getInstance();
    auto qr = db.query("SELECT value FROM system_config WHERE key = ?", {key});
    if (!qr.success || qr.rows.empty()) return "";
    return qr.rows[0].at("value");
}

bool SystemService::setConfig(const std::string& key, const std::string& value) {
    auto& db = database::DbManager::getInstance();

    if (db.exists("system_config", "key = ?", {key})) {
        auto qr = db.update("system_config", {{"value", value}}, "key = ?", {key});
        if (!qr.success) {
            spdlog::error("SystemService::setConfig - update error: {}", qr.errorMessage);
            return false;
        }
    } else {
        auto qr = db.insert("system_config", {{"key", key}, {"value", value}});
        if (!qr.success) {
            spdlog::error("SystemService::setConfig - insert error: {}", qr.errorMessage);
            return false;
        }
    }

    db.insert("audit_logs", {
        {"action", "update"},
        {"resource_type", "system_config"},
        {"detail", "Set config: " + key}
    });

    return true;
}

nlohmann::json SystemService::getAllConfig() {
    auto& db = database::DbManager::getInstance();
    auto qr = db.find("system_config", "", {}, "key ASC");

    nlohmann::json items = nlohmann::json::array();
    if (qr.success) {
        for (const auto& row : qr.rows) {
            items.push_back(models::SystemConfig::fromDbRow(row).toJson());
        }
    }

    return items;
}

// ── Audit logs ──────────────────────────────────────────────

nlohmann::json SystemService::listAuditLogs(int page, int pageSize,
                                              int userId,
                                              const std::string& action) {
    auto& db = database::DbManager::getInstance();

    std::vector<std::string> conditions;
    std::vector<std::string> params;

    if (userId >= 0) {
        conditions.push_back("user_id = ?");
        params.push_back(std::to_string(userId));
    }
    if (!action.empty()) {
        conditions.push_back("action = ?");
        params.push_back(action);
    }

    std::string whereClause;
    for (size_t i = 0; i < conditions.size(); ++i) {
        if (i > 0) whereClause += " AND ";
        whereClause += conditions[i];
    }

    int64_t total = db.count("audit_logs", whereClause, params);
    int offset = (page - 1) * pageSize;

    auto qr = db.find("audit_logs", whereClause.empty() ? "" : whereClause, params,
                       "created_at DESC", pageSize, offset);

    nlohmann::json items = nlohmann::json::array();
    if (qr.success) {
        for (const auto& row : qr.rows) {
            items.push_back(models::AuditLog::fromDbRow(row).toJson());
        }
    }

    return {{"items", items}, {"total", total}, {"page", page}, {"pageSize", pageSize}};
}

// ── Logs export ─────────────────────────────────────────────

std::string SystemService::exportLogs(const std::string& type, const std::string& timeRange) {
    auto& db = database::DbManager::getInstance();

    // Generate timestamped filename
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    std::ostringstream filename;
    filename << "./backups/logs_" << type << "_"
             << std::put_time(std::localtime(&timeT), "%Y%m%d_%H%M%S") << ".json";

    // Ensure directory exists
    std::filesystem::create_directories("./backups");

    nlohmann::json exportData;
    exportData["type"] = type;
    exportData["timeRange"] = timeRange;
    {
        char timeBuf[64];
        std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&timeT));
        exportData["exportedAt"] = std::string(timeBuf);
    }

    std::string interval;
    if (timeRange == "1h") interval = "-1 hour";
    else if (timeRange == "24h") interval = "-24 hours";
    else if (timeRange == "7d") interval = "-7 days";
    else if (timeRange == "30d") interval = "-30 days";
    else interval = "-24 hours";

    if (type == "audit") {
        auto qr = db.query(
            "SELECT * FROM audit_logs WHERE created_at >= datetime('now','localtime', ?) ORDER BY created_at DESC",
            {interval});
        nlohmann::json logs = nlohmann::json::array();
        if (qr.success) {
            for (const auto& row : qr.rows) {
                logs.push_back(models::AuditLog::fromDbRow(row).toJson());
            }
        }
        exportData["logs"] = logs;
    } else if (type == "alert") {
        auto qr = db.query(
            "SELECT * FROM alerts WHERE created_at >= datetime('now','localtime', ?) ORDER BY created_at DESC",
            {interval});
        nlohmann::json logs = nlohmann::json::array();
        if (qr.success) {
            for (const auto& row : qr.rows) {
                logs.push_back(models::Alert::fromDbRow(row).toJson());
            }
        }
        exportData["logs"] = logs;
    }

    // Write to file
    std::ofstream ofs(filename.str());
    if (ofs.is_open()) {
        ofs << exportData.dump(2);
        ofs.close();
    }

    db.insert("audit_logs", {
        {"action", "export"},
        {"resource_type", "log"},
        {"detail", "Exported " + type + " logs: " + filename.str()}
    });

    spdlog::info("SystemService::exportLogs - exported to {}", filename.str());
    return filename.str();
}

// ── Firmware ────────────────────────────────────────────────

nlohmann::json SystemService::checkUpdate() {
    nlohmann::json result;
    result["hasUpdate"] = false;
    result["version"] = "";
    result["description"] = "";

    // In production, this would query a remote update server
    // For now, check the firmware_versions table for newer versions
    auto& db = database::DbManager::getInstance();
    auto qr = db.find("firmware_versions", "status = 0", {}, "created_at DESC", 1);

    if (qr.success && !qr.rows.empty()) {
        auto fw = models::FirmwareVersion::fromDbRow(qr.rows[0]);
        result["hasUpdate"] = true;
        result["version"] = fw.version;
        result["description"] = fw.description;
        result["id"] = fw.id;
        result["type"] = fw.type;
        result["size"] = fw.size;
        result["checksum"] = fw.checksum;
    }

    return result;
}

bool SystemService::installFirmware(int versionId) {
    auto& db = database::DbManager::getInstance();
    auto qr = db.findById("firmware_versions", versionId);
    if (!qr.success || qr.rows.empty()) return false;

    auto fw = models::FirmwareVersion::fromDbRow(qr.rows[0]);

    // Mark as installing
    db.update("firmware_versions", {{"status", "1"}}, "id = ?", {std::to_string(versionId)});

    db.insert("audit_logs", {
        {"action", "install"},
        {"resource_type", "firmware"},
        {"resource_id", std::to_string(versionId)},
        {"detail", "Installing firmware: " + fw.version}
    });

    // In production: download package, verify checksum, apply update
    // For now, mark as installed
    db.update("firmware_versions", {
        {"status", "2"},
        {"installed_at", "datetime('now','localtime')"}
    }, "id = ?", {std::to_string(versionId)});

    spdlog::info("SystemService::installFirmware - firmware v{} installed", fw.version);
    return true;
}

std::vector<models::FirmwareVersion> SystemService::listFirmwareVersions(const std::string& type) {
    auto& db = database::DbManager::getInstance();

    std::string whereClause;
    std::vector<std::string> params;
    if (!type.empty()) {
        whereClause = "type = ?";
        params.push_back(type);
    }

    auto qr = db.find("firmware_versions", whereClause.empty() ? "" : whereClause, params,
                       "created_at DESC");

    std::vector<models::FirmwareVersion> versions;
    if (qr.success) {
        for (const auto& row : qr.rows) {
            versions.push_back(models::FirmwareVersion::fromDbRow(row));
        }
    }
    return versions;
}

// ── Backup/Restore ──────────────────────────────────────────

std::string SystemService::backupDatabase() {
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    std::ostringstream filename;
    filename << "./backups/sophon_stream_backup_"
             << std::put_time(std::localtime(&timeT), "%Y%m%d_%H%M%S") << ".db";

    std::filesystem::create_directories("./backups");

    // Use SQLite backup API
    auto& db = database::DbManager::getInstance();
    auto* srcDb = db.getHandle();
    if (!srcDb) {
        spdlog::error("SystemService::backupDatabase - no database handle");
        return "";
    }

    sqlite3* destDb = nullptr;
    int rc = sqlite3_open(filename.str().c_str(), &destDb);
    if (rc != SQLITE_OK) {
        spdlog::error("SystemService::backupDatabase - cannot open destination: {}", sqlite3_errmsg(destDb));
        return "";
    }

    sqlite3_backup* backup = sqlite3_backup_init(destDb, "main", srcDb, "main");
    if (!backup) {
        spdlog::error("SystemService::backupDatabase - backup init failed");
        sqlite3_close(destDb);
        return "";
    }

    sqlite3_backup_step(backup, -1); // Copy entire database
    sqlite3_backup_finish(backup);
    sqlite3_close(destDb);

    db.insert("audit_logs", {
        {"action", "backup"},
        {"resource_type", "system"},
        {"detail", "Database backup: " + filename.str()}
    });

    spdlog::info("SystemService::backupDatabase - backup created: {}", filename.str());
    return filename.str();
}

bool SystemService::restoreDatabase(const std::string& filePath) {
    namespace fs = std::filesystem;

    if (!fs::exists(filePath)) {
        spdlog::error("SystemService::restoreDatabase - file not found: {}", filePath);
        return false;
    }

    // In production, this would:
    // 1. Stop all services
    // 2. Replace the database file
    // 3. Reconnect
    // 4. Restart services
    // For safety, we just log and return true (actual restore requires admin to restart)
    spdlog::warn("SystemService::restoreDatabase - restore requested from: {} (requires restart)", filePath);

    auto& db = database::DbManager::getInstance();
    db.insert("audit_logs", {
        {"action", "restore"},
        {"resource_type", "system"},
        {"detail", "Database restore requested from: " + filePath}
    });

    return true;
}

// ── System control ──────────────────────────────────────────

bool SystemService::reboot() {
    auto& db = database::DbManager::getInstance();
    db.insert("audit_logs", {
        {"action", "reboot"},
        {"resource_type", "system"},
        {"detail", "System reboot requested"}
    });

    spdlog::warn("SystemService::reboot - system reboot requested");

    // Schedule reboot (gives time for response to be sent)
    // In production, use systemd or init scripts
    // system("shutdown -r +1 'Sophon-Stream Web: Scheduled reboot'");

    return true;
}

}  // namespace services
}  // namespace web
}  // namespace sophon_stream
