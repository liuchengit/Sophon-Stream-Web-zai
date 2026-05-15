/**
 * @file Router.cc
 * @brief Unified route registration for the Sophon-Stream Web API.
 *
 * Registers all API routes with httplib::Server, including CORS middleware,
 * JWT authentication, and delegates to service/controller layer.
 */

#include "Router.h"
#include "common/Version.h"

#include "httplib/httplib.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "utils/JwtUtils.h"
#include "database/DbManager.h"
#include "services/AuthService.h"
#include "services/DeviceService.h"
#include "services/TaskService.h"
#include "services/AlgorithmService.h"
#include "services/AlertService.h"
#include "services/MonitorService.h"
#include "services/PluginService.h"
#include "services/SystemService.h"
#include "sophon_bridge/EngineBridge.h"

// Model headers (for fromJson / fromDbRow)
#include "models/User.h"
#include "models/Device.h"
#include "models/Task.h"
#include "models/Algorithm.h"
#include "models/Alert.h"
#include "models/AlertRule.h"
#include "models/Plugin.h"
#include "models/SystemConfig.h"
#include "models/AuditLog.h"
#include "models/FirmwareVersion.h"

namespace sophon_stream {
namespace web {

namespace {

// ── CORS & Auth helpers ────────────────────────────────────────────────────

void setCORSHeaders(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
    res.set_header("Access-Control-Max-Age", "86400");
    res.set_header("Access-Control-Allow-Credentials", "true");
    res.set_header("Access-Control-Expose-Headers", "Content-Length, Content-Type, X-Request-Id");
}

struct AuthInfo {
    bool valid = false;
    int userId = 0;
    std::string username;
    std::string role;
};

AuthInfo checkAuth(const httplib::Request& req) {
    AuthInfo info;
    std::string auth = req.get_header_value("Authorization");
    if (auth.empty()) return info;
    if (auth.size() <= 7 || auth.substr(0, 7) != "Bearer ") return info;
    std::string token = auth.substr(7);
    auto& jwtUtils = utils::JwtUtils::getInstance();
    auto tokenInfo = jwtUtils.verifyToken(token);
    if (!tokenInfo.valid) return info;
    info.valid = true;
    info.userId = tokenInfo.userId;
    info.username = tokenInfo.username;
    info.role = tokenInfo.role;
    return info;
}

bool isWhitelistedPath(const std::string& path) {
    if (path == "/api/v1/auth/login") return true;
    if (path == "/api/v1/auth/register") return true;
    if (path == "/api/v1/auth/refresh") return true;
    if (path == "/api/v1/system/info") return true;
    if (path == "/health") return true;
    if (path == "/") return true;
    if (path.find("/ws/") == 0) return true;
    return false;
}

// ── Response helpers ────────────────────────────────────────────────────────

void setJsonResponse(httplib::Response& res, int code, const std::string& message,
                     const nlohmann::json& data = nullptr, int httpStatus = 200) {
    nlohmann::json resp;
    resp["code"] = code;
    resp["message"] = message;
    resp["data"] = data.is_null() ? nullptr : data;
    res.status = httpStatus;
    setCORSHeaders(res);
    res.set_content(resp.dump(), "application/json");
}

void setErrorResponse(httplib::Response& res, int code, const std::string& message, int httpStatus = 400) {
    setJsonResponse(res, code, message, nullptr, httpStatus);
}

// Helper to get query param with default
std::string getQueryParam(const httplib::Request& req, const std::string& key, const std::string& def = "") {
    if (!req.has_param(key)) return def;
    return req.get_param_value(key);
}

int getIntParam(const httplib::Request& req, const std::string& key, int def = 0) {
    std::string val = getQueryParam(req, key, "");
    if (val.empty()) return def;
    try { return std::stoi(val); } catch (...) { return def; }
}

}  // anonymous namespace

// ── Main route registration ────────────────────────────────────────────────

void registerRoutes(httplib::Server& server) {
    spdlog::info("Registering API routes...");

    // ── Global OPTIONS handler (CORS preflight) ────────────────────────────
    server.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        setCORSHeaders(res);
        res.status = 204;
    });

