#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "models/SystemConfig.h"
#include "models/AuditLog.h"
#include "models/FirmwareVersion.h"

namespace sophon_stream {
namespace web {
namespace services {

/**
 * System administration service.
 * Handles system info, configuration, audit logs, firmware, backup/restore, and reboot.
 */
class SystemService {
public:
    static SystemService& getInstance();

    // ── System info ─────────────────────────────────────────

    nlohmann::json getSystemInfo();

    // ── Configuration ───────────────────────────────────────

    std::string getConfig(const std::string& key);
    bool setConfig(const std::string& key, const std::string& value);
    nlohmann::json getAllConfig();

    // ── Audit logs ──────────────────────────────────────────

    nlohmann::json listAuditLogs(int page, int pageSize,
                                  int userId = -1,
                                  const std::string& action = "");

    // ── Logs export ─────────────────────────────────────────

    std::string exportLogs(const std::string& type, const std::string& timeRange);

    // ── Firmware ────────────────────────────────────────────

    nlohmann::json checkUpdate();
    bool installFirmware(int versionId);
    std::vector<models::FirmwareVersion> listFirmwareVersions(const std::string& type = "");

    // ── Backup/Restore ──────────────────────────────────────

    std::string backupDatabase();
    bool restoreDatabase(const std::string& filePath);

    // ── System control ──────────────────────────────────────

    bool reboot();

private:
    SystemService() = default;
    ~SystemService() = default;
    SystemService(const SystemService&) = delete;
    SystemService& operator=(const SystemService&) = delete;
};

}  // namespace services
}  // namespace web
}  // namespace sophon_stream
