#pragma once

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace sophon_stream {
namespace web {
namespace models {

/**
 * Plugin data model representing a dynamically loadable plugin (.so).
 * Maps to the 'plugins' database table.
 */
struct Plugin {
    int id = 0;
    std::string name;
    std::string version;
    std::string soPath;        // Path to the .so shared library
    std::string configSchema;  // JSON schema for plugin configuration
    std::string description;
    std::string author;
    std::string type;          // element / sink / source / filter
    int status = 0;            // 0=inactive, 1=active, 2=error
    std::string checksum;      // SHA-256 checksum of the .so file
    std::string installedAt;
    std::string updatedAt;

    // Runtime handle (not persisted)
    void* dlHandle = nullptr;  // dlopen handle

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["id"] = id;
        j["name"] = name;
        j["version"] = version;
        j["soPath"] = soPath;
        j["configSchema"] = configSchema;
        j["description"] = description;
        j["author"] = author;
        j["type"] = type;
        j["status"] = status;
        j["checksum"] = checksum;
        j["installedAt"] = installedAt;
        j["updatedAt"] = updatedAt;
        return j;
    }

    static Plugin fromJson(const nlohmann::json& j) {
        Plugin p;
        if (j.contains("id") && !j["id"].is_null()) p.id = j["id"].get<int>();
        if (j.contains("name") && !j["name"].is_null()) p.name = j["name"].get<std::string>();
        if (j.contains("version") && !j["version"].is_null()) p.version = j["version"].get<std::string>();
        if (j.contains("soPath") && !j["soPath"].is_null()) p.soPath = j["soPath"].get<std::string>();
        if (j.contains("configSchema") && !j["configSchema"].is_null())
            p.configSchema = j["configSchema"].is_string() ? j["configSchema"].get<std::string>() : j["configSchema"].dump();
        if (j.contains("description") && !j["description"].is_null()) p.description = j["description"].get<std::string>();
        if (j.contains("author") && !j["author"].is_null()) p.author = j["author"].get<std::string>();
        if (j.contains("type") && !j["type"].is_null()) p.type = j["type"].get<std::string>();
        if (j.contains("status") && !j["status"].is_null()) p.status = j["status"].get<int>();
        if (j.contains("checksum") && !j["checksum"].is_null()) p.checksum = j["checksum"].get<std::string>();
        return p;
    }

    static Plugin fromDbRow(const std::unordered_map<std::string, std::string>& row) {
        Plugin p;
        auto get = [&](const std::string& key) -> std::string {
            auto it = row.find(key);
            return it != row.end() ? it->second : "";
        };
        p.id           = !get("id").empty() ? std::stoi(get("id")) : 0;
        p.name         = get("name");
        p.version      = get("version");
        p.soPath       = get("so_path");
        p.configSchema = get("config_schema");
        p.description  = get("description");
        p.author       = get("author");
        p.type         = get("type");
        p.status       = !get("status").empty() ? std::stoi(get("status")) : 0;
        p.checksum     = get("checksum");
        p.installedAt  = get("installed_at");
        p.updatedAt    = get("updated_at");
        return p;
    }
};

}  // namespace models
}  // namespace web
}  // namespace sophon_stream
