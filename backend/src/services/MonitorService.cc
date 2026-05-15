#include "services/MonitorService.h"
#include "database/DbManager.h"
#include "sophon_bridge/EngineBridge.h"
#include "models/Task.h"
#include "models/Device.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <unistd.h>

namespace sophon_stream {
namespace web {
namespace services {

MonitorService& MonitorService::getInstance() {
    static MonitorService instance;
    return instance;
}

double MonitorService::readCpuUsage() {
    // Read total and idle from /proc/stat
    std::ifstream ifs("/proc/stat");
    if (!ifs.is_open()) return 0.0;

    std::string line;
    std::getline(ifs, line);
    std::istringstream iss(line);

    std::string cpu;
    long user, nice, system, idle, iowait, irq, softirq, steal;
    iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

    long total1 = user + nice + system + idle + iowait + irq + softirq + steal;
    long idle1 = idle + iowait;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ifs.clear();
    ifs.seekg(0);
    std::getline(ifs, line);
    std::istringstream iss2(line);
    iss2 >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

    long total2 = user + nice + system + idle + iowait + irq + softirq + steal;
    long idle2 = idle + iowait;

    long totalDiff = total2 - total1;
    long idleDiff = idle2 - idle1;

    if (totalDiff == 0) return 0.0;
    return 100.0 * (1.0 - static_cast<double>(idleDiff) / static_cast<double>(totalDiff));
}

double MonitorService::readMemoryUsage() {
    std::ifstream ifs("/proc/meminfo");
    if (!ifs.is_open()) return 0.0;

    long memTotal = 0, memAvailable = 0;
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.find("MemTotal:") == 0) {
            std::istringstream iss(line.substr(9));
            iss >> memTotal;
        } else if (line.find("MemAvailable:") == 0) {
            std::istringstream iss(line.substr(13));
            iss >> memAvailable;
        }
        if (memTotal > 0 && memAvailable > 0) break;
    }

    if (memTotal == 0) return 0.0;
    return 100.0 * (1.0 - static_cast<double>(memAvailable) / static_cast<double>(memTotal));
}

long MonitorService::readUptimeSeconds() {
    std::ifstream ifs("/proc/uptime");
    if (!ifs.is_open()) return 0;
    double uptime;
    ifs >> uptime;
    return static_cast<long>(uptime);
}

nlohmann::json MonitorService::getSystemMetrics() {
    nlohmann::json result;

    result["cpu"]["usage"] = std::round(readCpuUsage() * 100.0) / 100.0;

    double memUsage = readMemoryUsage();
    result["memory"]["usage"] = std::round(memUsage * 100.0) / 100.0;

    // Read memory details
    std::ifstream ifs("/proc/meminfo");
    long memTotal = 0, memAvailable = 0;
    if (ifs.is_open()) {
        std::string line;
        while (std::getline(ifs, line)) {
            if (line.find("MemTotal:") == 0) {
                std::istringstream iss(line.substr(9));
                iss >> memTotal;
            } else if (line.find("MemAvailable:") == 0) {
                std::istringstream iss(line.substr(13));
                iss >> memAvailable;
            }
            if (memTotal > 0 && memAvailable > 0) break;
        }
    }
    result["memory"]["totalMB"] = memTotal / 1024;
    result["memory"]["availableMB"] = memAvailable / 1024;
    result["memory"]["usedMB"] = (memTotal - memAvailable) / 1024;

    result["uptime"] = readUptimeSeconds();

    // TPU/GPU metrics (placeholder – real implementation reads from sophon TPU driver)
    result["tpu"]["available"] = false;
    result["tpu"]["usage"] = 0.0;
    result["gpu"]["available"] = false;
    result["gpu"]["usage"] = 0.0;

    // Check if TPU device exists
    std::ifstream tpuCheck("/dev/sophon0");
    if (tpuCheck.is_open()) {
        result["tpu"]["available"] = true;
        tpuCheck.close();
    }

    // Engine status
    auto& bridge = sophon_bridge::EngineBridge::getInstance();
    auto engineStatus = bridge.getEngineStatus();
    result["engine"]["initialized"] = engineStatus.initialized;
    result["engine"]["runningGraphs"] = engineStatus.runningGraphs;
    result["engine"]["totalGraphs"] = engineStatus.totalGraphs;
    result["engine"]["version"] = engineStatus.version;

    // Process info
    result["process"]["pid"] = static_cast<int>(getpid());
    result["process"]["threadCount"] = static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));

    return result;
}

