/**
 * @file MonitorWsController.cc
 * @brief WebSocket monitor controller (stub implementation).
 *
 * cpp-httplib does not support WebSocket. This is a no-op stub.
 * Real-time data is available via REST API polling endpoints.
 */

#include "websocket/MonitorWsController.h"
#include <spdlog/spdlog.h>

namespace sophon_stream {
namespace web {
namespace websocket {

// WebSocket stub - no-op
// Use REST API polling for monitor data:
//   GET /api/v1/monitor/system
//   GET /api/v1/monitor/dashboard
//   GET /api/v1/monitor/tasks/:id
//   GET /api/v1/monitor/devices/:id

}  // namespace websocket
}  // namespace web
}  // namespace sophon_stream
