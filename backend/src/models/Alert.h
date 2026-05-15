#pragma once

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace sophon_stream {
namespace web {
namespace models {

/**
 * Alert data model representing a triggered alert event.
 * Maps to the 'alerts' database table.
 */
struct Alert {
    int id = 0;
    int taskId = 0;
    int ruleId = 0;
    std::string type;          // intrusion / fire / helmet / crowd / custom
    int level = 1;             // 1=info, 2=warning, 3=critical
    std::string message;
    std::string evidencePath;  // Path to screenshot/video clip
    std::string metadata;      // JSON string with extra data
    int acknowledged = 0;      // 0=no, 1=yes
    std::string createdAt;

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["id"] = id;
        j["taskId"] = taskId;
        j["ruleId"] = ruleId;
        j["type"] = type;
        j["level"] = level;
        j["message"] = message;
        j["evidencePath"] = evidencePath;
        j["metadata"] = metadata;
        j["acknowledged"] = acknowledged;
        j["createdAt"] = createdAt;
        return j;
    }

    static Alert fromJson(const nlohmann::json& j) {
        Alert a;
        if (j.contains("id") && !j["id"].is_null()) a.id = j["id"].get<int>();
        if (j.contains("taskId") && !j["taskId"].is_null()) a.taskId = j["taskId"].get<int>();
        if (j.contains("ruleId") && !j["ruleId"].is_null()) a.ruleId = j["ruleId"].get<int>();
        if (j.contains("type") && !j["type"].is_null()) a.type = j["type"].get<std::string>();
        if (j.contains("level") && !j["level"].is_null()) a.level = j["level"].get<int>();
        if (j.contains("message") && !j["message"].is_null()) a.message = j["message"].get<std::string>();
        if (j.contains("evidencePath") && !j["evidencePath"].is_null()) a.evidencePath = j["evidencePath"].get<std::string>();
        if (j.contains("metadata") && !j["metadata"].is_null())
            a.metadata = j["metadata"].is_string() ? j["metadata"].get<std::string>() : j["metadata"].dump();
        if (j.contains("acknowledged") && !j["acknowledged"].is_null()) a.acknowledged = j["acknowledged"].get<int>();
        return a;
    }

    static Alert fromDbRow(const std::unordered_map<std::string, std::string>& row) {
        Alert a;
        auto get = [&](const std::string& key) -> std::string {
            auto it = row.find(key);
            return it != row.end() ? it->second : "";
        };
        a.id           = !get("id").empty() ? std::stoi(get("id")) : 0;
        a.taskId       = !get("task_id").empty() ? std::stoi(get("task_id")) : 0;
        a.ruleId       = !get("rule_id").empty() ? std::stoi(get("rule_id")) : 0;
        a.type         = get("type");
        a.level        = !get("level").empty() ? std::stoi(get("level")) : 1;
        a.message      = get("message");
        a.evidencePath = get("evidence_path");
        a.metadata     = get("metadata");
        a.acknowledged = !get("acknowledged").empty() ? std::stoi(get("acknowledged")) : 0;
        a.createdAt    = get("created_at");
        return a;
    }
};

}  // namespace models
}  // namespace web
}  // namespace sophon_stream