nlohmann::json MonitorService::getTaskMetrics(int taskId) {
    auto& db = database::DbManager::getInstance();
    auto qr = db.findById("tasks", taskId);
    if (!qr.success || qr.rows.empty()) {
        return {{"error", "Task not found"}};
    }

    auto task = models::Task::fromDbRow(qr.rows[0]);
    nlohmann::json result;
    result["taskId"] = taskId;
    result["taskName"] = task.name;
    result["status"] = task.status;

    int64_t graphId = -1;
    try {
        if (!task.graphConfig.empty()) {
            auto gc = nlohmann::json::parse(task.graphConfig);
            if (gc.contains("graph_id")) graphId = gc["graph_id"].get<int64_t>();
        }
    } catch (...) {}

    if (graphId >= 0) {
        auto& bridge = sophon_bridge::EngineBridge::getInstance();
        auto metrics = bridge.getGraphMetrics(graphId);
        result["fps"] = std::round(metrics.fps * 100.0) / 100.0;
        result["latencyMs"] = std::round(metrics.latencyMs * 100.0) / 100.0;
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

nlohmann::json MonitorService::getDeviceMetrics(int deviceId) {
    auto& db = database::DbManager::getInstance();
    auto qr = db.findById("devices", deviceId);
    if (!qr.success || qr.rows.empty()) {
        return {{"error", "Device not found"}};
    }

    auto device = models::Device::fromDbRow(qr.rows[0]);
    nlohmann::json result;
    result["deviceId"] = deviceId;
    result["deviceName"] = device.name;
    result["status"] = device.status;

    // Simulated metrics – in production, these come from the stream source
    result["bitrate"] = device.status == 1 ? 2048.0 : 0.0;  // kbps
    result["fps"] = device.status == 1 ? device.fps : 0.0;
    result["dropped"] = 0;
    result["lastHeartbeat"] = device.lastHeartbeat;

    return result;
}

nlohmann::json MonitorService::getAlertStats(const std::string& timeRange) {
    auto& db = database::DbManager::getInstance();

    // Convert timeRange to SQL interval
    std::string interval;
    if (timeRange == "1h") interval = "-1 hour";
    else if (timeRange == "24h") interval = "-24 hours";
    else if (timeRange == "7d") interval = "-7 days";
    else if (timeRange == "30d") interval = "-30 days";
    else interval = "-24 hours";

    nlohmann::json result;

    // By type
    auto typeResult = db.query(
        "SELECT type, COUNT(*) as cnt FROM alerts "
        "WHERE created_at >= datetime('now','localtime', ?) "
        "GROUP BY type ORDER BY cnt DESC",
        {interval});

    nlohmann::json byType = nlohmann::json::array();
    if (typeResult.success) {
        for (const auto& row : typeResult.rows) {
            byType.push_back({{"type", row.at("type")}, {"count", std::stoll(row.at("cnt"))}});
        }
    }
    result["byType"] = byType;

    // By level
    auto levelResult = db.query(
        "SELECT level, COUNT(*) as cnt FROM alerts "
        "WHERE created_at >= datetime('now','localtime', ?) "
        "GROUP BY level ORDER BY level",
        {interval});

    nlohmann::json byLevel = nlohmann::json::array();
    if (levelResult.success) {
        for (const auto& row : levelResult.rows) {
            byLevel.push_back({{"level", std::stoi(row.at("level"))}, {"count", std::stoll(row.at("cnt"))}});
        }
    }
    result["byLevel"] = byLevel;

    // Trend (hourly for 1h, daily otherwise)
    std::string trendGroup = (timeRange == "1h") ? "%Y-%m-%d %H" : "%Y-%m-%d";
    auto trendResult = db.query(
        "SELECT strftime('" + trendGroup + "', created_at) as period, COUNT(*) as cnt "
        "FROM alerts WHERE created_at >= datetime('now','localtime', ?) "
        "GROUP BY period ORDER BY period",
        {interval});

    nlohmann::json trend = nlohmann::json::array();
    if (trendResult.success) {
        for (const auto& row : trendResult.rows) {
            trend.push_back({{"period", row.at("period")}, {"count", std::stoll(row.at("cnt"))}});
        }
    }
    result["trend"] = trend;
    result["timeRange"] = timeRange;

    return result;
}

nlohmann::json MonitorService::getDashboardStats() {
    auto& db = database::DbManager::getInstance();
    auto& bridge = sophon_bridge::EngineBridge::getInstance();

    int64_t deviceCount = db.count("devices");
    int64_t deviceOnline = db.count("devices", "status = ?", {"1"});
    int64_t taskCount = db.count("tasks");
    int64_t taskRunning = db.count("tasks", "status = ?", {"1"});
    int64_t alertCount = db.count("alerts", "acknowledged = ?", {"0"});
    int64_t alertCritical = db.count("alerts", "acknowledged = ? AND level = ?", {"0", "3"});
    auto engineStatus = bridge.getEngineStatus();

    nlohmann::json result;
    result["devices"]["total"] = deviceCount;
    result["devices"]["online"] = deviceOnline;
    result["devices"]["offline"] = deviceCount - deviceOnline;
    result["tasks"]["total"] = taskCount;
    result["tasks"]["running"] = taskRunning;
    result["tasks"]["stopped"] = taskCount - taskRunning;
    result["alerts"]["unacknowledged"] = alertCount;
    result["alerts"]["critical"] = alertCritical;
    result["engine"]["initialized"] = engineStatus.initialized;
    result["engine"]["runningGraphs"] = engineStatus.runningGraphs;
    result["engine"]["totalGraphs"] = engineStatus.totalGraphs;

    return result;
}

}  // namespace services
}  // namespace web
}  // namespace sophon_stream
