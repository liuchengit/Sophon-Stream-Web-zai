#include "ApiController.h"
#include <drogon/drogon.h>
#include <nlohmann/json.hpp>
#include "../database/DbManager.h"
#include "../utils/JwtUtils.h"
#include "../utils/CryptoUtils.h"
#include <spdlog/spdlog.h>
#include <algorithm>
static std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
    gmtime_r(&time_t_now, &tm_now);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_now);
    return std::string(buf);
}



using namespace sophon_stream::web::controllers;
using namespace sophon_stream::web::utils;
using namespace sophon_stream::web::database;

std::string extractToken(const drogon::HttpRequestPtr& req) {
    auto authHeader = req->getHeader("Authorization");
    if (authHeader.size() > 7 && authHeader.substr(0, 7) == "Bearer ") {
        return authHeader.substr(7);
    }
    return "";
}

bool validateToken(const std::string& token, int& userId, std::string& username, std::string& role) {
    try {
        auto& jwtUtils = JwtUtils::getInstance();
        auto result = jwtUtils.verifyToken(token);
        if (result.valid && !result.expired) {
            userId = result.userId;
            username = result.username;
            role = result.role;
            return true;
        }
        return false;
    } catch (...) {
        return false;
    }
}

drogon::HttpResponsePtr buildJsonResponse(int code, const std::string& message, const nlohmann::json& data = nlohmann::json::object(), drogon::HttpStatusCode httpCode = drogon::k200OK) {
    nlohmann::json resp;
    resp["code"] = code;
    resp["message"] = message;
    resp["data"] = data;
    auto resp_ = drogon::HttpResponse::newHttpResponse();
    resp_->setStatusCode(httpCode);
    resp_->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    resp_->setBody(resp.dump());
    return resp_;
}

drogon::HttpResponsePtr buildErrorResponse(int code, const std::string& message, drogon::HttpStatusCode httpCode = drogon::k400BadRequest) {
    return buildJsonResponse(code, message, nlohmann::json::object(), httpCode);
}

void HealthCtrl::asyncHandleHttpRequest(const drogon::HttpRequestPtr& req,
                                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    callback(buildJsonResponse(0, "healthy", {{"status", "ok"}}));
}