    // ── Health check ───────────────────────────────────────────────────────
    server.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        nlohmann::json data;
        data["status"] = "healthy";
        data["version"] = PROJECT_VERSION;
        data["uptime"] = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        setJsonResponse(res, 0, "success", data);
    });

    // ══════════════════════════════════════════════════════════════════════
    // AUTH ROUTES
    // ══════════════════════════════════════════════════════════════════════

    server.Post("/api/v1/auth/login", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string username = body.value("username", "");
            std::string password = body.value("password", "");
            if (username.empty() || password.empty()) {
                return setErrorResponse(res, 1001, "Username and password are required", 400);
            }
            auto& service = services::AuthService::getInstance();
            auto result = service.login(username, password);
            if (result.contains("error")) {
                std::string error = result["error"].get<std::string>();
                int code = 401;
                int status = 401;
                if (error.find("disabled") != std::string::npos) { code = 403; status = 403; }
                return setErrorResponse(res, code, error, status);
            }
            setJsonResponse(res, 0, "success", result);
        } catch (const std::exception& e) {
            spdlog::error("Auth login error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Post("/api/v1/auth/register", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || auth.role != "admin") {
            return setErrorResponse(res, 403, "Only administrators can register new users", 403);
        }
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string username = body.value("username", "");
            std::string password = body.value("password", "");
            std::string role = body.value("role", "operator");
            std::string realName = body.value("realName", "");
            std::string email = body.value("email", "");
            std::string phone = body.value("phone", "");
            if (username.empty() || password.empty()) {
                return setErrorResponse(res, 1001, "Username and password are required", 400);
            }
            auto& service = services::AuthService::getInstance();
            auto result = service.registerUser(username, password, role, realName, email, phone);
            if (result.contains("error")) {
                return setErrorResponse(res, 1002, result["error"].get<std::string>(), 400);
            }
            setJsonResponse(res, 0, "success", result["user"], 201);
        } catch (const std::exception& e) {
            spdlog::error("Auth register error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Post("/api/v1/auth/refresh", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string refreshTokenStr = body.value("refresh_token", "");
            if (refreshTokenStr.empty()) {
                return setErrorResponse(res, 1001, "Refresh token is required", 400);
            }
            auto& service = services::AuthService::getInstance();
            auto result = service.refreshToken(refreshTokenStr);
            if (result.contains("error")) {
                return setErrorResponse(res, 401, result["error"].get<std::string>(), 401);
            }
            setJsonResponse(res, 0, "success", result);
        } catch (const std::exception& e) {
            spdlog::error("Auth refresh error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Put("/api/v1/auth/password", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid) {
            return setErrorResponse(res, 401, "Authentication required", 401);
        }
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string oldPassword = body.value("oldPassword", "");
            std::string newPassword = body.value("newPassword", "");
            if (oldPassword.empty() || newPassword.empty()) {
                return setErrorResponse(res, 1001, "Old and new passwords are required", 400);
            }
            auto& service = services::AuthService::getInstance();
            if (!service.changePassword(auth.userId, oldPassword, newPassword)) {
                return setErrorResponse(res, 401, "Invalid old password or new password too short", 400);
            }
            setJsonResponse(res, 0, "Password changed successfully");
        } catch (const std::exception& e) {
            spdlog::error("Auth password change error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    // ── User management routes ─────────────────────────────────────────────
    server.Get("/api/v1/auth/users", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || auth.role != "admin") {
            return setErrorResponse(res, 403, "Only administrators can list users", 403);
        }
        try {
            int page = getIntParam(req, "page", 1);
            int pageSize = getIntParam(req, "pageSize", 20);
            std::string keyword = getQueryParam(req, "keyword", "");
            auto& service = services::AuthService::getInstance();
            setJsonResponse(res, 0, "success", service.listUsers(page, pageSize, keyword));
        } catch (const std::exception& e) {
            spdlog::error("List users error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Get(R"(/api/v1/auth/users/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid) return setErrorResponse(res, 401, "Authentication required", 401);
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::AuthService::getInstance();
            auto user = service.getUser(id);
            if (!user) return setErrorResponse(res, 1003, "User not found", 404);
            setJsonResponse(res, 0, "success", user->toJson());
        } catch (const std::exception& e) {
            spdlog::error("Get user error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Put(R"(/api/v1/auth/users/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid) return setErrorResponse(res, 401, "Authentication required", 401);
        try {
            int id = std::stoi(req.matches[1]);
            if (auth.role != "admin" && auth.userId != id) {
                return setErrorResponse(res, 403, "Cannot update other users' profiles", 403);
            }
            auto body = nlohmann::json::parse(req.body);
            if (auth.role != "admin" && body.contains("role")) {
                return setErrorResponse(res, 403, "Cannot change your own role", 403);
            }
            auto& service = services::AuthService::getInstance();
            auto user = service.updateUser(id, body);
            if (!user) return setErrorResponse(res, 1003, "User not found", 404);
            setJsonResponse(res, 0, "success", user->toJson());
        } catch (const std::exception& e) {
            spdlog::error("Update user error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Delete(R"(/api/v1/auth/users/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || auth.role != "admin") {
            return setErrorResponse(res, 403, "Only administrators can delete users", 403);
        }
        try {
            int id = std::stoi(req.matches[1]);
            if (auth.userId == id) return setErrorResponse(res, 1001, "Cannot delete your own account", 400);
            auto& service = services::AuthService::getInstance();
            if (!service.deleteUser(id)) return setErrorResponse(res, 1003, "User not found", 404);
            setJsonResponse(res, 0, "User deleted successfully");
        } catch (const std::exception& e) {
            spdlog::error("Delete user error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    // ══════════════════════════════════════════════════════════════════════
    // DEVICE ROUTES
    // ══════════════════════════════════════════════════════════════════════

    server.Get("/api/v1/devices", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid) return setErrorResponse(res, 401, "Authentication required", 401);
        try {
            int page = getIntParam(req, "page", 1);
            int pageSize = getIntParam(req, "pageSize", 20);
            std::string type = getQueryParam(req, "type", "");
            int status = getIntParam(req, "status", -1);
            std::string keyword = getQueryParam(req, "keyword", "");
            auto& service = services::DeviceService::getInstance();
            setJsonResponse(res, 0, "success", service.listDevices(page, pageSize, type, status, keyword));
        } catch (const std::exception& e) {
            spdlog::error("List devices error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Post("/api/v1/devices", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "device", "write")) {
            return setErrorResponse(res, 403, "Insufficient permissions", 403);
        }
        try {
            auto body = nlohmann::json::parse(req.body);
            auto device = models::Device::fromJson(body);
            if (device.name.empty() || device.streamUrl.empty()) {
                return setErrorResponse(res, 1001, "Device name and stream URL are required", 400);
            }
            auto& service = services::DeviceService::getInstance();
            auto result = service.createDevice(device);
            if (!result) return setErrorResponse(res, 500, "Failed to create device", 500);
            setJsonResponse(res, 0, "success", result->toJson(), 201);
        } catch (const std::exception& e) {
            spdlog::error("Create device error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Get(R"(/api/v1/devices/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid) return setErrorResponse(res, 401, "Authentication required", 401);
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::DeviceService::getInstance();
            auto result = service.getDevice(id);
            if (!result) return setErrorResponse(res, 1003, "Device not found", 404);
            setJsonResponse(res, 0, "success", result->toJson());
        } catch (const std::exception& e) {
            spdlog::error("Get device error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Put(R"(/api/v1/devices/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "device", "write")) {
            return setErrorResponse(res, 403, "Insufficient permissions", 403);
        }
        try {
            int id = std::stoi(req.matches[1]);
            auto body = nlohmann::json::parse(req.body);
            auto& service = services::DeviceService::getInstance();
            auto result = service.updateDevice(id, body);
            if (!result) return setErrorResponse(res, 1003, "Device not found", 404);
            setJsonResponse(res, 0, "success", result->toJson());
        } catch (const std::exception& e) {
            spdlog::error("Update device error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Delete(R"(/api/v1/devices/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "device", "delete")) {
            return setErrorResponse(res, 403, "Insufficient permissions", 403);
        }
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::DeviceService::getInstance();
            if (!service.deleteDevice(id)) return setErrorResponse(res, 1003, "Device not found", 404);
            setJsonResponse(res, 0, "Device deleted successfully");
        } catch (const std::exception& e) {
            spdlog::error("Delete device error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Post(R"(/api/v1/devices/(\d+)/start)", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "device", "start")) {
            return setErrorResponse(res, 403, "Insufficient permissions", 403);
        }
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::DeviceService::getInstance();
            if (!service.startStream(id)) return setErrorResponse(res, 1003, "Device not found or start failed", 404);
            setJsonResponse(res, 0, "Stream started successfully");
        } catch (const std::exception& e) {
            spdlog::error("Start device error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Post(R"(/api/v1/devices/(\d+)/stop)", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "device", "stop")) {
            return setErrorResponse(res, 403, "Insufficient permissions", 403);
        }
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::DeviceService::getInstance();
            if (!service.stopStream(id)) return setErrorResponse(res, 1003, "Device not found", 404);
            setJsonResponse(res, 0, "Stream stopped successfully");
        } catch (const std::exception& e) {
            spdlog::error("Stop device error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Get(R"(/api/v1/devices/(\d+)/status)", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid) return setErrorResponse(res, 401, "Authentication required", 401);
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::DeviceService::getInstance();
            auto result = service.checkDeviceStatus(id);
            if (result.is_null() || result.empty()) return setErrorResponse(res, 1003, "Device not found", 404);
            setJsonResponse(res, 0, "success", result);
        } catch (const std::exception& e) {
            spdlog::error("Device status error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    // ══════════════════════════════════════════════════════════════════════
    // TASK ROUTES
    // ══════════════════════════════════════════════════════════════════════

    server.Get("/api/v1/tasks", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid) return setErrorResponse(res, 401, "Authentication required", 401);
        try {
            int page = getIntParam(req, "page", 1);
            int pageSize = getIntParam(req, "pageSize", 20);
            int status = getIntParam(req, "status", -1);
            int deviceId = getIntParam(req, "deviceId", -1);
            auto& service = services::TaskService::getInstance();
            setJsonResponse(res, 0, "success", service.listTasks(page, pageSize, status, deviceId));
        } catch (const std::exception& e) {
            spdlog::error("List tasks error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Post("/api/v1/tasks", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "task", "write")) {
            return setErrorResponse(res, 403, "Insufficient permissions", 403);
        }
        try {
            auto body = nlohmann::json::parse(req.body);
            auto task = models::Task::fromJson(body);
            if (task.name.empty()) return setErrorResponse(res, 1001, "Task name is required", 400);
            if (task.deviceId <= 0) return setErrorResponse(res, 1001, "Valid device ID is required", 400);
            auto& service = services::TaskService::getInstance();
            auto result = service.createTask(task);
            if (!result) return setErrorResponse(res, 500, "Failed to create task", 500);
            setJsonResponse(res, 0, "success", result->toJson(), 201);
        } catch (const std::exception& e) {
            spdlog::error("Create task error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Get(R"(/api/v1/tasks/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid) return setErrorResponse(res, 401, "Authentication required", 401);
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::TaskService::getInstance();
            auto result = service.getTask(id);
            if (!result) return setErrorResponse(res, 1003, "Task not found", 404);
            setJsonResponse(res, 0, "success", result->toJson());
        } catch (const std::exception& e) {
            spdlog::error("Get task error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Put(R"(/api/v1/tasks/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "task", "write")) {
            return setErrorResponse(res, 403, "Insufficient permissions", 403);
        }
        try {
            int id = std::stoi(req.matches[1]);
            auto body = nlohmann::json::parse(req.body);
            auto& service = services::TaskService::getInstance();
            auto result = service.updateTask(id, body);
            if (!result) return setErrorResponse(res, 1003, "Task not found", 404);
            setJsonResponse(res, 0, "success", result->toJson());
        } catch (const std::exception& e) {
            spdlog::error("Update task error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Delete(R"(/api/v1/tasks/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "task", "delete")) {
            return setErrorResponse(res, 403, "Insufficient permissions", 403);
        }
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::TaskService::getInstance();
            if (!service.deleteTask(id)) return setErrorResponse(res, 1003, "Task not found", 404);
            setJsonResponse(res, 0, "Task deleted successfully");
        } catch (const std::exception& e) {
            spdlog::error("Delete task error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Post(R"(/api/v1/tasks/(\d+)/start)", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "task", "start")) {
            return setErrorResponse(res, 403, "Insufficient permissions", 403);
        }
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::TaskService::getInstance();
            if (!service.startTask(id)) return setErrorResponse(res, 500, "Failed to start task", 500);
            setJsonResponse(res, 0, "Task started successfully");
        } catch (const std::exception& e) {
            spdlog::error("Start task error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Post(R"(/api/v1/tasks/(\d+)/stop)", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "task", "stop")) {
            return setErrorResponse(res, 403, "Insufficient permissions", 403);
        }
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::TaskService::getInstance();
            if (!service.stopTask(id)) return setErrorResponse(res, 500, "Failed to stop task", 500);
            setJsonResponse(res, 0, "Task stopped successfully");
        } catch (const std::exception& e) {
            spdlog::error("Stop task error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Post(R"(/api/v1/tasks/(\d+)/pause)", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "task", "manage")) {
            return setErrorResponse(res, 403, "Insufficient permissions", 403);
        }
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::TaskService::getInstance();
            if (!service.pauseTask(id)) return setErrorResponse(res, 400, "Failed to pause task (task may not be running)", 400);
            setJsonResponse(res, 0, "Task paused successfully");
        } catch (const std::exception& e) {
            spdlog::error("Pause task error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Post(R"(/api/v1/tasks/(\d+)/resume)", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "task", "manage")) {
            return setErrorResponse(res, 403, "Insufficient permissions", 403);
        }
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::TaskService::getInstance();
            if (!service.resumeTask(id)) return setErrorResponse(res, 400, "Failed to resume task (task may not be paused)", 400);
            setJsonResponse(res, 0, "Task resumed successfully");
        } catch (const std::exception& e) {
            spdlog::error("Resume task error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Put(R"(/api/v1/tasks/(\d+)/config)", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "task", "write")) {
            return setErrorResponse(res, 403, "Insufficient permissions", 403);
        }
        try {
            int id = std::stoi(req.matches[1]);
            auto body = nlohmann::json::parse(req.body);
            std::string configJson = body.contains("config") && !body["config"].is_null()
                ? (body["config"].is_string() ? body["config"].get<std::string>() : body["config"].dump())
                : body.dump();
            auto& service = services::TaskService::getInstance();
            if (!service.updateTaskConfig(id, configJson)) return setErrorResponse(res, 500, "Failed to update task config", 500);
            setJsonResponse(res, 0, "Task config updated successfully");
        } catch (const std::exception& e) {
            spdlog::error("Update task config error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Put(R"(/api/v1/tasks/(\d+)/roi)", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "task", "write")) {
            return setErrorResponse(res, 403, "Insufficient permissions", 403);
        }
        try {
            int id = std::stoi(req.matches[1]);
            auto body = nlohmann::json::parse(req.body);
            std::string roiConfig = body.contains("roi") && !body["roi"].is_null()
                ? (body["roi"].is_string() ? body["roi"].get<std::string>() : body["roi"].dump())
                : body.dump();
            auto& service = services::TaskService::getInstance();
            if (!service.updateTaskROI(id, roiConfig)) return setErrorResponse(res, 500, "Failed to update ROI config", 500);
            setJsonResponse(res, 0, "ROI config updated successfully");
        } catch (const std::exception& e) {
            spdlog::error("Update ROI config error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Get(R"(/api/v1/tasks/(\d+)/metrics)", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid) return setErrorResponse(res, 401, "Authentication required", 401);
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::TaskService::getInstance();
            auto result = service.getTaskMetrics(id);
            if (result.is_null() || result.empty()) return setErrorResponse(res, 1003, "Task not found", 404);
            setJsonResponse(res, 0, "success", result);
        } catch (const std::exception& e) {
            spdlog::error("Get task metrics error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    // ══════════════════════════════════════════════════════════════════════
    // ALGORITHM ROUTES
    // ══════════════════════════════════════════════════════════════════════

    server.Get("/api/v1/algorithms", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid) return setErrorResponse(res, 401, "Authentication required", 401);
        try {
            int page = getIntParam(req, "page", 1);
            int pageSize = getIntParam(req, "pageSize", 20);
            std::string type = getQueryParam(req, "type", "");
            auto& service = services::AlgorithmService::getInstance();
            setJsonResponse(res, 0, "success", service.listAlgorithms(page, pageSize, type));
        } catch (const std::exception& e) {
            spdlog::error("List algorithms error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Post("/api/v1/algorithms", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "algorithm", "write")) {
            return setErrorResponse(res, 403, "Insufficient permissions", 403);
        }
        try {
            auto body = nlohmann::json::parse(req.body);
            auto algo = models::Algorithm::fromJson(body);
            if (algo.name.empty() || algo.modelPath.empty()) {
                return setErrorResponse(res, 1001, "Algorithm name and model path are required", 400);
            }
            auto& service = services::AlgorithmService::getInstance();
            auto result = service.createAlgorithm(algo);
            if (!result) return setErrorResponse(res, 1002, "Algorithm already exists or creation failed", 400);
            setJsonResponse(res, 0, "success", result->toJson(), 201);
        } catch (const std::exception& e) {
            spdlog::error("Create algorithm error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Get(R"(/api/v1/algorithms/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid) return setErrorResponse(res, 401, "Authentication required", 401);
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::AlgorithmService::getInstance();
            auto result = service.getAlgorithm(id);
            if (!result) return setErrorResponse(res, 1003, "Algorithm not found", 404);
            setJsonResponse(res, 0, "success", result->toJson());
        } catch (const std::exception& e) {
            spdlog::error("Get algorithm error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Put(R"(/api/v1/algorithms/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "algorithm", "write")) {
            return setErrorResponse(res, 403, "Insufficient permissions", 403);
        }
        try {
            int id = std::stoi(req.matches[1]);
            auto body = nlohmann::json::parse(req.body);
            auto& service = services::AlgorithmService::getInstance();
            auto result = service.updateAlgorithm(id, body);
            if (!result) return setErrorResponse(res, 1003, "Algorithm not found", 404);
            setJsonResponse(res, 0, "success", result->toJson());
        } catch (const std::exception& e) {
            spdlog::error("Update algorithm error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Delete(R"(/api/v1/algorithms/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "algorithm", "delete")) {
            return setErrorResponse(res, 403, "Insufficient permissions", 403);
        }
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::AlgorithmService::getInstance();
            if (!service.deleteAlgorithm(id)) return setErrorResponse(res, 1003, "Algorithm not found", 404);
            setJsonResponse(res, 0, "Algorithm deleted successfully");
        } catch (const std::exception& e) {
            spdlog::error("Delete algorithm error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Get("/api/v1/algorithms/types", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid) return setErrorResponse(res, 401, "Authentication required", 401);
        try {
            auto types = services::AlgorithmService::getAlgorithmTypes();
            nlohmann::json typesJson = types;
            setJsonResponse(res, 0, "success", typesJson);
        } catch (const std::exception& e) {
            spdlog::error("Get algorithm types error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Post(R"(/api/v1/algorithms/(\d+)/validate)", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid) return setErrorResponse(res, 401, "Authentication required", 401);
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::AlgorithmService::getInstance();
            auto algo = service.getAlgorithm(id);
            if (!algo) return setErrorResponse(res, 1003, "Algorithm not found", 404);
            auto result = service.validateModel(algo->modelPath);
            setJsonResponse(res, 0, "success", result);
        } catch (const std::exception& e) {
            spdlog::error("Validate algorithm error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    // ══════════════════════════════════════════════════════════════════════
    // ALERT ROUTES
    // ══════════════════════════════════════════════════════════════════════

    server.Get("/api/v1/alerts", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid) return setErrorResponse(res, 401, "Authentication required", 401);
        try {
            int page = getIntParam(req, "page", 1);
            int pageSize = getIntParam(req, "pageSize", 20);
            int taskId = getIntParam(req, "taskId", -1);
            int level = getIntParam(req, "level", -1);
            std::string type = getQueryParam(req, "type", "");
            int acknowledged = getIntParam(req, "acknowledged", -1);
            auto& service = services::AlertService::getInstance();
            setJsonResponse(res, 0, "success", service.listAlerts(page, pageSize, taskId, level, type, acknowledged));
        } catch (const std::exception& e) {
            spdlog::error("List alerts error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Put(R"(/api/v1/alerts/(\d+)/acknowledge)", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "alert", "acknowledge")) {
            return setErrorResponse(res, 403, "Insufficient permissions", 403);
        }
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::AlertService::getInstance();
            if (!service.acknowledgeAlert(id)) return setErrorResponse(res, 1003, "Alert not found", 404);
            setJsonResponse(res, 0, "Alert acknowledged successfully");
        } catch (const std::exception& e) {
            spdlog::error("Acknowledge alert error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Post("/api/v1/alerts/batch-acknowledge", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "alert", "acknowledge")) {
            return setErrorResponse(res, 403, "Insufficient permissions", 403);
        }
        try {
            auto body = nlohmann::json::parse(req.body);
            if (!body.contains("ids") || !body["ids"].is_array()) {
                return setErrorResponse(res, 1001, "ids array is required", 400);
            }
            std::vector<int> ids;
            for (const auto& id : body["ids"]) ids.push_back(id.get<int>());
            auto& service = services::AlertService::getInstance();
            int count = service.batchAcknowledgeAlerts(ids);
            setJsonResponse(res, 0, "success", {{"acknowledged", count}});
        } catch (const std::exception& e) {
            spdlog::error("Batch acknowledge error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Get("/api/v1/alerts/rules", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid) return setErrorResponse(res, 401, "Authentication required", 401);
        try {
            int taskId = getIntParam(req, "taskId", 0);
            if (taskId <= 0) return setErrorResponse(res, 1001, "taskId query parameter is required", 400);
            int page = getIntParam(req, "page", 1);
            int pageSize = getIntParam(req, "pageSize", 20);
            auto& service = services::AlertService::getInstance();
            setJsonResponse(res, 0, "success", service.listAlertRules(taskId, page, pageSize));
        } catch (const std::exception& e) {
            spdlog::error("List alert rules error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Post("/api/v1/alerts/rules", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "alert", "write")) {
            return setErrorResponse(res, 403, "Insufficient permissions", 403);
        }
        try {
            auto body = nlohmann::json::parse(req.body);
            auto rule = models::AlertRule::fromJson(body);
            if (rule.name.empty() || rule.taskId <= 0) {
                return setErrorResponse(res, 1001, "Rule name and taskId are required", 400);
            }
            auto& service = services::AlertService::getInstance();
            auto result = service.createAlertRule(rule);
            if (!result) return setErrorResponse(res, 500, "Failed to create alert rule", 500);
            setJsonResponse(res, 0, "success", result->toJson(), 201);
        } catch (const std::exception& e) {
            spdlog::error("Create alert rule error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Get("/api/v1/alerts/stats", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid) return setErrorResponse(res, 401, "Authentication required", 401);
        try {
            std::string timeRange = getQueryParam(req, "timeRange", "24h");
            auto& service = services::AlertService::getInstance();
            setJsonResponse(res, 0, "success", service.getAlertStats(timeRange));
        } catch (const std::exception& e) {
            spdlog::error("Alert stats error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    // ══════════════════════════════════════════════════════════════════════
    // MONITOR ROUTES
    // ══════════════════════════════════════════════════════════════════════

    server.Get("/api/v1/monitor/system", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid) return setErrorResponse(res, 401, "Authentication required", 401);
        try {
            auto& service = services::MonitorService::getInstance();
            setJsonResponse(res, 0, "success", service.getSystemMetrics());
        } catch (const std::exception& e) {
            spdlog::error("Monitor system metrics error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Get("/api/v1/monitor/dashboard", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid) return setErrorResponse(res, 401, "Authentication required", 401);
        try {
            auto& service = services::MonitorService::getInstance();
            setJsonResponse(res, 0, "success", service.getDashboardStats());
        } catch (const std::exception& e) {
            spdlog::error("Monitor dashboard error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Get(R"(/api/v1/monitor/tasks/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid) return setErrorResponse(res, 401, "Authentication required", 401);
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::MonitorService::getInstance();
            auto result = service.getTaskMetrics(id);
            if (result.contains("error")) return setErrorResponse(res, 1003, result["error"].get<std::string>(), 404);
            setJsonResponse(res, 0, "success", result);
        } catch (const std::exception& e) {
            spdlog::error("Monitor task metrics error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Get(R"(/api/v1/monitor/devices/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid) return setErrorResponse(res, 401, "Authentication required", 401);
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::MonitorService::getInstance();
            auto result = service.getDeviceMetrics(id);
            if (result.contains("error")) return setErrorResponse(res, 1003, result["error"].get<std::string>(), 404);
            setJsonResponse(res, 0, "success", result);
        } catch (const std::exception& e) {
            spdlog::error("Monitor device metrics error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    // ══════════════════════════════════════════════════════════════════════
    // PLUGIN ROUTES
    // ══════════════════════════════════════════════════════════════════════

    server.Get("/api/v1/plugins", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid) return setErrorResponse(res, 401, "Authentication required", 401);
        try {
            int page = getIntParam(req, "page", 1);
            int pageSize = getIntParam(req, "pageSize", 20);
            std::string type = getQueryParam(req, "type", "");
            auto& service = services::PluginService::getInstance();
            setJsonResponse(res, 0, "success", service.listPlugins(page, pageSize, type));
        } catch (const std::exception& e) {
            spdlog::error("List plugins error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Post("/api/v1/plugins", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "plugin", "write")) {
            return setErrorResponse(res, 403, "Insufficient permissions (admin only)", 403);
        }
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string name = body.value("name", "");
            std::string soPath = body.value("soPath", "");
            std::string config = body.contains("config") && !body["config"].is_null()
                ? (body["config"].is_string() ? body["config"].get<std::string>() : body["config"].dump())
                : "";
            if (name.empty() || soPath.empty()) {
                return setErrorResponse(res, 1001, "Plugin name and soPath are required", 400);
            }
            auto& service = services::PluginService::getInstance();
            auto result = service.installPlugin(name, soPath, config);
            if (!result) return setErrorResponse(res, 1002, "Plugin already exists or validation failed", 400);
            setJsonResponse(res, 0, "success", result->toJson(), 201);
        } catch (const std::exception& e) {
            spdlog::error("Install plugin error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Get(R"(/api/v1/plugins/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid) return setErrorResponse(res, 401, "Authentication required", 401);
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::PluginService::getInstance();
            auto result = service.getPlugin(id);
            if (!result) return setErrorResponse(res, 1003, "Plugin not found", 404);
            setJsonResponse(res, 0, "success", result->toJson());
        } catch (const std::exception& e) {
            spdlog::error("Get plugin error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Delete(R"(/api/v1/plugins/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "plugin", "delete")) {
            return setErrorResponse(res, 403, "Insufficient permissions (admin only)", 403);
        }
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::PluginService::getInstance();
            if (!service.uninstallPlugin(id)) return setErrorResponse(res, 1003, "Plugin not found", 404);
            setJsonResponse(res, 0, "Plugin uninstalled successfully");
        } catch (const std::exception& e) {
            spdlog::error("Uninstall plugin error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Post(R"(/api/v1/plugins/(\d+)/activate)", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "plugin", "write")) {
            return setErrorResponse(res, 403, "Insufficient permissions (admin only)", 403);
        }
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::PluginService::getInstance();
            if (!service.activatePlugin(id)) return setErrorResponse(res, 1003, "Plugin not found or activation failed", 404);
            setJsonResponse(res, 0, "Plugin activated successfully");
        } catch (const std::exception& e) {
            spdlog::error("Activate plugin error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Post(R"(/api/v1/plugins/(\d+)/deactivate)", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "plugin", "write")) {
            return setErrorResponse(res, 403, "Insufficient permissions (admin only)", 403);
        }
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::PluginService::getInstance();
            if (!service.deactivatePlugin(id)) return setErrorResponse(res, 1003, "Plugin not found", 404);
            setJsonResponse(res, 0, "Plugin deactivated successfully");
        } catch (const std::exception& e) {
            spdlog::error("Deactivate plugin error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    // ══════════════════════════════════════════════════════════════════════
    // SYSTEM ROUTES
    // ══════════════════════════════════════════════════════════════════════

    server.Get("/api/v1/system/info", [](const httplib::Request&, httplib::Response& res) {
        try {
            auto& service = services::SystemService::getInstance();
            setJsonResponse(res, 0, "success", service.getSystemInfo());
        } catch (const std::exception& e) {
            spdlog::error("System info error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Get("/api/v1/system/config", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "system", "read")) {
            return setErrorResponse(res, 403, "Insufficient permissions", 403);
        }
        try {
            auto& service = services::SystemService::getInstance();
            setJsonResponse(res, 0, "success", service.getAllConfig());
        } catch (const std::exception& e) {
            spdlog::error("System config error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Put("/api/v1/system/config", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "system", "write")) {
            return setErrorResponse(res, 403, "Insufficient permissions (admin only)", 403);
        }
        try {
            auto body = nlohmann::json::parse(req.body);
            if (!body.contains("key") || !body.contains("value")) {
                return setErrorResponse(res, 1001, "key and value are required", 400);
            }
            auto& service = services::SystemService::getInstance();
            std::string key = body["key"].get<std::string>();
            std::string value = body["value"].is_string()
                ? body["value"].get<std::string>() : body["value"].dump();
            if (!service.setConfig(key, value)) return setErrorResponse(res, 500, "Failed to update config", 500);
            setJsonResponse(res, 0, "Config updated successfully");
        } catch (const std::exception& e) {
            spdlog::error("Update system config error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Get("/api/v1/system/audit-logs", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "system", "read")) {
            return setErrorResponse(res, 403, "Insufficient permissions", 403);
        }
        try {
            int page = getIntParam(req, "page", 1);
            int pageSize = getIntParam(req, "pageSize", 20);
            int userId = getIntParam(req, "userId", -1);
            std::string action = getQueryParam(req, "action", "");
            auto& service = services::SystemService::getInstance();
            setJsonResponse(res, 0, "success", service.listAuditLogs(page, pageSize, userId, action));
        } catch (const std::exception& e) {
            spdlog::error("List audit logs error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Post("/api/v1/system/backup", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "system", "write")) {
            return setErrorResponse(res, 403, "Insufficient permissions (admin only)", 403);
        }
        try {
            auto& service = services::SystemService::getInstance();
            std::string filePath = service.backupDatabase();
            if (filePath.empty()) return setErrorResponse(res, 500, "Failed to backup database", 500);
            setJsonResponse(res, 0, "success", {{"filePath", filePath}});
        } catch (const std::exception& e) {
            spdlog::error("Backup database error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Post("/api/v1/system/restore", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "system", "write")) {
            return setErrorResponse(res, 403, "Insufficient permissions (admin only)", 403);
        }
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string filePath = body.value("filePath", "");
            if (filePath.empty()) return setErrorResponse(res, 1001, "filePath is required", 400);
            auto& service = services::SystemService::getInstance();
            if (!service.restoreDatabase(filePath)) return setErrorResponse(res, 500, "Failed to restore database", 500);
            setJsonResponse(res, 0, "Database restore initiated. System restart may be required.");
        } catch (const std::exception& e) {
            spdlog::error("Restore database error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Get("/api/v1/system/firmware", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "system", "read")) {
            return setErrorResponse(res, 403, "Insufficient permissions", 403);
        }
        try {
            std::string type = getQueryParam(req, "type", "");
            auto& service = services::SystemService::getInstance();
            auto versions = service.listFirmwareVersions(type);
            nlohmann::json items = nlohmann::json::array();
            for (const auto& v : versions) items.push_back(v.toJson());
            setJsonResponse(res, 0, "success", items);
        } catch (const std::exception& e) {
            spdlog::error("List firmware error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Post(R"(/api/v1/system/firmware/(\d+)/install)", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || !services::AuthService::checkRolePermission(auth.role, "system", "write")) {
            return setErrorResponse(res, 403, "Insufficient permissions (admin only)", 403);
        }
        try {
            int id = std::stoi(req.matches[1]);
            auto& service = services::SystemService::getInstance();
            if (!service.installFirmware(id)) return setErrorResponse(res, 1003, "Firmware version not found", 404);
            setJsonResponse(res, 0, "Firmware installation initiated");
        } catch (const std::exception& e) {
            spdlog::error("Install firmware error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    server.Post("/api/v1/system/reboot", [](const httplib::Request& req, httplib::Response& res) {
        auto auth = checkAuth(req);
        if (!auth.valid || auth.role != "admin") {
            return setErrorResponse(res, 403, "Only administrators can reboot the system", 403);
        }
        try {
            auto& service = services::SystemService::getInstance();
            service.reboot();
            setJsonResponse(res, 0, "System reboot initiated");
        } catch (const std::exception& e) {
            spdlog::error("Reboot error: {}", e.what());
            setErrorResponse(res, 500, "Internal server error", 500);
        }
    });

    spdlog::info("All API routes registered successfully");
    spdlog::info("  Auth:     /api/v1/auth/*");
    spdlog::info("  Devices:  /api/v1/devices/*");
    spdlog::info("  Tasks:    /api/v1/tasks/*");
    spdlog::info("  Algorithms: /api/v1/algorithms/*");
    spdlog::info("  Alerts:   /api/v1/alerts/*");
    spdlog::info("  Monitor:  /api/v1/monitor/*");
    spdlog::info("  Plugins:  /api/v1/plugins/*");
    spdlog::info("  System:   /api/v1/system/*");
}

}  // namespace web
}  // namespace sophon_stream
