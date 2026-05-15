#pragma once

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace sophon_stream {
namespace web {
namespace models {

/**
 * Algorithm data model representing an AI inference algorithm.
 * Maps to the 'algorithms' database table.
 */
struct Algorithm {
    int id = 0;
    std::string name;
    std::string type;          // detection / classification / segmentation / pose / ocr / face
    std::string modelPath;
    std::string configJson;    // JSON string with algorithm-specific config
    std::string version;
    std::string description;
    std::string inputSize;     // e.g. "640x640"
    std::string classes;       // JSON array of class names
    int status = 1;            // 1=active, 0=disabled
    std::string createdAt;
    std::string updatedAt;

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["id"] = id;
        j["name"] = name;
        j["type"] = type;
        j["modelPath"] = modelPath;
        j["configJson"] = configJson;
        j["version"] = version;
        j["description"] = description;
        j["inputSize"] = inputSize;
        j["classes"] = classes;
        j["status"] = status;
        j["createdAt"] = createdAt;
        j["updatedAt"] = updatedAt;
        return j;
    }

    static Algorithm fromJson(const nlohmann::json& j) {
        Algorithm a;
        if (j.contains("id") && !j["id"].is_null()) a.id = j["id"].get<int>();
        if (j.contains("name") && !j["name"].is_null()) a.name = j["name"].get<std::string>();
        if (j.contains("type") && !j["type"].is_null()) a.type = j["type"].get<std::string>();
        if (j.contains("modelPath") && !j["modelPath"].is_null()) a.modelPath = j["modelPath"].get<std::string>();
        if (j.contains("configJson") && !j["configJson"].is_null()) a.configJson = j["configJson"].is_string() ? j["configJson"].get<std::string>() : j["configJson"].dump();
        if (j.contains("version") && !j["version"].is_null()) a.version = j["version"].get<std::string>();
        if (j.contains("description") && !j["description"].is_null()) a.description = j["description"].get<std::string>();
        if (j.contains("inputSize") && !j["inputSize"].is_null()) a.inputSize = j["inputSize"].get<std::string>();
        if (j.contains("classes") && !j["classes"].is_null()) a.classes = j["classes"].is_string() ? j["classes"].get<std::string>() : j["classes"].dump();
        if (j.contains("status") && !j["status"].is_null()) a.status = j["status"].get<int>();
        return a;
    }

    static Algorithm fromDbRow(const std::unordered_map<std::string, std::string>& row) {
        Algorithm a;
        auto get = [&](const std::string& key) -> std::string {
            auto it = row.find(key);
            return it != row.end() ? it->second : "";
        };
        a.id          = !get("id").empty() ? std::stoi(get("id")) : 0;
        a.name        = get("name");
        a.type        = get("type");
        a.modelPath   = get("model_path");
        a.configJson  = get("config_json");
        a.version     = get("version");
        a.description = get("description");
        a.inputSize   = get("input_size");
        a.classes     = get("classes");
        a.status      = !get("status").empty() ? std::stoi(get("status")) : 0;
        a.createdAt   = get("created_at");
        a.updatedAt   = get("updated_at");
        return a;
    }
};

}  // namespace models
}  // namespace web
}  // namespace sophon_stream
