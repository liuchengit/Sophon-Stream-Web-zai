#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace sophon_stream {
namespace web {
namespace services {

/**
 * Monitoring service.
 * Provides system metrics, task/device metrics, alert stats, and dashboard summary.
 */
class MonitorService {
public:
    static MonitorService& getInstance();

    /**
     * Get system-level metrics (CPU, memory, TPU, GPU, uptime).
     */
    nlohmann::json getSystemMetrics();

    /**
     * Get runtime metrics for a specific task.
     */
    nlohmann::json getTaskMetrics(int taskId);

    /**
     * Get metrics for a specific device.
     */
    nlohmann::json getDeviceMetrics(int deviceId);

    /**
     * Get alert statistics (by type, by level, trend).
     * @param timeRange e.g. "1h", "24h", "7d", "30d"
     */
    nlohmann::json getAlertStats(const std::string& timeRange = "24h");

    /**
     * Get dashboard summary stats.
     */
    nlohmann::json getDashboardStats();

private:
    MonitorService() = default;
    ~MonitorService() = default;
    MonitorService(const MonitorService&) = delete;
    MonitorService& operator=(const MonitorService&) = delete;

    /**
     * Read a value from /proc/stat or /proc/meminfo.
     */
    static double readCpuUsage();
    static double readMemoryUsage();
    static long readUptimeSeconds();
};

}  // namespace services
}  // namespace web
}  // namespace sophon_stream
