#include "services/AlertService.h"
#include "database/DbManager.h"
#include <spdlog/spdlog.h>
#include <chrono>

namespace sophon_stream {
namespace web {
namespace services {

AlertService& AlertService::getInstance() {
    static AlertService instance;
    return instance;
}

// ── Alert rules ─────────────────────────────────────────────

std::optional<models::AlertRule> AlertService::createAlertRule(const models::AlertRule& rule) {
    auto& db = database::DbManager::getInstance();

    if (rule.name.empty()) {
        spdlog::warn("AlertService::createAlertRule - name is empty");
        return std::nullopt;
    }

    auto qr = db.insert("alert_rules", {
        {"name",           rule.name},
        {"task_id",        std::to_string(rule.taskId)},
        {"type",           rule.type},
        {"condition_json", rule.conditionJson},
        {"debounce_ms",    std::to_string(rule.debounceMs)},
        {"enabled",        std::to_string(rule.enabled)},
        {"notify_type",    rule.notifyType},
        {"notify_config",  rule.notifyConfig}
    });

    if (!qr.success) {
        spdlog::error("AlertService::createAlertRule - DB error: {}", qr.errorMessage);
        return std::nullopt;
    }

    db.insert("audit_logs", {
        {"action", "create"},
        {"resource_type", "alert_rule"},
        {"resource_id", std::to_string(qr.lastInsertId)},
        {"detail", "Created alert rule: " + rule.name}
    });

    auto result = db.findById("alert_rules", qr.lastInsertId);
    if (!result.success || result.rows.empty()) return std::nullopt;
    return models::AlertRule::fromDbRow(result.rows[0]);
}

std::optional<models::AlertRule> AlertService::updateAlertRule(int id, const nlohmann::json& updates) {
    auto& db = database::DbManager::getInstance();
    if (!db.exists("alert_rules", "id = ?", {std::to_string(id)})) return std::nullopt;

    std::unordered_map<std::string, std::string> data;
    if (updates.contains("name") && !updates["name"].is_null())
        data["name"] = updates["name"].get<std::string>();
    if (updates.contains("taskId") && !updates["taskId"].is_null())
        data["task_id"] = std::to_string(updates["taskId"].get<int>());
    if (updates.contains("type") && !updates["type"].is_null())
        data["type"] = updates["type"].get<std::string>();
    if (updates.contains("conditionJson") && !updates["conditionJson"].is_null())
        data["condition_json"] = updates["conditionJson"].is_string()
            ? updates["conditionJson"].get<std::string>() : updates["conditionJson"].dump();
    if (updates.contains("debounceMs") && !updates["debounceMs"].is_null())
        data["debounce_ms"] = std::to_string(updates["debounceMs"].get<int>());
    if (updates.contains("enabled") && !updates["enabled"].is_null())
        data["enabled"] = std::to_string(updates["enabled"].get<int>());
    if (updates.contains("notifyType") && !updates["notifyType"].is_null())
        data["notify_type"] = updates["notifyType"].get<std::string>();
    if (updates.contains("notifyConfig") && !updates["notifyConfig"].is_null())
        data["notify_config"] = updates["notifyConfig"].is_string()
            ? updates["notifyConfig"].get<std::string>() : updates["notifyConfig"].dump();

    if (data.empty()) {
        auto result = db.findById("alert_rules", id);
        if (!result.success || result.rows.empty()) return std::nullopt;
        return models::AlertRule::fromDbRow(result.rows[0]);
    }

    auto qr = db.update("alert_rules", data, "id = ?", {std::to_string(id)});
    if (!qr.success) {
        spdlog::error("AlertService::updateAlertRule - DB error: {}", qr.errorMessage);
        return std::nullopt;
    }

    db.insert("audit_logs", {
        {"action", "update"},
        {"resource_type", "alert_rule"},
        {"resource_id", std::to_string(id)},
        {"detail", "Updated alert rule"}
    });

    auto result = db.findById("alert_rules", id);
    if (!result.success || result.rows.empty()) return std::nullopt;
    return models::AlertRule::fromDbRow(result.rows[0]);
}

bool AlertService::deleteAlertRule(int id) {
    auto& db = database::DbManager::getInstance();
    if (!db.exists("alert_rules", "id = ?", {std::to_string(id)})) return false;

    auto qr = db.remove("alert_rules", "id = ?", {std::to_string(id)});
    if (!qr.success) {
        spdlog::error("AlertService::deleteAlertRule - DB error: {}", qr.errorMessage);
        return false;
    }

    db.insert("audit_logs", {
        {"action", "delete"},
        {"resource_type", "alert_rule"},
        {"resource_id", std::to_string(id)},
        {"detail", "Deleted alert rule"}
    });

    return true;
}

nlohmann::json AlertService::listAlertRules(int taskId, int page, int pageSize) {
    auto& db = database::DbManager::getInstance();

    std::string whereClause = "task_id = ?";
    std::vector<std::string> params = {std::to_string(taskId)};

    int64_t total = db.count("alert_rules", whereClause, params);
    int offset = (page - 1) * pageSize;

    auto qr = db.find("alert_rules", whereClause, params, "created_at DESC", pageSize, offset);

    nlohmann::json items = nlohmann::json::array();
    if (qr.success) {
        for (const auto& row : qr.rows) {
            items.push_back(models::AlertRule::fromDbRow(row).toJson());
        }
    }

    return {{"items", items}, {"total", total}, {"page", page}, {"pageSize", pageSize}};
}

// ── Alerts ──────────────────────────────────────────────────

nlohmann::json AlertService::listAlerts(int page, int pageSize,
                                         int taskId, int level,
                                         const std::string& type,
                                         int acknowledged) {
    auto& db = database::DbManager::getInstance();

    std::vector<std::string> conditions;
    std::vector<std::string> params;

    if (taskId >= 0) {
        conditions.push_back("task_id = ?");
        params.push_back(std::to_string(taskId));
    }
    if (level >= 0) {
        conditions.push_back("level = ?");
        params.push_back(std::to_string(level));
    }
    if (!type.empty()) {
        conditions.push_back("type = ?");
        params.push_back(type);
    }
    if (acknowledged >= 0) {
        conditions.push_back("acknowledged = ?");
        params.push_back(std::to_string(acknowledged));
    }

    std::string whereClause;
    for (size_t i = 0; i < conditions.size(); ++i) {
        if (i > 0) whereClause += " AND ";
        whereClause += conditions[i];
    }

    int64_t total = db.count("alerts", whereClause, params);
    int offset = (page - 1) * pageSize;

    auto qr = db.find("alerts", whereClause.empty() ? "" : whereClause, params,
                       "created_at DESC", pageSize, offset);

    nlohmann::json items = nlohmann::json::array();
    if (qr.success) {
        for (const auto& row : qr.rows) {
            items.push_back(models::Alert::fromDbRow(row).toJson());
        }
    }

    return {{"items", items}, {"total", total}, {"page", page}, {"pageSize", pageSize}};
}

bool AlertService::acknowledgeAlert(int id) {
    auto& db = database::DbManager::getInstance();
    if (!db.exists("alerts", "id = ?", {std::to_string(id)})) return false;

    auto qr = db.update("alerts", {{"acknowledged", "1"}}, "id = ?", {std::to_string(id)});
    return qr.success;
}

int AlertService::batchAcknowledgeAlerts(const std::vector<int>& ids) {
    auto& db = database::DbManager::getInstance();
    int count = 0;

    for (int id : ids) {
        auto qr = db.update("alerts", {{"acknowledged", "1"}}, "id = ?", {std::to_string(id)});
        if (qr.success) ++count;
    }

    if (count > 0) {
        db.insert("audit_logs", {
            {"action", "acknowledge"},
            {"resource_type", "alert"},
            {"detail", "Batch acknowledged " + std::to_string(count) + " alerts"}
        });
    }

    return count;
}

nlohmann::json AlertService::getAlertStats(const std::string& timeRange) {
    auto& db = database::DbManager::getInstance();

    std::string interval;
    if (timeRange == "1h") interval = "-1 hour";
    else if (timeRange == "24h") interval = "-24 hours";
    else if (timeRange == "7d") interval = "-7 days";
    else if (timeRange == "30d") interval = "-30 days";
    else interval = "-24 hours";

    nlohmann::json result;

    // By type
    auto typeResult = db.query(
        "SELECT type, COUNT(*) as cnt FROM alerts "
        "WHERE created_at >= datetime('now','localtime', ?) GROUP BY type ORDER BY cnt DESC",
        {interval});
    nlohmann::json byType = nlohmann::json::array();
    if (typeResult.success) {
        for (const auto& row : typeResult.rows) {
            byType.push_back({{"type", row.at("type")}, {"count", std::stoll(row.at("cnt"))}});
        }
    }
    result["byType"] = byType;

    // By level
    auto levelResult = db.query(
        "SELECT level, COUNT(*) as cnt FROM alerts "
        "WHERE created_at >= datetime('now','localtime', ?) GROUP BY level ORDER BY level",
        {interval});
    nlohmann::json byLevel = nlohmann::json::array();
    if (levelResult.success) {
        for (const auto& row : levelResult.rows) {
            byLevel.push_back({{"level", std::stoi(row.at("level"))}, {"count", std::stoll(row.at("cnt"))}});
        }
    }
    result["byLevel"] = byLevel;

    // Trend
    std::string trendGroup = (timeRange == "1h") ? "%Y-%m-%d %H" : "%Y-%m-%d";
    auto trendResult = db.query(
        "SELECT strftime('" + trendGroup + "', created_at) as period, COUNT(*) as cnt "
        "FROM alerts WHERE created_at >= datetime('now','localtime', ?) GROUP BY period ORDER BY period",
        {interval});
    nlohmann::json trend = nlohmann::json::array();
    if (trendResult.success) {
        for (const auto& row : trendResult.rows) {
            trend.push_back({{"period", row.at("period")}, {"count", std::stoll(row.at("cnt"))}});
        }
    }
    result["trend"] = trend;
    result["timeRange"] = timeRange;

    return result;
}

// ── Rule engine ─────────────────────────────────────────────

bool AlertService::evaluateRule(const models::AlertRule& rule, const nlohmann::json& detectionResult) {
    if (!rule.enabled) return false;

    try {
        auto condition = nlohmann::json::parse(rule.conditionJson);

        // Type match
        if (condition.contains("type")) {
            std::string condType = condition["type"].get<std::string>();
            if (detectionResult.contains("type") &&
                detectionResult["type"].get<std::string>() != condType) {
                return false;
            }
        }

        // Class match
        if (condition.contains("classes")) {
            auto& condClasses = condition["classes"];
            if (detectionResult.contains("class_name") && condClasses.is_array()) {
                std::string className = detectionResult["class_name"].get<std::string>();
                bool found = false;
                for (const auto& c : condClasses) {
                    if (c.get<std::string>() == className) { found = true; break; }
                }
                if (!found) return false;
            }
        }

        // Confidence threshold
        if (condition.contains("min_confidence")) {
            double minConf = condition["min_confidence"].get<double>();
            if (detectionResult.contains("confidence")) {
                double conf = detectionResult["confidence"].get<double>();
                if (conf < minConf) return false;
            }
        }

        // Count threshold (e.g. minimum number of detections)
        if (condition.contains("min_count")) {
            int minCount = condition["min_count"].get<int>();
            if (detectionResult.contains("count")) {
                int count = detectionResult["count"].get<int>();
                if (count < minCount) return false;
            }
        }

        // Custom expression (simple key=value matching)
        if (condition.contains("expression")) {
            auto& expr = condition["expression"];
            for (auto it = expr.begin(); it != expr.end(); ++it) {
                if (detectionResult.contains(it.key())) {
                    if (detectionResult[it.key()] != it.value()) return false;
                }
            }
        }

        return true;

    } catch (const std::exception& e) {
        spdlog::error("AlertService::evaluateRule - error evaluating rule {}: {}", rule.id, e.what());
        return false;
    }
}

void AlertService::processDetection(int taskId, const nlohmann::json& result) {
    auto& db = database::DbManager::getInstance();

    // Fetch all enabled rules for this task
    auto qr = db.find("alert_rules", "task_id = ? AND enabled = 1",
                       {std::to_string(taskId)}, "id ASC");

    if (!qr.success) return;

    for (const auto& row : qr.rows) {
        auto rule = models::AlertRule::fromDbRow(row);

        if (evaluateRule(rule, result)) {
            // Check debounce: has a similar alert been created recently?
            std::string debounceInterval = "-" + std::to_string(rule.debounceMs) + " milliseconds";
            auto recentCheck = db.query(
                "SELECT id FROM alerts WHERE task_id = ? AND type = ? AND rule_id = ? "
                "AND created_at >= datetime('now','localtime', ?) LIMIT 1",
                {std::to_string(taskId), rule.type, std::to_string(rule.id), debounceInterval});

            if (recentCheck.success && !recentCheck.rows.empty()) {
                continue; // Debounced — skip
            }

            // Create alert
            models::Alert alert;
            alert.taskId = taskId;
            alert.ruleId = rule.id;
            alert.type = rule.type;
            alert.level = 2; // Default warning; could be from rule config
            try {
                auto condition = nlohmann::json::parse(rule.conditionJson);
                if (condition.contains("level")) alert.level = condition["level"].get<int>();
            } catch (...) {}
            alert.message = rule.type + " detected on task " + std::to_string(taskId);
            if (result.contains("class_name")) {
                alert.message += ": " + result["class_name"].get<std::string>();
            }
            alert.metadata = result.dump();

            createAlert(alert);
            sendNotification(rule, alert);
        }
    }
}

bool AlertService::createAlert(const models::Alert& alert) {
    auto& db = database::DbManager::getInstance();

    auto qr = db.insert("alerts", {
        {"task_id",       std::to_string(alert.taskId)},
        {"rule_id",       std::to_string(alert.ruleId)},
        {"type",          alert.type},
        {"level",         std::to_string(alert.level)},
        {"message",       alert.message},
        {"evidence_path", alert.evidencePath},
        {"metadata",      alert.metadata},
        {"acknowledged",  "0"}
    });

    if (!qr.success) {
        spdlog::error("AlertService::createAlert - DB error: {}", qr.errorMessage);
        return false;
    }

    spdlog::info("AlertService::createAlert - alert created (id={}) type={} level={}",
                 qr.lastInsertId, alert.type, alert.level);
    return true;
}

void AlertService::sendNotification(const models::AlertRule& rule, const models::Alert& alert) {
    if (rule.notifyType == "webhook" && !rule.notifyConfig.empty()) {
        try {
            auto config = nlohmann::json::parse(rule.notifyConfig);
            std::string url = config.value("url", "");
            if (!url.empty()) {
                spdlog::info("AlertService::sendNotification - webhook to {} for alert {}", url, alert.id);
                // In production: perform HTTP POST to webhook URL
            }
        } catch (...) {}
    } else if (rule.notifyType == "email" && !rule.notifyConfig.empty()) {
        try {
            auto config = nlohmann::json::parse(rule.notifyConfig);
            spdlog::info("AlertService::sendNotification - email notification for alert {}", alert.id);
            // In production: send email via SMTP
        } catch (...) {}
    }
}

}  // namespace services
}  // namespace web
}  // namespace sophon_stream
