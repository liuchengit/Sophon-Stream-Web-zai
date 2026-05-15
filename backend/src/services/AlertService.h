#pragma once

#include <string>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>
#include "models/Alert.h"
#include "models/AlertRule.h"

namespace sophon_stream {
namespace web {
namespace services {

/**
 * Alert management and rule engine service.
 * Handles alert rules, alert listing/acknowledgement, and detection processing.
 */
class AlertService {
public:
    static AlertService& getInstance();

    // ── Alert rules ─────────────────────────────────────────

    std::optional<models::AlertRule> createAlertRule(const models::AlertRule& rule);
    std::optional<models::AlertRule> updateAlertRule(int id, const nlohmann::json& updates);
    bool deleteAlertRule(int id);
    nlohmann::json listAlertRules(int taskId, int page, int pageSize);

    // ── Alerts ──────────────────────────────────────────────

    nlohmann::json listAlerts(int page, int pageSize,
                               int taskId = -1,
                               int level = -1,
                               const std::string& type = "",
                               int acknowledged = -1);

    bool acknowledgeAlert(int id);
    int batchAcknowledgeAlerts(const std::vector<int>& ids);

    /**
     * Get alert statistics.
     */
    nlohmann::json getAlertStats(const std::string& timeRange = "24h");

    // ── Rule engine ─────────────────────────────────────────

    /**
     * Evaluate a rule against a detection result.
     */
    bool evaluateRule(const models::AlertRule& rule, const nlohmann::json& detectionResult);

    /**
     * Process a detection result: evaluate all active rules for the task,
     * generate alerts as needed.
     */
    void processDetection(int taskId, const nlohmann::json& result);

private:
    AlertService() = default;
    ~AlertService() = default;
    AlertService(const AlertService&) = delete;
    AlertService& operator=(const AlertService&) = delete;

    /**
     * Create an alert in the database.
     */
    bool createAlert(const models::Alert& alert);

    /**
     * Send notification for a triggered alert.
     */
    void sendNotification(const models::AlertRule& rule, const models::Alert& alert);
};

}  // namespace services
}  // namespace web
}  // namespace sophon_stream
