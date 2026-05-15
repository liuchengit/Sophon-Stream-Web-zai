#pragma once

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace sophon_stream {
namespace web {
namespace models {

/**
 * AlertRule data model representing a configurable alert trigger rule.
 * Maps to the 'alert_rules' database table.
 */
struct AlertRule {
    int id = 0;
    std::string name;
    int taskId = 0;
    std::string type;          // intrusion / fire / helmet / crowd / threshold / custom
    std::string conditionJson; // JSON string describing the rule condition
    int debounceMs = 3000;     // Minimum interval between repeated alerts
    int enabled = 1;
    std::string notifyType;    // webhook / email / sms
    std::string notifyConfig;  // JSON string with notification configuration
    std::string createdAt;
    std::string updatedAt;

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["id"] = id;
        j["name"] = name;
        j["taskId"] = taskId;
        j["type"] = type;
        j["conditionJson"] = conditionJson;
        j["debounceMs"] = debounceMs;
        j["enabled"] = enabled;
        j["notifyType"] = notifyType;
        j["notifyConfig"] = notifyConfig;
        j["createdAt"] = createdAt;
        j["updatedAt"] = updatedAt;
        return j;
    }

    static AlertRule fromJson(const nlohmann::json& j) {
        AlertRule r;
        if (j.contains("id") && !j["id"].is_null()) r.id = j["id"].get<int>();
        if (j.contains("name") && !j["name"].is_null()) r.name = j["name"].get<std::string>();
        if (j.contains("taskId") && !j["taskId"].is_null()) r.taskId = j["taskId"].get<int>();
        if (j.contains("type") && !j["type"].is_null()) r.type = j["type"].get<std::string>();
        if (j.contains("conditionJson") && !j["conditionJson"].is_null())
            r.conditionJson = j["conditionJson"].is_string() ? j["conditionJson"].get<std::string>() : j["conditionJson"].dump();
        if (j.contains("debounceMs") && !j["debounceMs"].is_null()) r.debounceMs = j["debounceMs"].get<int>();
        if (j.contains("enabled") && !j["enabled"].is_null()) r.enabled = j["enabled"].get<int>();
        if (j.contains("notifyType") && !j["notifyType"].is_null()) r.notifyType = j["notifyType"].get<std::string>();
        if (j.contains("notifyConfig") && !j["notifyConfig"].is_null())
            r.notifyConfig = j["notifyConfig"].is_string() ? j["notifyConfig"].get<std::string>() : j["notifyConfig"].dump();
        return r;
    }

    static AlertRule fromDbRow(const std::unordered_map<std::string, std::string>& row) {
        AlertRule r;
        auto get = [&](const std::string& key) -> std::string {
            auto it = row.find(key);
            return it != row.end() ? it->second : "";
        };
        r.id           = !get("id").empty() ? std::stoi(get("id")) : 0;
        r.name         = get("name");
        r.taskId       = !get("task_id").empty() ? std::stoi(get("task_id")) : 0;
        r.type         = get("type");
        r.conditionJson = get("condition_json");
        r.debounceMs   = !get("debounce_ms").empty() ? std::stoi(get("debounce_ms")) : 3000;
        r.enabled      = !get("enabled").empty() ? std::stoi(get("enabled")) : 1;
        r.notifyType   = get("notify_type");
        r.notifyConfig = get("notify_config");
        r.createdAt    = get("created_at");
        r.updatedAt    = get("updated_at");
        return r;
    }
};

}  // namespace models
}  // namespace web
}  // namespace sophon_stream