void AuthCtrl::asyncHandleHttpRequest(const drogon::HttpRequestPtr& req,
                                      std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    try {
        auto path = req->path();
        auto method = req->method();
        auto& db = DbManager::getInstance();

        if (path == "/api/v1/auth/login" && method == drogon::Post) {
            auto body = req->getJsonObject();
            if (!body) {
                callback(buildErrorResponse(400, "Invalid JSON body"));
                return;
            }

            std::string username = (*body)["username"].asString();
            std::string password = (*body)["password"].asString();

            auto result = db.find("users", "username = ?", {username});
            if (result.rows.empty()) {
                callback(buildErrorResponse(401, "Invalid username or password", drogon::k401Unauthorized));
                return;
            }

            const auto& row = result.rows[0];
            std::string salt = row.at("salt");
            std::string inputHash = CryptoUtils::hashPassword(password, salt);

            std::string storedHash = row.at("password_hash");
            if (inputHash != storedHash) {
                callback(buildErrorResponse(401, "Invalid username or password", drogon::k401Unauthorized));
                return;
            }
            

            int userId = std::stoi(row.at("id"));
            std::string userRole = row.at("role");

            auto& jwtUtils = JwtUtils::getInstance();
            std::string accessToken = jwtUtils.generateToken(userId, username, userRole);
            std::string refreshToken = jwtUtils.generateRefreshToken(userId, username, userRole);

            nlohmann::json userData;
            userData["id"] = userId;
            userData["username"] = username;
            userData["role"] = userRole;

            nlohmann::json data;
            data["token"] = accessToken;
            data["refreshToken"] = refreshToken;
            data["user"] = userData;

            callback(buildJsonResponse(0, "success", data));
            return;
        }

        if (path == "/api/v1/auth/register" && method == drogon::Post) {
            auto body = req->getJsonObject();
            if (!body) {
                callback(buildErrorResponse(400, "Invalid JSON body"));
                return;
            }

            std::string username = (*body)["username"].asString();
            std::string password = (*body)["password"].asString();
            std::string role = body->isMember("role") ? (*body)["role"].asString() : "viewer";

            // Check if username already exists
            auto existingUser = db.find("users", "username = ?", {username});
            if (!existingUser.rows.empty()) {
                callback(buildErrorResponse(400, "Username already exists"));
                return;
            }

            // Generate salt and hash password
            std::string salt = CryptoUtils::generateSalt();
            std::string passwordHash = CryptoUtils::hashPassword(password, salt);

            // Insert new user
            auto result = db.insert("users", {
                {"username", username},
                {"password_hash", passwordHash},
                {"salt", salt},
                {"role", role}
            });

            if (result.success) {
                nlohmann::json userData;
                userData["id"] = (int64_t)result.lastInsertId;
                userData["username"] = username;
                userData["role"] = role;

                callback(buildJsonResponse(0, "success", userData, drogon::k201Created));
            } else {
                callback(buildErrorResponse(400, "Failed to create user: " + result.errorMessage));
            }
            return;
        }

        if (path == "/api/v1/auth/refresh" && method == drogon::Post) {
            auto body = req->getJsonObject();
            if (!body || !body->isMember("refreshToken")) {
                callback(buildErrorResponse(400, "Missing refreshToken"));
                return;
            }

            std::string refreshToken = (*body)["refreshToken"].asString();
            auto& jwtUtils = JwtUtils::getInstance();
            
            int userId;
            std::string username, role;
            auto refreshResult = jwtUtils.verifyToken(refreshToken);
            if (!refreshResult.valid || refreshResult.expired) {
                callback(buildErrorResponse(401, "Invalid refresh token", drogon::k401Unauthorized));
                return;
            }

            std::string newAccessToken = jwtUtils.generateToken(refreshResult.userId, refreshResult.username, refreshResult.role);
            std::string newRefreshToken = jwtUtils.generateRefreshToken(refreshResult.userId, refreshResult.username, refreshResult.role);

            nlohmann::json userData;
            userData["id"] = userId;
            userData["username"] = username;
            userData["role"] = role;

            nlohmann::json data;
            data["token"] = newAccessToken;
            data["refreshToken"] = newRefreshToken;
            data["user"] = userData;

            callback(buildJsonResponse(0, "success", data));
            return;
        }

        if (path == "/api/v1/auth/users" && method == drogon::Get) {
            int userId;
            std::string username, role;
            if (!validateToken(extractToken(req), userId, username, role) || role != "admin") {
                callback(buildErrorResponse(403, "Admin access required", drogon::k403Forbidden));
                return;
            }

            auto result = db.find("users");
            nlohmann::json array = nlohmann::json::array();
            for (const auto& row : result.rows) {
                nlohmann::json obj;
                obj["id"] = std::stoi(row.at("id"));
                obj["username"] = row.at("username");
                obj["role"] = row.at("role");
                array.push_back(obj);
            }
            callback(buildJsonResponse(0, "success", array));
            return;
        }

        callback(buildErrorResponse(404, "Not found"));

    } catch (const std::exception& e) {
        spdlog::error("AuthCtrl exception: {}", e.what());
        callback(buildErrorResponse(500, "Internal server error"));
    }
}

