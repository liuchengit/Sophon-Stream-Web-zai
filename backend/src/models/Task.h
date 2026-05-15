#pragma once

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace sophon_stream {
namespace web {
namespace models {

/**
 * Task data model representing an analysis task (a running graph).
 * Maps to the 'tasks' database table.
 */
struct Task {
    int id = 0;
    std::string name;
    std::string type;          // detect / classify / segment / multi
    int status = 0;            // 0=stopped, 1=running, 2=paused, 3=error
    int deviceId = 0;
    std::string algorithmIds;  // JSON array of algorithm IDs
    std::string graphConfig;   // JSON graph/DAG configuration
    std::string scheduleConfig;// JSON schedule config
    std::string roiConfig;     // JSON ROI configuration
    int osdEnabled = 1;        // On-Screen Display
    int recordEnabled = 0;
    std::string outputUrl;
    std::string description;
    std::string startedAt;
    std::string stoppedAt;
    std::string createdAt;
    std::string updatedAt;

    // Runtime fields (not in DB)
    int64_t graphId = -1;      // Associated EngineBridge graph ID

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["id"] = id;
        j["name"] = name;
        j["type"] = type;
        j["status"] = status;
        j["deviceId"] = deviceId;
        j["algorithmIds"] = algorithmIds;
        j["graphConfig"] = graphConfig;
        j["scheduleConfig"] = scheduleConfig;
        j["roiConfig"] = roiConfig;
        j["osdEnabled"] = osdEnabled;
        j["recordEnabled"] = recordEnabled;
        j["outputUrl"] = outputUrl;
        j["description"] = description;
        j["startedAt"] = startedAt;
        j["stoppedAt"] = stoppedAt;
        j["createdAt"] = createdAt;
        j["updatedAt"] = updatedAt;
        return j;
    }

    static Task fromJson(const nlohmann::json& j) {
        Task t;
        if (j.contains("id") && !j["id"].is_null()) t.id = j["id"].get<int>();
        if (j.contains("name") && !j["name"].is_null()) t.name = j["name"].get<std::string>();
        if (j.contains("type") && !j["type"].is_null()) t.type = j["type"].get<std::string>();
        if (j.contains("status") && !j["status"].is_null()) t.status = j["status"].get<int>();
        if (j.contains("deviceId") && !j["deviceId"].is_null()) t.deviceId = j["deviceId"].get<int>();
        if (j.contains("algorithmIds") && !j["algorithmIds"].is_null())
            t.algorithmIds = j["algorithmIds"].is_string() ? j["algorithmIds"].get<std::string>() : j["algorithmIds"].dump();
        if (j.contains("graphConfig") && !j["graphConfig"].is_null())
            t.graphConfig = j["graphConfig"].is_string() ? j["graphConfig"].get<std::string>() : j["graphConfig"].dump();
        if (j.contains("scheduleConfig") && !j["scheduleConfig"].is_null())
            t.scheduleConfig = j["scheduleConfig"].is_string() ? j["scheduleConfig"].get<std::string>() : j["scheduleConfig"].dump();
        if (j.contains("roiConfig") && !j["roiConfig"].is_null())
            t.roiConfig = j["roiConfig"].is_string() ? j["roiConfig"].get<std::string>() : j["roiConfig"].dump();
        if (j.contains("osdEnabled") && !j["osdEnabled"].is_null()) t.osdEnabled = j["osdEnabled"].get<int>();
        if (j.contains("recordEnabled") && !j["recordEnabled"].is_null()) t.recordEnabled = j["recordEnabled"].get<int>();
        if (j.contains("outputUrl") && !j["outputUrl"].is_null()) t.outputUrl = j["outputUrl"].get<std::string>();
        if (j.contains("description") && !j["description"].is_null()) t.description = j["description"].get<std::string>();
        return t;
    }

    static Task fromDbRow(const std::unordered_map<std::string, std::string>& row) {
        Task t;
        auto get = [&](const std::string& key) -> std::string {
            auto it = row.find(key);
            return it != row.end() ? it->second : "";
        };
        t.id            = !get("id").empty() ? std::stoi(get("id")) : 0;
        t.name          = get("name");
        t.type          = get("type");
        t.status        = !get("status").empty() ? std::stoi(get("status")) : 0;
        t.deviceId      = !get("device_id").empty() ? std::stoi(get("device_id")) : 0;
        t.algorithmIds  = get("algorithm_ids");
        t.graphConfig   = get("graph_config");
        t.scheduleConfig = get("schedule_config");
        t.roiConfig     = get("roi_config");
        t.osdEnabled    = !get("osd_enabled").empty() ? std::stoi(get("osd_enabled")) : 1;
        t.recordEnabled = !get("record_enabled").empty() ? std::stoi(get("record_enabled")) : 0;
        t.outputUrl     = get("output_url");
        t.description   = get("description");
        t.startedAt     = get("started_at");
        t.stoppedAt     = get("stopped_at");
        t.createdAt     = get("created_at");
        t.updatedAt     = get("updated_at");
        return t;
    }
};

}  // namespace models
}  // namespace web
}  // namespace sophon_stream
