#include "services/TaskService.h"
#include "database/DbManager.h"
#include "sophon_bridge/EngineBridge.h"
#include <spdlog/spdlog.h>

namespace sophon_stream {
namespace web {
namespace services {

TaskService& TaskService::getInstance() {
    static TaskService instance;
    return instance;
}

std::optional<models::Task> TaskService::createTask(const models::Task& task) {
    auto& db = database::DbManager::getInstance();

    if (task.name.empty()) {
        spdlog::warn("TaskService::createTask - name is empty");
        return std::nullopt;
    }

    auto qr = db.insert("tasks", {
        {"name",            task.name},
        {"type",            task.type.empty() ? "detect" : task.type},
        {"status",          "0"},
        {"device_id",       std::to_string(task.deviceId)},
        {"algorithm_ids",   task.algorithmIds.empty() ? "[]" : task.algorithmIds},
        {"graph_config",    task.graphConfig},
        {"schedule_config", task.scheduleConfig},
        {"roi_config",      task.roiConfig},
        {"osd_enabled",     std::to_string(task.osdEnabled)},
        {"record_enabled",  std::to_string(task.recordEnabled)},
        {"output_url",      task.outputUrl},
        {"description",     task.description}
    });

    if (!qr.success) {
        spdlog::error("TaskService::createTask - DB error: {}", qr.errorMessage);
        return std::nullopt;
    }

    db.insert("audit_logs", {
        {"action", "create"},
        {"resource_type", "task"},
        {"resource_id", std::to_string(qr.lastInsertId)},
        {"detail", "Created task: " + task.name}
    });

    return getTask(static_cast<int>(qr.lastInsertId));
}

std::optional<models::Task> TaskService::updateTask(int id, const nlohmann::json& updates) {
    auto& db = database::DbManager::getInstance();
    if (!db.exists("tasks", "id = ?", {std::to_string(id)})) return std::nullopt;

    std::unordered_map<std::string, std::string> data;
    if (updates.contains("name") && !updates["name"].is_null())
        data["name"] = updates["name"].get<std::string>();
    if (updates.contains("type") && !updates["type"].is_null())
        data["type"] = updates["type"].get<std::string>();
    if (updates.contains("deviceId") && !updates["deviceId"].is_null())
        data["device_id"] = std::to_string(updates["deviceId"].get<int>());
    if (updates.contains("algorithmIds") && !updates["algorithmIds"].is_null())
        data["algorithm_ids"] = updates["algorithmIds"].is_string()
            ? updates["algorithmIds"].get<std::string>() : updates["algorithmIds"].dump();
    if (updates.contains("graphConfig") && !updates["graphConfig"].is_null())
        data["graph_config"] = updates["graphConfig"].is_string()
            ? updates["graphConfig"].get<std::string>() : updates["graphConfig"].dump();
    if (updates.contains("scheduleConfig") && !updates["scheduleConfig"].is_null())
        data["schedule_config"] = updates["scheduleConfig"].is_string()
            ? updates["scheduleConfig"].get<std::string>() : updates["scheduleConfig"].dump();
    if (updates.contains("roiConfig") && !updates["roiConfig"].is_null())
        data["roi_config"] = updates["roiConfig"].is_string()
            ? updates["roiConfig"].get<std::string>() : updates["roiConfig"].dump();
    if (updates.contains("osdEnabled") && !updates["osdEnabled"].is_null())
        data["osd_enabled"] = std::to_string(updates["osdEnabled"].get<int>());
    if (updates.contains("recordEnabled") && !updates["recordEnabled"].is_null())
        data["record_enabled"] = std::to_string(updates["recordEnabled"].get<int>());
    if (updates.contains("outputUrl") && !updates["outputUrl"].is_null())
        data["output_url"] = updates["outputUrl"].get<std::string>();
    if (updates.contains("description") && !updates["description"].is_null())
        data["description"] = updates["description"].get<std::string>();

    if (data.empty()) return getTask(id);

    auto qr = db.update("tasks", data, "id = ?", {std::to_string(id)});
    if (!qr.success) {
        spdlog::error("TaskService::updateTask - DB error: {}", qr.errorMessage);
        return std::nullopt;
    }

    db.insert("audit_logs", {
        {"action", "update"},
        {"resource_type", "task"},
        {"resource_id", std::to_string(id)},
        {"detail", "Updated task"}
    });

    return getTask(id);
}

bool TaskService::deleteTask(int id) {
    auto& db = database::DbManager::getInstance();
    if (!db.exists("tasks", "id = ?", {std::to_string(id)})) return false;

    // Stop the task first if running
    auto task = getTask(id);
    if (task && task->status == 1) {
        stopTask(id);
    }

    auto qr = db.remove("tasks", "id = ?", {std::to_string(id)});
    if (!qr.success) {
        spdlog::error("TaskService::deleteTask - DB error: {}", qr.errorMessage);
        return false;
    }

    db.insert("audit_logs", {
        {"action", "delete"},
        {"resource_type", "task"},
        {"resource_id", std::to_string(id)},
        {"detail", "Deleted task"}
    });

    spdlog::info("TaskService::deleteTask - task id={} deleted", id);
    return true;
}

std::optional<models::Task> TaskService::getTask(int id) {
    auto& db = database::DbManager::getInstance();
    auto qr = db.findById("tasks", id);
    if (!qr.success || qr.rows.empty()) return std::nullopt;
    return models::Task::fromDbRow(qr.rows[0]);
}

nlohmann::json TaskService::listTasks(int page, int pageSize, int status, int deviceId) {
    auto& db = database::DbManager::getInstance();

    std::vector<std::string> conditions;
    std::vector<std::string> params;

    if (status >= 0) {
        conditions.push_back("status = ?");
        params.push_back(std::to_string(status));
    }
    if (deviceId >= 0) {
        conditions.push_back("device_id = ?");
        params.push_back(std::to_string(deviceId));
    }

    std::string whereClause;
    for (size_t i = 0; i < conditions.size(); ++i) {
        if (i > 0) whereClause += " AND ";
        whereClause += conditions[i];
    }

    int64_t total = db.count("tasks", whereClause, params);
    int offset = (page - 1) * pageSize;

    auto qr = db.find("tasks", whereClause.empty() ? "" : whereClause, params,
                       "created_at DESC", pageSize, offset);

    nlohmann::json items = nlohmann::json::array();
    if (qr.success) {
        for (const auto& row : qr.rows) {
            items.push_back(models::Task::fromDbRow(row).toJson());
        }
    }

    return {{"items", items}, {"total", total}, {"page", page}, {"pageSize", pageSize}};
}

nlohmann::json TaskService::buildGraphConfig(const models::Task& task) {
    nlohmann::json graphConfig;

    // If task has a graph_config already, use it
    if (!task.graphConfig.empty()) {
        try {
            graphConfig = nlohmann::json::parse(task.graphConfig);
            return graphConfig;
        } catch (...) {
            spdlog::warn("TaskService::buildGraphConfig - failed to parse graph_config for task {}", task.id);
        }
    }

    // Otherwise, build a default graph configuration
    graphConfig["graph_id"] = 0; // Will be assigned by EngineBridge
    graphConfig["name"] = task.name;

    // Source element (connects to device stream)
    nlohmann::json sourceElement;
    sourceElement["element_id"] = 1;
    sourceElement["element_name"] = "source";
    sourceElement["element_type"] = "source";
    sourceElement["config"]["url"] = ""; // Set from device stream URL
    sourceElement["config"]["device_id"] = task.deviceId;

    // Decode element (algorithm inference)
    nlohmann::json decodeElement;
    decodeElement["element_id"] = 2;
    decodeElement["element_name"] = "decode";
    decodeElement["element_type"] = "decode";

    // Algorithm inference element
    nlohmann::json inferElement;
    inferElement["element_id"] = 3;
    inferElement["element_name"] = "infer";
    inferElement["element_type"] = "infer";
    if (!task.algorithmIds.empty()) {
        try {
            inferElement["config"]["algorithm_ids"] = nlohmann::json::parse(task.algorithmIds);
        } catch (...) {}
    }

    // Sink element (output/callback)
    nlohmann::json sinkElement;
    sinkElement["element_id"] = 4;
    sinkElement["element_name"] = "sink";
    sinkElement["element_type"] = "sink";

    // Connections
    nlohmann::json dag;
    dag["1"] = nlohmann::json::array({2});
    dag["2"] = nlohmann::json::array({3});
    dag["3"] = nlohmann::json::array({4});

    graphConfig["elements"] = nlohmann::json::array({sourceElement, decodeElement, inferElement, sinkElement});
    graphConfig["dag"] = dag;

    return graphConfig;
}

bool TaskService::startTask(int id) {
    auto task = getTask(id);
    if (!task) return false;

    if (task->status == 1) {
        spdlog::warn("TaskService::startTask - task id={} already running", id);
        return true;
    }

    // Build and create graph in EngineBridge
    auto& bridge = sophon_bridge::EngineBridge::getInstance();
    nlohmann::json graphConfig = buildGraphConfig(*task);

    // Set source URL from device
    auto& db = database::DbManager::getInstance();
    auto devResult = db.findById("devices", task->deviceId);
    if (devResult.success && !devResult.rows.empty()) {
        std::string streamUrl = devResult.rows[0].at("stream_url");
        bridge.setSourceUrl(0, streamUrl); // Temporary; graphId assigned after createGraph
    }

    int64_t graphId = bridge.createGraph(graphConfig);
    if (graphId < 0) {
        spdlog::error("TaskService::startTask - failed to create graph for task id={}", id);
        return false;
    }

    if (!bridge.startGraph(graphId)) {
        spdlog::error("TaskService::startTask - failed to start graph {} for task id={}", graphId, id);
        bridge.destroyGraph(graphId);
        return false;
    }

    // Update task status and graph_id
    db.update("tasks", {
        {"status", "1"},
        {"started_at", "datetime('now','localtime')"}
    }, "id = ?", {std::to_string(id)});

    // Store graph_id in graph_config
    try {
        nlohmann::json gc = task->graphConfig.empty() ? nlohmann::json::object() : nlohmann::json::parse(task->graphConfig);
        gc["graph_id"] = graphId;
        db.update("tasks", {{"graph_config", gc.dump()}}, "id = ?", {std::to_string(id)});
    } catch (...) {}

    db.insert("audit_logs", {
        {"action", "start"},
        {"resource_type", "task"},
        {"resource_id", std::to_string(id)},
        {"detail", "Started task: " + task->name + " (graphId=" + std::to_string(graphId) + ")"}
    });

    spdlog::info("TaskService::startTask - task id={} started (graphId={})", id, graphId);
    return true;
}

bool TaskService::stopTask(int id) {
    auto task = getTask(id);
    if (!task) return false;

    // Find graph_id from graph_config
    int64_t graphId = -1;
    try {
        if (!task->graphConfig.empty()) {
            auto gc = nlohmann::json::parse(task->graphConfig);
            if (gc.contains("graph_id")) graphId = gc["graph_id"].get<int64_t>();
        }
    } catch (...) {}

    if (graphId >= 0) {
        auto& bridge = sophon_bridge::EngineBridge::getInstance();
        bridge.stopGraph(graphId);
        bridge.destroyGraph(graphId);
    }

    auto& db = database::DbManager::getInstance();
    db.update("tasks", {
        {"status", "0"},
        {"stopped_at", "datetime('now','localtime')"}
    }, "id = ?", {std::to_string(id)});

    db.insert("audit_logs", {
        {"action", "stop"},
        {"resource_type", "task"},
        {"resource_id", std::to_string(id)},
        {"detail", "Stopped task: " + task->name}
    });

    spdlog::info("TaskService::stopTask - task id={} stopped", id);
    return true;
}

bool TaskService::pauseTask(int id) {
    auto task = getTask(id);
    if (!task || task->status != 1) return false;

    int64_t graphId = -1;
    try {
        if (!task->graphConfig.empty()) {
            auto gc = nlohmann::json::parse(task->graphConfig);
            if (gc.contains("graph_id")) graphId = gc["graph_id"].get<int64_t>();
        }
    } catch (...) {}

    if (graphId >= 0) {
        auto& bridge = sophon_bridge::EngineBridge::getInstance();
        if (!bridge.pauseGraph(graphId)) return false;
    }

    auto& db = database::DbManager::getInstance();
    db.update("tasks", {{"status", "2"}}, "id = ?", {std::to_string(id)});

    db.insert("audit_logs", {
        {"action", "pause"},
        {"resource_type", "task"},
        {"resource_id", std::to_string(id)},
        {"detail", "Paused task: " + task->name}
    });

    spdlog::info("TaskService::pauseTask - task id={} paused", id);
    return true;
}

bool TaskService::resumeTask(int id) {
    auto task = getTask(id);
    if (!task || task->status != 2) return false;

    int64_t graphId = -1;
    try {
        if (!task->graphConfig.empty()) {
            auto gc = nlohmann::json::parse(task->graphConfig);
            if (gc.contains("graph_id")) graphId = gc["graph_id"].get<int64_t>();
        }
    } catch (...) {}

    if (graphId >= 0) {
        auto& bridge = sophon_bridge::EngineBridge::getInstance();
        if (!bridge.resumeGraph(graphId)) return false;
    }

    auto& db = database::DbManager::getInstance();
    db.update("tasks", {{"status", "1"}}, "id = ?", {std::to_string(id)});

    db.insert("audit_logs", {
        {"action", "resume"},
        {"resource_type", "task"},
        {"resource_id", std::to_string(id)},
        {"detail", "Resumed task: " + task->name}
    });

    spdlog::info("TaskService::resumeTask - task id={} resumed", id);
    return true;
}

bool TaskService::updateTaskConfig(int id, const std::string& configJson) {
    auto task = getTask(id);
    if (!task) return false;

    // Validate JSON
    nlohmann::json config;
    try {
        config = nlohmann::json::parse(configJson);
    } catch (const std::exception& e) {
        spdlog::error("TaskService::updateTaskConfig - invalid JSON: {}", e.what());
        return false;
    }

    // Hot-update via EngineBridge if task is running
    int64_t graphId = -1;
    try {
        if (!task->graphConfig.empty()) {
            auto gc = nlohmann::json::parse(task->graphConfig);
            if (gc.contains("graph_id")) graphId = gc["graph_id"].get<int64_t>();
        }
    } catch (...) {}

    if (graphId >= 0 && task->status == 1) {
        auto& bridge = sophon_bridge::EngineBridge::getInstance();
        // Update each element config in the graph
        if (config.contains("elements") && config["elements"].is_array()) {
            for (const auto& elem : config["elements"]) {
                int64_t elemId = elem.value("element_id", 0);
                if (elemId > 0 && elem.contains("config")) {
                    bridge.updateElementConfig(graphId, elemId, elem["config"]);
                }
            }
        }
    }

    // Persist updated config
    auto& db = database::DbManager::getInstance();
    db.update("tasks", {{"graph_config", configJson}}, "id = ?", {std::to_string(id)});

    db.insert("audit_logs", {
        {"action", "update"},
        {"resource_type", "task"},
        {"resource_id", std::to_string(id)},
        {"detail", "Hot-updated task config"}
    });

    spdlog::info("TaskService::updateTaskConfig - config updated for task id={}", id);
    return true;
}

bool TaskService::updateTaskROI(int id, const std::string& roiConfig) {
    auto task = getTask(id);
    if (!task) return false;

    // Validate JSON
    try {
        nlohmann::json::parse(roiConfig);
    } catch (const std::exception& e) {
        spdlog::error("TaskService::updateTaskROI - invalid JSON: {}", e.what());
        return false;
    }

    // Hot-update ROI via EngineBridge if running
    int64_t graphId = -1;
    try {
        if (!task->graphConfig.empty()) {
            auto gc = nlohmann::json::parse(task->graphConfig);
            if (gc.contains("graph_id")) graphId = gc["graph_id"].get<int64_t>();
        }
    } catch (...) {}

    if (graphId >= 0 && task->status == 1) {
        auto& bridge = sophon_bridge::EngineBridge::getInstance();
        try {
            bridge.setRoiConfig(graphId, nlohmann::json::parse(roiConfig));
        } catch (...) {}
    }

    auto& db = database::DbManager::getInstance();
    db.update("tasks", {{"roi_config", roiConfig}}, "id = ?", {std::to_string(id)});

    db.insert("audit_logs", {
        {"action", "update"},
        {"resource_type", "task"},
        {"resource_id", std::to_string(id)},
        {"detail", "Updated task ROI config"}
    });

    spdlog::info("TaskService::updateTaskROI - ROI updated for task id={}", id);
    return true;
}

nlohmann::json TaskService::getTaskMetrics(int id) {
    auto task = getTask(id);
    if (!task) return {};

    nlohmann::json result;
    result["taskId"] = id;
    result["status"] = task->status;

    int64_t graphId = -1;
    try {
        if (!task->graphConfig.empty()) {
            auto gc = nlohmann::json::parse(task->graphConfig);
            if (gc.contains("graph_id")) graphId = gc["graph_id"].get<int64_t>();
        }
    } catch (...) {}

    if (graphId >= 0) {
        auto& bridge = sophon_bridge::EngineBridge::getInstance();
        auto metrics = bridge.getGraphMetrics(graphId);
        result["fps"] = metrics.fps;
        result["latencyMs"] = metrics.latencyMs;
        result["throughput"] = metrics.throughput;
        result["totalFrames"] = metrics.totalFrames;
        result["droppedFrames"] = metrics.droppedFrames;
    } else {
        result["fps"] = 0.0;
        result["latencyMs"] = 0.0;
        result["throughput"] = 0;
        result["totalFrames"] = 0;
        result["droppedFrames"] = 0;
    }

    return result;
}

}  // namespace services
}  // namespace web
}  // namespace sophon_stream