void DeviceCtrl::asyncHandleHttpRequest(const drogon::HttpRequestPtr& req,
                                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    try {
        std::string token = extractToken(req);
        int userId;
        std::string username, role;
        if (!validateToken(token, userId, username, role)) {
            callback(buildErrorResponse(401, "Invalid or expired token", drogon::k401Unauthorized));
            return;
        }

        auto path = req->path();
        auto method = req->method();
        auto& db = DbManager::getInstance();
        
        // Extract device ID from query parameter or path
        int deviceId = 0;
        if (req->getParameter("id") != "") {
            try { deviceId = std::stoi(req->getParameter("id")); } catch (...) {}
        } else {
            // Try to extract from path /api/v1/devices/123
            auto parts = drogon::utils::splitString(path, "/");
            if (parts.size() >= 4 && parts[0] == "api" && parts[1] == "v1" && parts[2] == "devices") {
                try { deviceId = std::stoi(parts[3]); } catch (...) {}
            }
        }

        // List devices
        if (path == "/api/v1/devices" && method == drogon::Get) {
            auto result = db.find("devices");
            nlohmann::json array = nlohmann::json::array();
            for (const auto& row : result.rows) {
                nlohmann::json obj;
                obj["id"] = std::stoi(row.at("id"));
                obj["name"] = row.at("name");
                obj["type"] = row.at("type");
                obj["streamUrl"] = row.at("stream_url");
                obj["status"] = row.at("status");
                array.push_back(obj);
            }
            callback(buildJsonResponse(0, "success", array));
            return;
        }

        // Create device
        if (path == "/api/v1/devices" && method == drogon::Post) {
            auto body = req->getJsonObject();
            if (!body) {
                callback(buildErrorResponse(400, "Invalid JSON body"));
                return;
            }

            std::string name = (*body)["name"].asString();
            std::string type = body->isMember("type") ? (*body)["type"].asString() : "IPC";
            std::string streamUrl = body->isMember("streamUrl") ? (*body)["streamUrl"].asString() : "";
            std::string protocol = body->isMember("protocol") ? (*body)["protocol"].asString() : "RTSP";
            std::string resolution = body->isMember("resolution") ? (*body)["resolution"].asString() : "";
            int fps = body->isMember("fps") ? (*body)["fps"].asInt() : 25;
            std::string codec = body->isMember("codec") ? (*body)["codec"].asString() : "H264";
            std::string status = body->isMember("status") ? (*body)["status"].asString() : "offline";
            std::string location = body->isMember("location") ? (*body)["location"].asString() : "";
            std::string description = body->isMember("description") ? (*body)["description"].asString() : "";
            std::string deviceId = body->isMember("deviceId") ? (*body)["deviceId"].asString() : "";

            auto result = db.insert("devices", {
                {"name", name},
                {"type", type},
                {"stream_url", streamUrl},
                {"protocol", protocol},
                {"resolution", resolution},
                {"fps", std::to_string(fps)},
                {"codec", codec},
                {"status", status},
                {"location", location},
                {"description", description},
                {"device_id", deviceId}
            });

            if (result.success) {
                nlohmann::json data;
                data["id"] = (int64_t)result.lastInsertId;
                data["name"] = name;
                data["type"] = type;
                data["streamUrl"] = streamUrl;
                data["protocol"] = protocol;
                data["resolution"] = resolution;
                data["fps"] = fps;
                data["codec"] = codec;
                data["status"] = status;
                data["location"] = location;
                data["description"] = description;
                data["deviceId"] = deviceId;
                data["createdAt"] = getCurrentTimestamp();
                data["updatedAt"] = getCurrentTimestamp();
                callback(buildJsonResponse(0, "success", data, drogon::k201Created));
            } else {
                callback(buildErrorResponse(400, "Failed to create device: " + result.errorMessage));
            }
            return;
        }

        // Handle /api/v1/devices/{id} routes
        if (deviceId > 0) {
            // Get single device
            if (method == drogon::Get) {
                auto result = db.findById("devices", deviceId);
                if (result.success && !result.rows.empty()) {
                    const auto& row = result.rows[0];
                    nlohmann::json obj;
                    obj["id"] = std::stoi(row.at("id"));
                    obj["name"] = row.at("name");
                    obj["type"] = row.at("type");
                    obj["streamUrl"] = row.at("stream_url");
                    obj["status"] = row.at("status");
                    obj["location"] = row.at("location");
                    obj["description"] = row.at("description");
                    callback(buildJsonResponse(0, "success", obj));
                } else {
                    callback(buildErrorResponse(404, "Device not found", drogon::k404NotFound));
                }
                return;
            }

            // Update device
            if (method == drogon::Put) {
                auto body = req->getJsonObject();
                if (!body) {
                    callback(buildErrorResponse(400, "Invalid JSON"));
                    return;
                }

                std::unordered_map<std::string, std::string> updateData;
                if (body->isMember("name")) updateData["name"] = (*body)["name"].asString();
                if (body->isMember("type")) updateData["type"] = (*body)["type"].asString();
                if (body->isMember("streamUrl")) updateData["stream_url"] = (*body)["streamUrl"].asString();
                if (body->isMember("location")) updateData["location"] = (*body)["location"].asString();
                if (body->isMember("description")) updateData["description"] = (*body)["description"].asString();
                if (body->isMember("status")) updateData["status"] = (*body)["status"].asString();

                if (updateData.empty()) {
                    callback(buildErrorResponse(400, "No fields to update"));
                    return;
                }

                auto result = db.update("devices", updateData, "id = ?", {std::to_string(deviceId)});
                if (result.success && result.affectedRows > 0) {
                    callback(buildJsonResponse(0, "success"));
                } else {
                    callback(buildErrorResponse(400, "Failed to update device"));
                }
                return;
            }

            // Delete device
            if (method == drogon::Delete) {
                auto result = db.remove("devices", "id = ?", {std::to_string(deviceId)});
                if (result.success) {
                    callback(buildJsonResponse(0, "success"));
                } else {
                    callback(buildErrorResponse(400, "Failed to delete device"));
                }
                return;
            }

            // Start device
            if (path.find("/start") != std::string::npos && method == drogon::Post) {
                callback(buildJsonResponse(0, "Device started"));
                return;
            }

            // Stop device
            if (path.find("/stop") != std::string::npos && method == drogon::Post) {
                callback(buildJsonResponse(0, "Device stopped"));
                return;
            }
        }

        callback(buildErrorResponse(404, "Not found"));

    } catch (const std::exception& e) {
        spdlog::error("DeviceCtrl exception: {}", e.what());
        callback(buildErrorResponse(500, "Internal server error", drogon::k500InternalServerError));
    }
}

