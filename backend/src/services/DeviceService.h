#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>
#include "models/Device.h"

namespace sophon_stream {
namespace web {
namespace services {

/**
 * Device management service.
 * Handles CRUD, status checks, and stream start/stop via EngineBridge.
 */
class DeviceService {
public:
    static DeviceService& getInstance();

    std::optional<models::Device> createDevice(const models::Device& device);
    std::optional<models::Device> updateDevice(int id, const nlohmann::json& updates);
    bool deleteDevice(int id);
    std::optional<models::Device> getDevice(int id);
    nlohmann::json listDevices(int page, int pageSize,
                               const std::string& type = "",
                               int status = -1,
                               const std::string& keyword = "");

    /**
     * Check device online status and stream info.
     */
    nlohmann::json checkDeviceStatus(int id);

    /**
     * Start streaming from a device (connect to sophon-stream source).
     */
    bool startStream(int id);

    /**
     * Stop streaming from a device.
     */
    bool stopStream(int id);

private:
    DeviceService() = default;
    ~DeviceService() = default;
    DeviceService(const DeviceService&) = delete;
    DeviceService& operator=(const DeviceService&) = delete;
};

}  // namespace services
}  // namespace web
}  // namespace sophon_stream
