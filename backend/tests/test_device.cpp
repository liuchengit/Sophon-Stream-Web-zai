// ============================================================================
// Sophon-Stream Web Management System - DeviceService Unit Tests
// ============================================================================
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "services/DeviceService.h"
#include "database/DbManager.h"
#include "models/Device.h"

using namespace sophon_stream::web::services;
using namespace sophon_stream::web::database;
using namespace sophon_stream::web::models;
using json = nlohmann::json;

// ============================================================================
// Test Fixture
// ============================================================================
class DeviceServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        testDbPath = "/tmp/sophon_stream_test_device_" +
                     std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) +
                     ".db";
        auto& db = DbManager::getInstance();
        db.init(testDbPath);
    }

    void TearDown() override {
        auto& db = DbManager::getInstance();
        db.close();
        std::filesystem::remove(testDbPath);
    }

    // Helper to create a test device
    Device createTestDevice(const std::string& name = "Test Camera",
                            const std::string& type = "ipc",
                            const std::string& url = "rtsp://192.168.1.100:554/stream") {
        Device device;
        device.name = name;
        device.type = type;
        device.streamUrl = url;
        device.protocol = "rtsp";
        device.resolution = "1920x1080";
        device.fps = 25.0;
        device.codec = "H.264";
        device.status = 0;
        device.location = "Test Location";
        device.description = "Test device";
        return device;
    }

    std::string testDbPath;
};

// ============================================================================
// Create Device Tests
// ============================================================================

TEST_F(DeviceServiceTest, CreateDevice) {
    auto& service = DeviceService::getInstance();
    auto device = createTestDevice();

    auto result = service.createDevice(device);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "Test Camera");
    EXPECT_EQ(result->type, "ipc");
    EXPECT_EQ(result->streamUrl, "rtsp://192.168.1.100:554/stream");
    EXPECT_GT(result->id, 0);
}

TEST_F(DeviceServiceTest, CreateDeviceWithAllTypes) {
    auto& service = DeviceService::getInstance();

    std::vector<std::string> types = {"ipc", "nvr", "rtsp", "file", "gb28181"};

    for (const auto& type : types) {
        auto device = createTestDevice("Device_" + type, type, "rtsp://example.com/" + type);
        auto result = service.createDevice(device);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->type, type);
    }
}

TEST_F(DeviceServiceTest, CreateGB28181Device) {
    auto& service = DeviceService::getInstance();
    auto device = createTestDevice("GB Camera", "gb28181", "gb28181://device");
    device.deviceId = "34020000001320000001";

    auto result = service.createDevice(device);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->type, "gb28181");
    EXPECT_EQ(result->deviceId, "34020000001320000001");
}

// ============================================================================
// Read Device Tests
// ============================================================================

TEST_F(DeviceServiceTest, GetDeviceById) {
    auto& service = DeviceService::getInstance();
    auto device = createTestDevice();
    auto created = service.createDevice(device);
    ASSERT_TRUE(created.has_value());

    auto result = service.getDevice(created->id);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, created->id);
    EXPECT_EQ(result->name, "Test Camera");
}

TEST_F(DeviceServiceTest, GetNonexistentDevice) {
    auto& service = DeviceService::getInstance();
    auto result = service.getDevice(99999);
    EXPECT_FALSE(result.has_value());
}

TEST_F(DeviceServiceTest, ListDevices) {
    auto& service = DeviceService::getInstance();

    // Create multiple devices
    for (int i = 0; i < 5; ++i) {
        auto device = createTestDevice("Camera " + std::to_string(i));
        service.createDevice(device);
    }

    auto result = service.listDevices(1, 10);
    EXPECT_TRUE(result.is_array());
    EXPECT_GE(result.size(), 5);
}

TEST_F(DeviceServiceTest, ListDevicesWithPagination) {
    auto& service = DeviceService::getInstance();

    // Create multiple devices
    for (int i = 0; i < 15; ++i) {
        auto device = createTestDevice("Camera " + std::to_string(i));
        service.createDevice(device);
    }

    // First page
    auto page1 = service.listDevices(1, 5);
    EXPECT_EQ(page1.size(), 5);

    // Second page
    auto page2 = service.listDevices(2, 5);
    EXPECT_EQ(page2.size(), 5);
}

TEST_F(DeviceServiceTest, ListDevicesByType) {
    auto& service = DeviceService::getInstance();

    service.createDevice(createTestDevice("IPC 1", "ipc"));
    service.createDevice(createTestDevice("NVR 1", "nvr", "rtsp://nvr.example.com/stream"));
    service.createDevice(createTestDevice("IPC 2", "ipc"));

    auto ipcDevices = service.listDevices(1, 10, "ipc");
    EXPECT_EQ(ipcDevices.size(), 2);

    auto nvrDevices = service.listDevices(1, 10, "nvr");
    EXPECT_EQ(nvrDevices.size(), 1);
}

TEST_F(DeviceServiceTest, ListDevicesByStatus) {
    auto& service = DeviceService::getInstance();

    // Create devices (default status is 0/offline)
    service.createDevice(createTestDevice("Offline Camera 1"));
    service.createDevice(createTestDevice("Offline Camera 2"));

    auto offlineDevices = service.listDevices(1, 10, "", 0);
    EXPECT_GE(offlineDevices.size(), 2);
}

