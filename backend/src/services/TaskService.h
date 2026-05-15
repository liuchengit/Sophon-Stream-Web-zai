#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>
#include "models/Task.h"

namespace sophon_stream {
namespace web {
namespace services {

/**
 * Task management service.
 * Handles CRUD and lifecycle (start/stop/pause/resume) via EngineBridge.
 */
class TaskService {
public:
    static TaskService& getInstance();

    std::optional<models::Task> createTask(const models::Task& task);
    std::optional<models::Task> updateTask(int id, const nlohmann::json& updates);
    bool deleteTask(int id);
    std::optional<models::Task> getTask(int id);
    nlohmann::json listTasks(int page, int pageSize,
                             int status = -1,
                             int deviceId = -1);

    /**
     * Start a task: create Graph in EngineBridge, then start it.
     */
    bool startTask(int id);

    /**
     * Stop a task: stop the Graph in EngineBridge.
     */
    bool stopTask(int id);

    /**
     * Pause a running task.
     */
    bool pauseTask(int id);

    /**
     * Resume a paused task.
     */
    bool resumeTask(int id);

    /**
     * Hot-update task configuration via EngineBridge.
     */
    bool updateTaskConfig(int id, const std::string& configJson);

    /**
     * Update ROI configuration for a task.
     */
    bool updateTaskROI(int id, const std::string& roiConfig);

    /**
     * Get runtime metrics for a task.
     */
    nlohmann::json getTaskMetrics(int id);

private:
    TaskService() = default;
    ~TaskService() = default;
    TaskService(const TaskService&) = delete;
    TaskService& operator=(const TaskService&) = delete;

    /**
     * Build a graph configuration from task data.
     */
    nlohmann::json buildGraphConfig(const models::Task& task);
};

}  // namespace services
}  // namespace web
}  // namespace sophon_stream