void TaskCtrl::asyncHandleHttpRequest(const drogon::HttpRequestPtr& req,
                                      std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    try {
        std::string token = extractToken(req);
        int userId;
        std::string username, role;
        if (!validateToken(token, userId, username, role)) {
            callback(buildErrorResponse(401, "Invalid or expired token", drogon::k401Unauthorized));
            return;
        }

        auto path = req->path();
        auto method = req->method();
        auto& db = DbManager::getInstance();
        
        int taskId = 0;
        if (req->getParameter("id") != "") {
            try { taskId = std::stoi(req->getParameter("id")); } catch (...) {}
        } else {
            auto parts = drogon::utils::splitString(path, "/");
            if (parts.size() >= 4 && parts[0] == "api" && parts[1] == "v1" && parts[2] == "tasks") {
                try { taskId = std::stoi(parts[3]); } catch (...) {}
            }
        }

        // List tasks
        if (path == "/api/v1/tasks" && method == drogon::Get) {
            auto result = db.find("tasks");
            nlohmann::json array = nlohmann::json::array();
            for (const auto& row : result.rows) {
                nlohmann::json obj;
                obj["id"] = std::stoi(row.at("id"));
                obj["name"] = row.at("name");
                obj["status"] = row.at("status");
                obj["deviceId"] = std::stoi(row.at("device_id"));
                obj["algorithmId"] = std::stoi(row.at("algorithm_id"));
                array.push_back(obj);
            }
            callback(buildJsonResponse(0, "success", array));
            return;
        }

        // Create task
        if (path == "/api/v1/tasks" && method == drogon::Post) {
            auto body = req->getJsonObject();
            if (!body) {
                callback(buildErrorResponse(400, "Invalid JSON body"));
                return;
            }

            std::string name = (*body)["name"].asString();
            int deviceId = (*body)["deviceId"].asInt();
            int algorithmId = (*body)["algorithmId"].asInt();
            std::string status = "stopped";
            std::string config = body->isMember("config") ? (*body)["config"].toStyledString() : "{}";
            std::string description = body->isMember("description") ? (*body)["description"].asString() : "";

            auto result = db.insert("tasks", {
                {"name", name},
                {"status", status},
                {"device_id", std::to_string(deviceId)},
                {"algorithm_id", std::to_string(algorithmId)},
                {"config", config},
                {"description", description}
            });

            if (result.success) {
                nlohmann::json data;
                data["id"] = (int64_t)result.lastInsertId;
                callback(buildJsonResponse(0, "success", data, drogon::k201Created));
            } else {
                callback(buildErrorResponse(400, "Failed to create task: " + result.errorMessage));
            }
            return;
        }

        // Handle /api/v1/tasks/{id} routes
        if (taskId > 0) {
            // Get single task
            if (method == drogon::Get) {
                auto result = db.findById("tasks", taskId);
                if (result.success && !result.rows.empty()) {
                    const auto& row = result.rows[0];
                    nlohmann::json obj;
                    obj["id"] = std::stoi(row.at("id"));
                    obj["name"] = row.at("name");
                    obj["status"] = row.at("status");
                    obj["deviceId"] = std::stoi(row.at("device_id"));
                    obj["algorithmId"] = std::stoi(row.at("algorithm_id"));
                    obj["description"] = row.at("description");
                    callback(buildJsonResponse(0, "success", obj));
                } else {
                    callback(buildErrorResponse(404, "Task not found", drogon::k404NotFound));
                }
                return;
            }

            // Update task
            if (method == drogon::Put) {
                auto body = req->getJsonObject();
                if (!body) {
                    callback(buildErrorResponse(400, "Invalid JSON"));
                    return;
                }

                std::unordered_map<std::string, std::string> updateData;
                if (body->isMember("name")) updateData["name"] = (*body)["name"].asString();
                if (body->isMember("status")) updateData["status"] = (*body)["status"].asString();
                if (body->isMember("config")) updateData["config"] = (*body)["config"].toStyledString();
                if (body->isMember("description")) updateData["description"] = (*body)["description"].asString();

                if (updateData.empty()) {
                    callback(buildErrorResponse(400, "No fields to update"));
                    return;
                }

                auto result = db.update("tasks", updateData, "id = ?", {std::to_string(taskId)});
                if (result.success && result.affectedRows > 0) {
                    callback(buildJsonResponse(0, "success"));
                } else {
                    callback(buildErrorResponse(400, "Failed to update task"));
                }
                return;
            }

            // Delete task
            if (method == drogon::Delete) {
                auto result = db.remove("tasks", "id = ?", {std::to_string(taskId)});
                if (result.success) {
                    callback(buildJsonResponse(0, "success"));
                } else {
                    callback(buildErrorResponse(400, "Failed to delete task"));
                }
                return;
            }

            // Start task
            if (path.find("/start") != std::string::npos && method == drogon::Post) {
                auto result = db.update("tasks", {{"status", "running"}}, "id = ?", {std::to_string(taskId)});
                callback(buildJsonResponse(0, result.success ? "Task started" : "Failed to start task"));
                return;
            }

            // Stop task
            if (path.find("/stop") != std::string::npos && method == drogon::Post) {
                auto result = db.update("tasks", {{"status", "stopped"}}, "id = ?", {std::to_string(taskId)});
                callback(buildJsonResponse(0, result.success ? "Task stopped" : "Failed to stop task"));
                return;
            }

            // Pause task
            if (path.find("/pause") != std::string::npos && method == drogon::Post) {
                auto result = db.update("tasks", {{"status", "paused"}}, "id = ?", {std::to_string(taskId)});
                callback(buildJsonResponse(0, result.success ? "Task paused" : "Failed to pause task"));
                return;
            }

            // Resume task
            if (path.find("/resume") != std::string::npos && method == drogon::Post) {
                auto result = db.update("tasks", {{"status", "running"}}, "id = ?", {std::to_string(taskId)});
                callback(buildJsonResponse(0, result.success ? "Task resumed" : "Failed to resume task"));
                return;
            }
        }

        callback(buildErrorResponse(404, "Not found"));

    } catch (const std::exception& e) {
        spdlog::error("TaskCtrl exception: {}", e.what());
        callback(buildErrorResponse(500, "Internal server error", drogon::k500InternalServerError));
    }
}

