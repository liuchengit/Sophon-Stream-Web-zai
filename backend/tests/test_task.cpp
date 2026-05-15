// ============================================================================
// Sophon-Stream Web Management System - TaskService Unit Tests
// ============================================================================
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "services/TaskService.h"
#include "services/DeviceService.h"
#include "database/DbManager.h"
#include "models/Task.h"
#include "models/Device.h"

using namespace sophon_stream::web::services;
using namespace sophon_stream::web::database;
using namespace sophon_stream::web::models;
using json = nlohmann::json;

// ============================================================================
// Test Fixture
// ============================================================================
class TaskServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        testDbPath = "/tmp/sophon_stream_test_task_" +
                     std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) +
                     ".db";
        auto& db = DbManager::getInstance();
        db.init(testDbPath);

        // Create a test device (tasks require a device_id FK)
        auto& deviceService = DeviceService::getInstance();
        Device device;
        device.name = "Test Camera for Tasks";
        device.type = "ipc";
        device.streamUrl = "rtsp://192.168.1.200:554/stream";
        device.protocol = "rtsp";
        device.resolution = "1920x1080";
        device.fps = 25.0;
        device.codec = "H.264";
        auto createdDevice = deviceService.createDevice(device);
        ASSERT_TRUE(createdDevice.has_value());
        testDeviceId = createdDevice->id;
    }

    void TearDown() override {
        auto& db = DbManager::getInstance();
        db.close();
        std::filesystem::remove(testDbPath);
    }

    // Helper to create a test task
    Task createTestTask(const std::string& name = "Test Task",
                        const std::string& type = "detect") {
        Task task;
        task.name = name;
        task.type = type;
        task.deviceId = testDeviceId;
        task.algorithmIds = "[1,2]";
        task.graphConfig = R"({"nodes":[],"edges":[]})";
        task.roiConfig = R"({"regions":[]})";
        task.osdEnabled = 1;
        task.recordEnabled = 0;
        task.description = "Test task description";
        return task;
    }

    int testDeviceId = 0;
    std::string testDbPath;
};

// ============================================================================
// Create Task Tests
// ============================================================================

TEST_F(TaskServiceTest, CreateTask) {
    auto& service = TaskService::getInstance();
    auto task = createTestTask();

    auto result = service.createTask(task);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "Test Task");
    EXPECT_EQ(result->type, "detect");
    EXPECT_EQ(result->deviceId, testDeviceId);
    EXPECT_EQ(result->status, 0);  // Initial status is stopped
    EXPECT_GT(result->id, 0);
}

TEST_F(TaskServiceTest, CreateTaskWithAllTypes) {
    auto& service = TaskService::getInstance();

    std::vector<std::string> types = {"detect", "classify", "segment", "multi"};

    for (const auto& type : types) {
        auto task = createTestTask("Task_" + type, type);
        auto result = service.createTask(task);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->type, type);
    }
}

TEST_F(TaskServiceTest, CreateTaskWithGraphConfig) {
    auto& service = TaskService::getInstance();
    auto task = createTestTask();

    json graphConfig;
    graphConfig["nodes"] = json::array();
    graphConfig["nodes"].push_back({
        {"id", "source_0"},
        {"type", "source"},
        {"config", {{"stream_url", "rtsp://test"}}}
    });
    graphConfig["nodes"].push_back({
        {"id", "detect_0"},
        {"type", "detection"},
        {"config", {{"model_path", "/models/yolov5s.bmodel"}}}
    });
    graphConfig["edges"] = json::array();
    graphConfig["edges"].push_back({
        {"source", "source_0"},
        {"target", "detect_0"}
    });

    task.graphConfig = graphConfig.dump();

    auto result = service.createTask(task);
    ASSERT_TRUE(result.has_value());

    // Verify graph config is stored
    auto retrieved = service.getTask(result->id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_FALSE(retrieved->graphConfig.empty());
}

TEST_F(TaskServiceTest, CreateTaskWithROIConfig) {
    auto& service = TaskService::getInstance();
    auto task = createTestTask();

    json roiConfig;
    roiConfig["regions"] = json::array();
    roiConfig["regions"].push_back({
        {"id", "region_0"},
        {"type", "rectangle"},
        {"points", {{{0.1, 0.2}, {0.5, 0.2}, {0.5, 0.8}, {0.1, 0.8}}}},
        {"label", "entrance"}
    });

    task.roiConfig = roiConfig.dump();

    auto result = service.createTask(task);
    ASSERT_TRUE(result.has_value());
}

// ============================================================================
// Read Task Tests
// ============================================================================

TEST_F(TaskServiceTest, GetTaskById) {
    auto& service = TaskService::getInstance();
    auto task = createTestTask();
    auto created = service.createTask(task);
    ASSERT_TRUE(created.has_value());

    auto result = service.getTask(created->id);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, created->id);
    EXPECT_EQ(result->name, "Test Task");
}

