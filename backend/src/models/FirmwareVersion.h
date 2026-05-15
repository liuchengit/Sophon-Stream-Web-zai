#pragma once

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace sophon_stream {
namespace web {
namespace models {

/**
 * FirmwareVersion data model representing a firmware/software update package.
 * Maps to the 'firmware_versions' database table.
 */
struct FirmwareVersion {
    int id = 0;
    std::string version;
    std::string type;          // system / algorithm / plugin
    std::string packageUrl;
    std::string checksum;      // SHA-256 checksum
    int64_t size = 0;          // Package size in bytes
    std::string description;
    int status = 0;            // 0=available, 1=installing, 2=installed, 3=failed
    std::string installedAt;
    std::string createdAt;

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["id"] = id;
        j["version"] = version;
        j["type"] = type;
        j["packageUrl"] = packageUrl;
        j["checksum"] = checksum;
        j["size"] = size;
        j["description"] = description;
        j["status"] = status;
        j["installedAt"] = installedAt;
        j["createdAt"] = createdAt;
        return j;
    }

    static FirmwareVersion fromJson(const nlohmann::json& j) {
        FirmwareVersion f;
        if (j.contains("id") && !j["id"].is_null()) f.id = j["id"].get<int>();
        if (j.contains("version") && !j["version"].is_null()) f.version = j["version"].get<std::string>();
        if (j.contains("type") && !j["type"].is_null()) f.type = j["type"].get<std::string>();
        if (j.contains("packageUrl") && !j["packageUrl"].is_null()) f.packageUrl = j["packageUrl"].get<std::string>();
        if (j.contains("checksum") && !j["checksum"].is_null()) f.checksum = j["checksum"].get<std::string>();
        if (j.contains("size") && !j["size"].is_null()) f.size = j["size"].get<int64_t>();
        if (j.contains("description") && !j["description"].is_null()) f.description = j["description"].get<std::string>();
        if (j.contains("status") && !j["status"].is_null()) f.status = j["status"].get<int>();
        return f;
    }

    static FirmwareVersion fromDbRow(const std::unordered_map<std::string, std::string>& row) {
        FirmwareVersion f;
        auto get = [&](const std::string& key) -> std::string {
            auto it = row.find(key);
            return it != row.end() ? it->second : "";
        };
        f.id          = !get("id").empty() ? std::stoi(get("id")) : 0;
        f.version     = get("version");
        f.type        = get("type");
        f.packageUrl  = get("package_url");
        f.checksum    = get("checksum");
        f.size        = !get("size").empty() ? std::stoll(get("size")) : 0;
        f.description = get("description");
        f.status      = !get("status").empty() ? std::stoi(get("status")) : 0;
        f.installedAt = get("installed_at");
        f.createdAt   = get("created_at");
        return f;
    }
};

}  // namespace models
}  // namespace web
}  // namespace sophon_stream