void AlgorithmCtrl::asyncHandleHttpRequest(const drogon::HttpRequestPtr& req,
                                           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    try {
        int userId;
        std::string username, role;
        if (!validateToken(extractToken(req), userId, username, role)) {
            callback(buildErrorResponse(401, "Invalid or expired token", drogon::k401Unauthorized));
            return;
        }

        auto& db = DbManager::getInstance();
        auto result = db.find("algorithms");
        nlohmann::json array = nlohmann::json::array();
        for (const auto& row : result.rows) {
            nlohmann::json obj;
            obj["id"] = std::stoi(row.at("id"));
            obj["name"] = row.at("name");
            obj["type"] = row.at("type");
            obj["version"] = row.at("version");
            array.push_back(obj);
        }
        callback(buildJsonResponse(0, "success", array));

    } catch (const std::exception& e) {
        spdlog::error("AlgorithmCtrl exception: {}", e.what());
        callback(buildErrorResponse(500, "Internal server error"));
    }
}

void AlertCtrl::asyncHandleHttpRequest(const drogon::HttpRequestPtr& req,
                                       std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    try {
        int userId;
        std::string username, role;
        if (!validateToken(extractToken(req), userId, username, role)) {
            callback(buildErrorResponse(401, "Invalid or expired token", drogon::k401Unauthorized));
            return;
        }

        auto path = req->path();
        auto& db = DbManager::getInstance();

        if (path == "/api/v1/alerts") {
            auto result = db.find("alerts");
            auto alertsArray = nlohmann::json::array();
            for (const auto& row : result.rows) {
                auto obj = nlohmann::json::object();
                obj["id"] = std::stoi(row.at("id"));
                obj["taskId"] = std::stoi(row.at("task_id"));
                obj["type"] = row.at("type");
                obj["level"] = row.at("level");
                obj["message"] = row.at("message");
                obj["evidence"] = row.at("evidence");
                obj["acknowledged"] = row.at("acknowledged") == "true" || row.at("acknowledged") == "1";
                obj["acknowledgedBy"] = row.at("acknowledged_by");
                obj["createdAt"] = row.at("created_at");
                alertsArray.push_back(obj);
            }
            auto alertsResp = nlohmann::json::object();
            alertsResp["items"] = alertsArray;
            alertsResp["total"] = alertsArray.size();
            callback(buildJsonResponse(0, "success", alertsResp));
            return;
        }

        if (path == "/api/v1/alerts/stats") {
            nlohmann::json stats;
            stats["todayAlerts"] = 0;
            stats["totalAlerts"] = 0;
            callback(buildJsonResponse(0, "success", stats));
            return;
        }

        if (path == "/api/v1/alert-rules") {
            auto result = db.find("alert_rules");
            nlohmann::json array = nlohmann::json::array();
            for (const auto& row : result.rows) {
                nlohmann::json obj;
                obj["id"] = std::stoi(row.at("id"));
                obj["name"] = row.at("name");
                obj["type"] = row.at("type");
                array.push_back(obj);
            }
            callback(buildJsonResponse(0, "success", array));
            return;
        }

        callback(buildErrorResponse(404, "Not found"));

    } catch (const std::exception& e) {
        spdlog::error("AlertCtrl exception: {}", e.what());
        callback(buildErrorResponse(500, "Internal server error"));
    }
}