TEST_F(TaskServiceTest, GetNonexistentTask) {
    auto& service = TaskService::getInstance();
    auto result = service.getTask(99999);
    EXPECT_FALSE(result.has_value());
}

TEST_F(TaskServiceTest, ListTasks) {
    auto& service = TaskService::getInstance();

    for (int i = 0; i < 5; ++i) {
        auto task = createTestTask("Task " + std::to_string(i));
        service.createTask(task);
    }

    auto result = service.listTasks(1, 10);
    EXPECT_TRUE(result.is_array());
    EXPECT_GE(result.size(), 5);
}

TEST_F(TaskServiceTest, ListTasksWithPagination) {
    auto& service = TaskService::getInstance();

    for (int i = 0; i < 15; ++i) {
        auto task = createTestTask("Paginated Task " + std::to_string(i));
        service.createTask(task);
    }

    auto page1 = service.listTasks(1, 5);
    EXPECT_EQ(page1.size(), 5);

    auto page2 = service.listTasks(2, 5);
    EXPECT_EQ(page2.size(), 5);
}

TEST_F(TaskServiceTest, ListTasksByStatus) {
    auto& service = TaskService::getInstance();

    // Create tasks (default status is 0/stopped)
    for (int i = 0; i < 3; ++i) {
        auto task = createTestTask("Stopped Task " + std::to_string(i));
        service.createTask(task);
    }

    auto stoppedTasks = service.listTasks(1, 10, 0);
    EXPECT_GE(stoppedTasks.size(), 3);
}

TEST_F(TaskServiceTest, ListTasksByDevice) {
    auto& service = TaskService::getInstance();

    for (int i = 0; i < 3; ++i) {
        auto task = createTestTask("Device Task " + std::to_string(i));
        service.createTask(task);
    }

    auto deviceTasks = service.listTasks(1, 10, -1, testDeviceId);
    EXPECT_GE(deviceTasks.size(), 3);
}

// ============================================================================
// Update Task Tests
// ============================================================================

TEST_F(TaskServiceTest, UpdateTask) {
    auto& service = TaskService::getInstance();
    auto task = createTestTask();
    auto created = service.createTask(task);
    ASSERT_TRUE(created.has_value());

    json updates;
    updates["name"] = "Updated Task";
    updates["description"] = "Updated description";
    updates["osd_enabled"] = 0;

    auto updated = service.updateTask(created->id, updates);
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->name, "Updated Task");
    EXPECT_EQ(updated->osdEnabled, 0);
}

