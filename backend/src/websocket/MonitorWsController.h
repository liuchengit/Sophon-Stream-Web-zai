#pragma once

/**
 * @file MonitorWsController.h
 * @brief WebSocket monitor controller (stub).
 *
 * cpp-httplib does not natively support WebSocket connections.
 * This is a stub placeholder. Full WebSocket support requires
 * a separate WebSocket server library (e.g., libwebsocket, uWebSockets).
 *
 * For now, real-time monitoring data is available via the REST API:
 *   GET /api/v1/monitor/system
 *   GET /api/v1/monitor/dashboard
 *   GET /api/v1/monitor/tasks/:id
 *   GET /api/v1/monitor/devices/:id
 */

#include <string>
#include <nlohmann/json.hpp>

namespace sophon_stream {
namespace web {
namespace websocket {

/**
 * Stub WebSocket controller.
 * WebSocket functionality is not available with cpp-httplib.
 * Use the REST-based monitor API endpoints instead.
 */
class MonitorWsController {
public:
    MonitorWsController() = default;
    ~MonitorWsController() = default;

    /**
     * Get the singleton instance.
     */
    static MonitorWsController& getInstance() {
        static MonitorWsController instance;
        return instance;
    }

    /**
     * Start metrics broadcasting (no-op in stub).
     */
    void startMetricsBroadcast(int intervalMs = 2000) {
        (void)intervalMs;
        // WebSocket not available - use REST polling instead
    }

    /**
     * Stop metrics broadcasting (no-op in stub).
     */
    void stopMetricsBroadcast() {
        // No-op
    }

    /**
     * Get connection count (always 0 in stub).
     */
    size_t getConnectionCount() const { return 0; }
};

}  // namespace websocket
}  // namespace web
}  // namespace sophon_stream
