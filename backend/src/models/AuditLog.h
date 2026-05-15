#pragma once

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace sophon_stream {
namespace web {
namespace models {

/**
 * AuditLog data model representing a system audit log entry.
 * Maps to the 'audit_logs' database table.
 */
struct AuditLog {
    int id = 0;
    int userId = 0;
    std::string action;        // login / create / update / delete / start / stop
    std::string resourceType;  // user / device / task / algorithm / plugin / system
    int resourceId = 0;
    std::string detail;
    std::string ipAddress;
    std::string createdAt;

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["id"] = id;
        j["userId"] = userId;
        j["action"] = action;
        j["resourceType"] = resourceType;
        j["resourceId"] = resourceId;
        j["detail"] = detail;
        j["ipAddress"] = ipAddress;
        j["createdAt"] = createdAt;
        return j;
    }

    static AuditLog fromJson(const nlohmann::json& j) {
        AuditLog a;
        if (j.contains("id") && !j["id"].is_null()) a.id = j["id"].get<int>();
        if (j.contains("userId") && !j["userId"].is_null()) a.userId = j["userId"].get<int>();
        if (j.contains("action") && !j["action"].is_null()) a.action = j["action"].get<std::string>();
        if (j.contains("resourceType") && !j["resourceType"].is_null()) a.resourceType = j["resourceType"].get<std::string>();
        if (j.contains("resourceId") && !j["resourceId"].is_null()) a.resourceId = j["resourceId"].get<int>();
        if (j.contains("detail") && !j["detail"].is_null()) a.detail = j["detail"].get<std::string>();
        if (j.contains("ipAddress") && !j["ipAddress"].is_null()) a.ipAddress = j["ipAddress"].get<std::string>();
        return a;
    }

    static AuditLog fromDbRow(const std::unordered_map<std::string, std::string>& row) {
        AuditLog a;
        auto get = [&](const std::string& key) -> std::string {
            auto it = row.find(key);
            return it != row.end() ? it->second : "";
        };
        a.id           = !get("id").empty() ? std::stoi(get("id")) : 0;
        a.userId       = !get("user_id").empty() ? std::stoi(get("user_id")) : 0;
        a.action       = get("action");
        a.resourceType = get("resource_type");
        a.resourceId   = !get("resource_id").empty() ? std::stoi(get("resource_id")) : 0;
        a.detail       = get("detail");
        a.ipAddress    = get("ip_address");
        a.createdAt    = get("created_at");
        return a;
    }
};

}  // namespace models
}  // namespace web
}  // namespace sophon_stream
