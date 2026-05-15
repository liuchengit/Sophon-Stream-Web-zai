#pragma once

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace sophon_stream {
namespace web {
namespace models {

/**
 * SystemConfig data model representing a key-value system configuration entry.
 * Maps to the 'system_config' database table.
 */
struct SystemConfig {
    std::string key;
    std::string value;
    std::string description;
    std::string updatedAt;

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["key"] = key;
        j["value"] = value;
        j["description"] = description;
        j["updatedAt"] = updatedAt;
        return j;
    }

    static SystemConfig fromJson(const nlohmann::json& j) {
        SystemConfig c;
        if (j.contains("key") && !j["key"].is_null()) c.key = j["key"].get<std::string>();
        if (j.contains("value") && !j["value"].is_null()) c.value = j["value"].is_string() ? j["value"].get<std::string>() : j["value"].dump();
        if (j.contains("description") && !j["description"].is_null()) c.description = j["description"].get<std::string>();
        return c;
    }

    static SystemConfig fromDbRow(const std::unordered_map<std::string, std::string>& row) {
        SystemConfig c;
        auto get = [&](const std::string& key) -> std::string {
            auto it = row.find(key);
            return it != row.end() ? it->second : "";
        };
        c.key         = get("key");
        c.value       = get("value");
        c.description = get("description");
        c.updatedAt   = get("updated_at");
        return c;
    }
};

}  // namespace models
}  // namespace web
}  // namespace sophon_stream