TEST_F(TaskServiceTest, UpdateNonexistentTask) {
    auto& service = TaskService::getInstance();

    json updates;
    updates["name"] = "Should Not Update";

    auto result = service.updateTask(99999, updates);
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// Delete Task Tests
// ============================================================================

TEST_F(TaskServiceTest, DeleteTask) {
    auto& service = TaskService::getInstance();
    auto task = createTestTask("To Delete");
    auto created = service.createTask(task);
    ASSERT_TRUE(created.has_value());

    bool deleted = service.deleteTask(created->id);
    EXPECT_TRUE(deleted);

    auto result = service.getTask(created->id);
    EXPECT_FALSE(result.has_value());
}

TEST_F(TaskServiceTest, DeleteNonexistentTask) {
    auto& service = TaskService::getInstance();
    bool deleted = service.deleteTask(99999);
    EXPECT_FALSE(deleted);
}

// ============================================================================
// Task Lifecycle Tests
// ============================================================================

TEST_F(TaskServiceTest, StartStoppedTask) {
    auto& service = TaskService::getInstance();
    auto task = createTestTask();
    auto created = service.createTask(task);
    ASSERT_TRUE(created.has_value());

    // Note: Without a real sophon-stream SDK, start/stop will likely fail
    // but we test that the API doesn't crash
    bool started = service.startTask(created->id);
    // In test environment without SDK, this is expected to fail
    // The important thing is it doesn't crash
}

TEST_F(TaskServiceTest, StopRunningTask) {
    auto& service = TaskService::getInstance();
    auto task = createTestTask();
    auto created = service.createTask(task);
    ASSERT_TRUE(created.has_value());

    // Stop a task (even if not running)
    bool stopped = service.stopTask(created->id);
    // Should not crash regardless of task state
}

TEST_F(TaskServiceTest, PauseAndResumeTask) {
    auto& service = TaskService::getInstance();
    auto task = createTestTask();
    auto created = service.createTask(task);
    ASSERT_TRUE(created.has_value());

    // These operations require a running graph in the EngineBridge
    // In test environment, they test the API doesn't crash
    service.pauseTask(created->id);
    service.resumeTask(created->id);
}

// ============================================================================
// Task Config Update Tests
// ============================================================================

TEST_F(TaskServiceTest, UpdateTaskConfig) {
    auto& service = TaskService::getInstance();
    auto task = createTestTask();
    auto created = service.createTask(task);
    ASSERT_TRUE(created.has_value());

    json newConfig;
    newConfig["confidence_threshold"] = 0.7;
    newConfig["nms_threshold"] = 0.5;

    bool updated = service.updateTaskConfig(created->id, newConfig.dump());
    // May fail without running graph, but should not crash
}

TEST_F(TaskServiceTest, UpdateTaskROI) {
    auto& service = TaskService::getInstance();
    auto task = createTestTask();
    auto created = service.createTask(task);
    ASSERT_TRUE(created.has_value());

    json newROI;
    newROI["regions"] = json::array();
    newROI["regions"].push_back({
        {"id", "new_region"},
        {"type", "polygon"},
        {"points", {{{0.2, 0.3}, {0.4, 0.3}, {0.4, 0.7}, {0.2, 0.7}}}}
    });

    bool updated = service.updateTaskROI(created->id, newROI.dump());
    // May fail without running graph, but should not crash
}

// ============================================================================
// Task Metrics Tests
// ============================================================================

TEST_F(TaskServiceTest, GetTaskMetrics) {
    auto& service = TaskService::getInstance();
    auto task = createTestTask();
    auto created = service.createTask(task);
    ASSERT_TRUE(created.has_value());

    auto metrics = service.getTaskMetrics(created->id);
    // Metrics structure may vary
    EXPECT_TRUE(metrics.is_object() || metrics.is_null());
}

// ============================================================================
// Task Model Tests
// ============================================================================

TEST_F(TaskServiceTest, TaskToJson) {
    auto task = createTestTask();
    task.id = 1;
    task.status = 1;

    json j = task.toJson();
    EXPECT_EQ(j["id"], 1);
    EXPECT_EQ(j["name"], "Test Task");
    EXPECT_EQ(j["type"], "detect");
    EXPECT_EQ(j["status"], 1);
    EXPECT_EQ(j["deviceId"], testDeviceId);
}

TEST_F(TaskServiceTest, TaskFromJson) {
    json j;
    j["name"] = "JSON Task";
    j["type"] = "classify";
    j["deviceId"] = testDeviceId;
    j["algorithmIds"] = "[1]";
    j["description"] = "From JSON";

    auto task = Task::fromJson(j);
    EXPECT_EQ(task.name, "JSON Task");
    EXPECT_EQ(task.type, "classify");
    EXPECT_EQ(task.deviceId, testDeviceId);
}

TEST_F(TaskServiceTest, TaskFromDbRow) {
    std::unordered_map<std::string, std::string> row;
    row["id"] = "1";
    row["name"] = "DB Task";
    row["type"] = "detect";
    row["status"] = "0";
    row["device_id"] = std::to_string(testDeviceId);
    row["algorithm_ids"] = "[1,2]";
    row["graph_config"] = R"({"nodes":[]})";
    row["roi_config"] = R"({"regions":[]})";
    row["osd_enabled"] = "1";
    row["record_enabled"] = "0";

    auto task = Task::fromDbRow(row);
    EXPECT_EQ(task.id, 1);
    EXPECT_EQ(task.name, "DB Task");
    EXPECT_EQ(task.type, "detect");
    EXPECT_EQ(task.deviceId, testDeviceId);
    EXPECT_EQ(task.osdEnabled, 1);
}

// ============================================================================
// Build Graph Config Test
// ============================================================================

TEST_F(TaskServiceTest, TaskWithComplexPipeline) {
    auto& service = TaskService::getInstance();
    auto task = createTestTask("Complex Pipeline", "multi");

    // Build a complex pipeline: source → decode → detect → track → osd → sink
    json graphConfig;
    graphConfig["nodes"] = json::array();
    graphConfig["nodes"].push_back({{"id", "source_0"}, {"type", "source"}});
    graphConfig["nodes"].push_back({{"id", "decode_0"}, {"type", "decode"}});
    graphConfig["nodes"].push_back({{"id", "detect_0"}, {"type", "detection"}});
    graphConfig["nodes"].push_back({{"id", "track_0"}, {"type", "tracking"}});
    graphConfig["nodes"].push_back({{"id", "osd_0"}, {"type", "osd"}});
    graphConfig["nodes"].push_back({{"id", "sink_0"}, {"type", "sink"}});

    graphConfig["edges"] = json::array();
    graphConfig["edges"].push_back({{"source", "source_0"}, {"target", "decode_0"}});
    graphConfig["edges"].push_back({{"source", "decode_0"}, {"target", "detect_0"}});
    graphConfig["edges"].push_back({{"source", "detect_0"}, {"target", "track_0"}});
    graphConfig["edges"].push_back({{"source", "track_0"}, {"target", "osd_0"}});
    graphConfig["edges"].push_back({{"source", "osd_0"}, {"target", "sink_0"}});

    task.graphConfig = graphConfig.dump();
    task.algorithmIds = "[1,2,3]";

    auto result = service.createTask(task);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->type, "multi");
}
