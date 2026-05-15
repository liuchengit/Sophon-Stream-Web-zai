#include "services/DeviceService.h"
#include "database/DbManager.h"
#include "sophon_bridge/EngineBridge.h"
#include <spdlog/spdlog.h>

namespace sophon_stream {
namespace web {
namespace services {

DeviceService& DeviceService::getInstance() {
    static DeviceService instance;
    return instance;
}

std::optional<models::Device> DeviceService::createDevice(const models::Device& device) {
    auto& db = database::DbManager::getInstance();

    if (device.name.empty() || device.streamUrl.empty()) {
        spdlog::warn("DeviceService::createDevice - name or streamUrl is empty");
        return std::nullopt;
    }

    auto qr = db.insert("devices", {
        {"name",       device.name},
        {"type",       device.type.empty() ? "ipc" : device.type},
        {"stream_url", device.streamUrl},
        {"protocol",   device.protocol},
        {"resolution", device.resolution},
        {"fps",        std::to_string(device.fps)},
        {"codec",      device.codec.empty() ? "H.264" : device.codec},
        {"status",     "0"},
        {"location",   device.location},
        {"description", device.description},
        {"device_id",  device.deviceId}
    });

    if (!qr.success) {
        spdlog::error("DeviceService::createDevice - DB error: {}", qr.errorMessage);
        return std::nullopt;
    }

    // Audit log
    db.insert("audit_logs", {
        {"action", "create"},
        {"resource_type", "device"},
        {"resource_id", std::to_string(qr.lastInsertId)},
        {"detail", "Created device: " + device.name}
    });

    return getDevice(static_cast<int>(qr.lastInsertId));
}

std::optional<models::Device> DeviceService::updateDevice(int id, const nlohmann::json& updates) {
    auto& db = database::DbManager::getInstance();

    if (!db.exists("devices", "id = ?", {std::to_string(id)})) return std::nullopt;

    std::unordered_map<std::string, std::string> data;
    if (updates.contains("name") && !updates["name"].is_null())
        data["name"] = updates["name"].get<std::string>();
    if (updates.contains("type") && !updates["type"].is_null())
        data["type"] = updates["type"].get<std::string>();
    if (updates.contains("streamUrl") && !updates["streamUrl"].is_null())
        data["stream_url"] = updates["streamUrl"].get<std::string>();
    if (updates.contains("protocol") && !updates["protocol"].is_null())
        data["protocol"] = updates["protocol"].get<std::string>();
    if (updates.contains("resolution") && !updates["resolution"].is_null())
        data["resolution"] = updates["resolution"].get<std::string>();
    if (updates.contains("fps") && !updates["fps"].is_null())
        data["fps"] = std::to_string(updates["fps"].get<double>());
    if (updates.contains("codec") && !updates["codec"].is_null())
        data["codec"] = updates["codec"].get<std::string>();
    if (updates.contains("status") && !updates["status"].is_null())
        data["status"] = std::to_string(updates["status"].get<int>());
    if (updates.contains("location") && !updates["location"].is_null())
        data["location"] = updates["location"].get<std::string>();
    if (updates.contains("description") && !updates["description"].is_null())
        data["description"] = updates["description"].get<std::string>();
    if (updates.contains("deviceId") && !updates["deviceId"].is_null())
        data["device_id"] = updates["deviceId"].get<std::string>();

    if (data.empty()) return getDevice(id);

    auto qr = db.update("devices", data, "id = ?", {std::to_string(id)});
    if (!qr.success) {
        spdlog::error("DeviceService::updateDevice - DB error: {}", qr.errorMessage);
        return std::nullopt;
    }

    db.insert("audit_logs", {
        {"action", "update"},
        {"resource_type", "device"},
        {"resource_id", std::to_string(id)},
        {"detail", "Updated device"}
    });

    return getDevice(id);
}

bool DeviceService::deleteDevice(int id) {
    auto& db = database::DbManager::getInstance();
    if (!db.exists("devices", "id = ?", {std::to_string(id)})) return false;

    auto qr = db.remove("devices", "id = ?", {std::to_string(id)});
    if (!qr.success) {
        spdlog::error("DeviceService::deleteDevice - DB error: {}", qr.errorMessage);
        return false;
    }

    db.insert("audit_logs", {
        {"action", "delete"},
        {"resource_type", "device"},
        {"resource_id", std::to_string(id)},
        {"detail", "Deleted device"}
    });

    spdlog::info("DeviceService::deleteDevice - device id={} deleted", id);
    return true;
}

std::optional<models::Device> DeviceService::getDevice(int id) {
    auto& db = database::DbManager::getInstance();
    auto qr = db.findById("devices", id);
    if (!qr.success || qr.rows.empty()) return std::nullopt;
    return models::Device::fromDbRow(qr.rows[0]);
}

nlohmann::json DeviceService::listDevices(int page, int pageSize,
                                           const std::string& type,
                                           int status,
                                           const std::string& keyword) {
    auto& db = database::DbManager::getInstance();

    std::vector<std::string> conditions;
    std::vector<std::string> params;

    if (!type.empty()) {
        conditions.push_back("type = ?");
        params.push_back(type);
    }
    if (status >= 0) {
        conditions.push_back("status = ?");
        params.push_back(std::to_string(status));
    }
    if (!keyword.empty()) {
        conditions.push_back("(name LIKE ? OR location LIKE ? OR description LIKE ?)");
        std::string pattern = "%" + keyword + "%";
        params.push_back(pattern);
        params.push_back(pattern);
        params.push_back(pattern);
    }

    std::string whereClause;
    for (size_t i = 0; i < conditions.size(); ++i) {
        if (i > 0) whereClause += " AND ";
        whereClause += conditions[i];
    }

    int64_t total = db.count("devices", whereClause, params);
    int offset = (page - 1) * pageSize;

    auto qr = db.find("devices", whereClause.empty() ? "" : whereClause, params,
                       "created_at DESC", pageSize, offset);

    nlohmann::json items = nlohmann::json::array();
    if (qr.success) {
        for (const auto& row : qr.rows) {
            items.push_back(models::Device::fromDbRow(row).toJson());
        }
    }

    return {{"items", items}, {"total", total}, {"page", page}, {"pageSize", pageSize}};
}

nlohmann::json DeviceService::checkDeviceStatus(int id) {
    auto device = getDevice(id);
    if (!device) return {};

    nlohmann::json result;
    result["id"] = id;
    result["online"] = (device->status == 1);
    result["status"] = device->status;

    // Check EngineBridge for stream info if online
    if (device->status == 1) {
        auto& bridge = sophon_bridge::EngineBridge::getInstance();
        if (bridge.isInitialized()) {
            result["streamInfo"]["url"] = device->streamUrl;
            result["streamInfo"]["protocol"] = device->protocol;
            result["streamInfo"]["fps"] = device->fps;
            result["streamInfo"]["codec"] = device->codec;
        }
    }

    return result;
}

bool DeviceService::startStream(int id) {
    auto device = getDevice(id);
    if (!device) return false;

    if (device->status == 1) {
        spdlog::warn("DeviceService::startStream - device id={} already online", id);
        return true;
    }

    // Update device status to online
    auto& db = database::DbManager::getInstance();
    db.update("devices", {
        {"status", "1"},
        {"last_heartbeat", "datetime('now','localtime')"}
    }, "id = ?", {std::to_string(id)});

    db.insert("audit_logs", {
        {"action", "start"},
        {"resource_type", "device"},
        {"resource_id", std::to_string(id)},
        {"detail", "Started stream for device: " + device->name}
    });

    spdlog::info("DeviceService::startStream - stream started for device id={}", id);
    return true;
}

bool DeviceService::stopStream(int id) {
    auto device = getDevice(id);
    if (!device) return false;

    auto& db = database::DbManager::getInstance();
    db.update("devices", {
        {"status", "0"}
    }, "id = ?", {std::to_string(id)});

    db.insert("audit_logs", {
        {"action", "stop"},
        {"resource_type", "device"},
        {"resource_id", std::to_string(id)},
        {"detail", "Stopped stream for device: " + device->name}
    });

    spdlog::info("DeviceService::stopStream - stream stopped for device id={}", id);
    return true;
}

}  // namespace services
}  // namespace web
}  // namespace sophon_stream