void MonitorCtrl::asyncHandleHttpRequest(const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    try {
        int userId;
        std::string username, role;
        if (!validateToken(extractToken(req), userId, username, role)) {
            callback(buildErrorResponse(401, "Invalid or expired token", drogon::k401Unauthorized));
            return;
        }

        auto path = req->path();
        auto& db = DbManager::getInstance();

        if (path == "/api/v1/monitor/system") {
            nlohmann::json metrics;
            metrics["cpuUsage"] = 45.2;
            metrics["memoryUsage"] = 62.8;
            metrics["memoryTotal"] = 16384;
            metrics["memoryUsed"] = 10288;
            metrics["tpuUsage"] = 23.1;
            metrics["gpuUsage"] = 0;
            metrics["uptime"] = 86400;
            metrics["timestamp"] = "2024-01-01T00:00:00Z";
            callback(buildJsonResponse(0, "success", metrics));
            return;
        }

        if (path == "/api/v1/dashboard") {
            auto deviceResult = db.find("devices");
            auto taskResult = db.find("tasks");
            
            int onlineDevices = 0, totalDevices = 0;
            for (const auto& row : deviceResult.rows) {
                totalDevices++;
                if (row.at("status") == "online") onlineDevices++;
            }

            int runningTasks = 0;
            nlohmann::json taskStatusDist = nlohmann::json::object();
            for (const auto& row : taskResult.rows) {
                std::string status = row.at("status");
                runningTasks += (status == "running" ? 1 : 0);
                taskStatusDist[status] = taskStatusDist.value(status, 0) + 1;
            }

            nlohmann::json dashboard;
            dashboard["totalDevices"] = totalDevices;
            dashboard["onlineDevices"] = onlineDevices;
            dashboard["runningTasks"] = runningTasks;
            dashboard["todayAlerts"] = 0;
            dashboard["systemMetrics"] = nlohmann::json{
                {"cpuUsage", 45.2}, {"memoryUsage", 62.8}, {"tpuUsage", 23.1}
            };
            dashboard["recentAlerts"] = nlohmann::json::array();
            dashboard["taskStatusDistribution"] = taskStatusDist;
            dashboard["alertTrend"] = nlohmann::json::array();
            callback(buildJsonResponse(0, "success", dashboard));
            return;
        }

        if (path == "/api/v1/system/audit-logs") {
            auto result = db.find("audit_logs");
            nlohmann::json array = nlohmann::json::array();
            for (const auto& row : result.rows) {
                nlohmann::json obj;
                obj["id"] = std::stoi(row.at("id"));
                obj["userId"] = std::stoi(row.at("user_id"));
                obj["username"] = row.at("username");
                obj["action"] = row.at("action");
                obj["resource"] = row.at("resource");
                obj["ipAddress"] = row.at("ip_address");
                obj["userAgent"] = row.at("user_agent");
                obj["createdAt"] = row.at("created_at");
                array.push_back(obj);
            }
            nlohmann::json resp;
            resp["items"] = array;
            resp["total"] = (int64_t)array.size();
            callback(buildJsonResponse(0, "success", resp));
            return;
        }

        if (path == "/api/v1/system/firmware") {
            nlohmann::json array = nlohmann::json::array();
            // 返回模拟的固件版本数据
            nlohmann::json fw1;
            fw1["id"] = 1;
            fw1["version"] = "v1.0.0";
            fw1["description"] = "初始版本";
            fw1["releaseDate"] = "2024-01-01";
            fw1["fileSize"] = "128MB";
            fw1["downloadUrl"] = "";
            fw1["isLatest"] = true;
            array.push_back(fw1);
            
            nlohmann::json fw2;
            fw2["id"] = 2;
            fw2["version"] = "v0.9.5";
            fw2["description"] = "测试版本";
            fw2["releaseDate"] = "2023-12-01";
            fw2["fileSize"] = "120MB";
            fw2["downloadUrl"] = "";
            fw2["isLatest"] = false;
            array.push_back(fw2);
            
            callback(buildJsonResponse(0, "success", array));
            return;
        }

        callback(buildErrorResponse(404, "Not found"));

    } catch (const std::exception& e) {
        spdlog::error("MonitorCtrl exception: {}", e.what());
        callback(buildErrorResponse(500, "Internal server error"));
    }
}

void PluginCtrl::asyncHandleHttpRequest(const drogon::HttpRequestPtr& req,
                                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    try {
        int userId;
        std::string username, role;
        if (!validateToken(extractToken(req), userId, username, role)) {
            callback(buildErrorResponse(401, "Invalid or expired token", drogon::k401Unauthorized));
            return;
        }

        nlohmann::json resp;
        resp["plugins"] = nlohmann::json();
        callback(buildJsonResponse(0, "success", resp));

    } catch (const std::exception& e) {
        spdlog::error("PluginCtrl exception: {}", e.what());
        callback(buildErrorResponse(500, "Internal server error"));
    }
}
