#pragma once

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace sophon_stream {
namespace web {
namespace models {

/**
 * Device data model representing a video source (IPC, NVR, RTSP, etc.).
 * Maps to the 'devices' database table.
 */
struct Device {
    int id = 0;
    std::string name;
    std::string type;          // ipc / nvr / rtsp / file / gb28181
    std::string streamUrl;
    std::string protocol;      // rtsp / rtmp / http / gb28181
    std::string resolution;
    double fps = 25.0;
    std::string codec;
    int status = 0;            // 0=offline, 1=online, 2=error
    std::string location;
    std::string description;
    std::string deviceId;      // GB28181 device ID
    std::string lastHeartbeat;
    std::string createdAt;
    std::string updatedAt;

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["id"] = id;
        j["name"] = name;
        j["type"] = type;
        j["streamUrl"] = streamUrl;
        j["protocol"] = protocol;
        j["resolution"] = resolution;
        j["fps"] = fps;
        j["codec"] = codec;
        j["status"] = status;
        j["location"] = location;
        j["description"] = description;
        j["deviceId"] = deviceId;
        j["lastHeartbeat"] = lastHeartbeat;
        j["createdAt"] = createdAt;
        j["updatedAt"] = updatedAt;
        return j;
    }

    static Device fromJson(const nlohmann::json& j) {
        Device d;
        if (j.contains("id") && !j["id"].is_null()) d.id = j["id"].get<int>();
        if (j.contains("name") && !j["name"].is_null()) d.name = j["name"].get<std::string>();
        if (j.contains("type") && !j["type"].is_null()) d.type = j["type"].get<std::string>();
        if (j.contains("streamUrl") && !j["streamUrl"].is_null()) d.streamUrl = j["streamUrl"].get<std::string>();
        if (j.contains("protocol") && !j["protocol"].is_null()) d.protocol = j["protocol"].get<std::string>();
        if (j.contains("resolution") && !j["resolution"].is_null()) d.resolution = j["resolution"].get<std::string>();
        if (j.contains("fps") && !j["fps"].is_null()) d.fps = j["fps"].get<double>();
        if (j.contains("codec") && !j["codec"].is_null()) d.codec = j["codec"].get<std::string>();
        if (j.contains("status") && !j["status"].is_null()) d.status = j["status"].get<int>();
        if (j.contains("location") && !j["location"].is_null()) d.location = j["location"].get<std::string>();
        if (j.contains("description") && !j["description"].is_null()) d.description = j["description"].get<std::string>();
        if (j.contains("deviceId") && !j["deviceId"].is_null()) d.deviceId = j["deviceId"].get<std::string>();
        return d;
    }

    static Device fromDbRow(const std::unordered_map<std::string, std::string>& row) {
        Device d;
        auto get = [&](const std::string& key) -> std::string {
            auto it = row.find(key);
            return it != row.end() ? it->second : "";
        };
        d.id            = !get("id").empty() ? std::stoi(get("id")) : 0;
        d.name          = get("name");
        d.type          = get("type");
        d.streamUrl     = get("stream_url");
        d.protocol      = get("protocol");
        d.resolution    = get("resolution");
        d.fps           = !get("fps").empty() ? std::stod(get("fps")) : 25.0;
        d.codec         = get("codec");
        d.status        = !get("status").empty() ? std::stoi(get("status")) : 0;
        d.location      = get("location");
        d.description   = get("description");
        d.deviceId      = get("device_id");
        d.lastHeartbeat = get("last_heartbeat");
        d.createdAt     = get("created_at");
        d.updatedAt     = get("updated_at");
        return d;
    }
};

}  // namespace models
}  // namespace web
}  // namespace sophon_stream