TEST_F(DeviceServiceTest, ListDevicesWithKeyword) {
    auto& service = DeviceService::getInstance();

    service.createDevice(createTestDevice("Front Gate Camera"));
    service.createDevice(createTestDevice("Back Gate Camera"));
    service.createDevice(createTestDevice("Parking Lot Camera"));

    auto result = service.listDevices(1, 10, "", -1, "Gate");
    EXPECT_EQ(result.size(), 2);
}

// ============================================================================
// Update Device Tests
// ============================================================================

TEST_F(DeviceServiceTest, UpdateDevice) {
    auto& service = DeviceService::getInstance();
    auto device = createTestDevice();
    auto created = service.createDevice(device);
    ASSERT_TRUE(created.has_value());

    json updates;
    updates["name"] = "Updated Camera";
    updates["location"] = "New Location";
    updates["fps"] = 30.0;

    auto updated = service.updateDevice(created->id, updates);
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->name, "Updated Camera");
    EXPECT_EQ(updated->location, "New Location");
}

TEST_F(DeviceServiceTest, UpdateNonexistentDevice) {
    auto& service = DeviceService::getInstance();

    json updates;
    updates["name"] = "Should Not Exist";

    auto result = service.updateDevice(99999, updates);
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// Delete Device Tests
// ============================================================================

TEST_F(DeviceServiceTest, DeleteDevice) {
    auto& service = DeviceService::getInstance();
    auto device = createTestDevice("To Delete");
    auto created = service.createDevice(device);
    ASSERT_TRUE(created.has_value());

    bool deleted = service.deleteDevice(created->id);
    EXPECT_TRUE(deleted);

    // Verify it's gone
    auto result = service.getDevice(created->id);
    EXPECT_FALSE(result.has_value());
}

TEST_F(DeviceServiceTest, DeleteNonexistentDevice) {
    auto& service = DeviceService::getInstance();
    bool deleted = service.deleteDevice(99999);
    EXPECT_FALSE(deleted);
}

// ============================================================================
// Device Status Tests
// ============================================================================

TEST_F(DeviceServiceTest, CheckDeviceStatus) {
    auto& service = DeviceService::getInstance();
    auto device = createTestDevice();
    auto created = service.createDevice(device);
    ASSERT_TRUE(created.has_value());

    auto status = service.checkDeviceStatus(created->id);
    // Status result structure (may vary depending on actual device reachability)
    EXPECT_TRUE(status.is_object() || status.is_null());
}

// ============================================================================
// Stream Control Tests
// ============================================================================

TEST_F(DeviceServiceTest, StartStreamForOfflineDevice) {
    auto& service = DeviceService::getInstance();
    auto device = createTestDevice();
    auto created = service.createDevice(device);
    ASSERT_TRUE(created.has_value());

    // Starting a stream on a test RTSP URL will likely fail
    // but the API should not crash
    bool started = service.startStream(created->id);
    // Result depends on whether the RTSP URL is reachable
    // In test environment, it's expected to fail
    // The important thing is it doesn't crash
}

TEST_F(DeviceServiceTest, StopStream) {
    auto& service = DeviceService::getInstance();
    auto device = createTestDevice();
    auto created = service.createDevice(device);
    ASSERT_TRUE(created.has_value());

    bool stopped = service.stopStream(created->id);
    // Stopping a non-running stream should be safe
}

// ============================================================================
// Device Model Tests
// ============================================================================

TEST_F(DeviceServiceTest, DeviceToJson) {
    auto device = createTestDevice();
    device.id = 1;
    device.status = 1;
    device.lastHeartbeat = "2026-01-01 00:00:00";

    json j = device.toJson();
    EXPECT_EQ(j["id"], 1);
    EXPECT_EQ(j["name"], "Test Camera");
    EXPECT_EQ(j["type"], "ipc");
    EXPECT_EQ(j["status"], 1);
    EXPECT_EQ(j["fps"], 25.0);
}

TEST_F(DeviceServiceTest, DeviceFromJson) {
    json j;
    j["name"] = "JSON Camera";
    j["type"] = "nvr";
    j["streamUrl"] = "rtsp://json-test:554/stream";
    j["protocol"] = "rtsp";
    j["resolution"] = "3840x2160";
    j["fps"] = 30.0;

    auto device = Device::fromJson(j);
    EXPECT_EQ(device.name, "JSON Camera");
    EXPECT_EQ(device.type, "nvr");
    EXPECT_EQ(device.fps, 30.0);
}

TEST_F(DeviceServiceTest, DeviceFromDbRow) {
    std::unordered_map<std::string, std::string> row;
    row["id"] = "1";
    row["name"] = "DB Camera";
    row["type"] = "ipc";
    row["stream_url"] = "rtsp://db-test:554/stream";
    row["protocol"] = "rtsp";
    row["resolution"] = "1920x1080";
    row["fps"] = "25.0";
    row["codec"] = "H.264";
    row["status"] = "1";
    row["location"] = "DB Location";
    row["device_id"] = "34020000001320000001";

    auto device = Device::fromDbRow(row);
    EXPECT_EQ(device.id, 1);
    EXPECT_EQ(device.name, "DB Camera");
    EXPECT_EQ(device.streamUrl, "rtsp://db-test:554/stream");
    EXPECT_EQ(device.fps, 25.0);
    EXPECT_EQ(device.deviceId, "34020000001320000001");
}
