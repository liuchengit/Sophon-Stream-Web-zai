#include "services/AuthService.h"
#include "database/DbManager.h"
#include "utils/JwtUtils.h"
#include "utils/CryptoUtils.h"
#include <spdlog/spdlog.h>
#include <chrono>

namespace sophon_stream {
namespace web {
namespace services {

AuthService& AuthService::getInstance() {
    static AuthService instance;
    return instance;
}

// ── Authentication ──────────────────────────────────────────

nlohmann::json AuthService::login(const std::string& username, const std::string& password) {
    nlohmann::json result;

    auto& db = database::DbManager::getInstance();
    auto qr = db.query(
        "SELECT id, username, password_hash, salt, role, status, real_name, email, phone, "
        "       last_login_at, created_at, updated_at "
        "FROM users WHERE username = ?",
        {username});

    if (!qr.success || qr.rows.empty()) {
        spdlog::warn("AuthService::login - user not found: {}", username);
        result["error"] = "Invalid username or password";
        return result;
    }

    auto& row = qr.rows[0];
    int userStatus = std::stoi(row.at("status"));
    if (userStatus != 1) {
        spdlog::warn("AuthService::login - account disabled: {}", username);
        result["error"] = "Account is disabled";
        return result;
    }

    std::string storedHash = row.at("password_hash");
    std::string salt = row.at("salt");
    std::string computedHash = utils::CryptoUtils::hashPassword(password, salt);

    if (!utils::CryptoUtils::constantTimeCompare(storedHash, computedHash)) {
        spdlog::warn("AuthService::login - invalid password for: {}", username);
        result["error"] = "Invalid username or password";
        return result;
    }

    int userId = std::stoi(row.at("id"));
    std::string role = row.at("role");

    // Generate tokens
    auto& jwtUtils = utils::JwtUtils::getInstance();
    std::string accessToken = jwtUtils.generateToken(userId, username, role);
    std::string refreshToken = jwtUtils.generateRefreshToken(userId, username, role);

    // Update last login
    db.execute("UPDATE users SET last_login_at = datetime('now','localtime') WHERE id = ?",
               {std::to_string(userId)});

    // Audit log
    db.insert("audit_logs", {
        {"user_id", std::to_string(userId)},
        {"action", "login"},
        {"resource_type", "auth"},
        {"detail", "User logged in"}
    });

    // Build user object
    models::User user = models::User::fromDbRow(row);

    result["access_token"] = accessToken;
    result["refresh_token"] = refreshToken;
    result["user"] = user.toJson();

    spdlog::info("AuthService::login - user '{}' logged in", username);
    return result;
}

nlohmann::json AuthService::registerUser(const std::string& username,
                                         const std::string& password,
                                         const std::string& role,
                                         const std::string& realName,
                                         const std::string& email,
                                         const std::string& phone) {
    nlohmann::json result;
    auto& db = database::DbManager::getInstance();

    // Validate inputs
    if (username.empty() || username.size() < 3) {
        result["error"] = "Username must be at least 3 characters";
        return result;
    }
    if (password.empty() || password.size() < 6) {
        result["error"] = "Password must be at least 6 characters";
        return result;
    }
    if (role != "admin" && role != "operator" && role != "viewer") {
        result["error"] = "Invalid role. Must be admin, operator, or viewer";
        return result;
    }

    // Check uniqueness
    if (db.exists("users", "username = ?", {username})) {
        result["error"] = "Username already exists";
        return result;
    }

    // Hash password
    std::string salt = utils::CryptoUtils::generateSalt(32);
    std::string passwordHash = utils::CryptoUtils::hashPassword(password, salt);

    auto qr = db.insert("users", {
        {"username",      username},
        {"password_hash", passwordHash},
        {"salt",          salt},
        {"role",          role},
        {"real_name",     realName},
        {"email",         email},
        {"phone",         phone},
        {"status",        "1"}
    });

    if (!qr.success) {
        spdlog::error("AuthService::registerUser - DB error: {}", qr.errorMessage);
        result["error"] = "Failed to create user";
        return result;
    }

    // Fetch the created user
    auto fetchResult = db.findById("users", qr.lastInsertId);
    if (!fetchResult.success || fetchResult.rows.empty()) {
        result["error"] = "Failed to retrieve created user";
        return result;
    }

    models::User user = models::User::fromDbRow(fetchResult.rows[0]);

    // Audit log
    db.insert("audit_logs", {
        {"action", "create"},
        {"resource_type", "user"},
        {"resource_id", std::to_string(qr.lastInsertId)},
        {"detail", "Created user: " + username}
    });

    result["user"] = user.toJson();
    spdlog::info("AuthService::registerUser - user '{}' created (id={})", username, qr.lastInsertId);
    return result;
}

nlohmann::json AuthService::refreshToken(const std::string& oldRefreshToken) {
    nlohmann::json result;

    auto& jwtUtils = utils::JwtUtils::getInstance();
    auto tokenInfo = jwtUtils.verifyToken(oldRefreshToken);

    if (!tokenInfo.valid || tokenInfo.expired) {
        result["error"] = "Invalid or expired refresh token";
        return result;
    }

    std::string newAccessToken = jwtUtils.generateToken(tokenInfo.userId, tokenInfo.username, tokenInfo.role);
    std::string newRefreshToken = jwtUtils.generateRefreshToken(tokenInfo.userId, tokenInfo.username, tokenInfo.role);

    result["access_token"] = newAccessToken;
    result["refresh_token"] = newRefreshToken;
    return result;
}

bool AuthService::changePassword(int userId, const std::string& oldPassword, const std::string& newPassword) {
    auto& db = database::DbManager::getInstance();
    auto qr = db.query("SELECT password_hash, salt FROM users WHERE id = ?", {std::to_string(userId)});

    if (!qr.success || qr.rows.empty()) {
        spdlog::warn("AuthService::changePassword - user not found: id={}", userId);
        return false;
    }

    std::string storedHash = qr.rows[0].at("password_hash");
    std::string salt = qr.rows[0].at("salt");
    std::string computedHash = utils::CryptoUtils::hashPassword(oldPassword, salt);

    if (!utils::CryptoUtils::constantTimeCompare(storedHash, computedHash)) {
        spdlog::warn("AuthService::changePassword - old password mismatch: id={}", userId);
        return false;
    }

    if (newPassword.size() < 6) {
        spdlog::warn("AuthService::changePassword - new password too short: id={}", userId);
        return false;
    }

    std::string newSalt = utils::CryptoUtils::generateSalt(32);
    std::string newHash = utils::CryptoUtils::hashPassword(newPassword, newSalt);

    auto updateResult = db.update("users", {
        {"password_hash", newHash},
        {"salt", newSalt}
    }, "id = ?", {std::to_string(userId)});

    if (!updateResult.success) {
        spdlog::error("AuthService::changePassword - DB update failed: id={}", userId);
        return false;
    }

    // Audit log
    db.insert("audit_logs", {
        {"user_id", std::to_string(userId)},
        {"action", "update"},
        {"resource_type", "user"},
        {"resource_id", std::to_string(userId)},
        {"detail", "Password changed"}
    });

    spdlog::info("AuthService::changePassword - password changed: id={}", userId);
    return true;
}

// ── User management ─────────────────────────────────────────

nlohmann::json AuthService::listUsers(int page, int pageSize, const std::string& keyword) {
    auto& db = database::DbManager::getInstance();

    std::string whereClause;
    std::vector<std::string> params;
    if (!keyword.empty()) {
        whereClause = "(username LIKE ? OR real_name LIKE ? OR email LIKE ?)";
        std::string pattern = "%" + keyword + "%";
        params = {pattern, pattern, pattern};
    }

    int64_t total = db.count("users", whereClause, params);
    int offset = (page - 1) * pageSize;

    auto qr = db.find("users", whereClause.empty() ? "" : whereClause, params,
                       "created_at DESC", pageSize, offset);

    nlohmann::json items = nlohmann::json::array();
    if (qr.success) {
        for (const auto& row : qr.rows) {
            models::User user = models::User::fromDbRow(row);
            items.push_back(user.toJson());
        }
    }

    return {{"items", items}, {"total", total}, {"page", page}, {"pageSize", pageSize}};
}

std::optional<models::User> AuthService::getUser(int userId) {
    auto& db = database::DbManager::getInstance();
    auto qr = db.findById("users", userId);

    if (!qr.success || qr.rows.empty()) return std::nullopt;
    return models::User::fromDbRow(qr.rows[0]);
}

std::optional<models::User> AuthService::updateUser(int userId, const nlohmann::json& updates) {
    auto& db = database::DbManager::getInstance();

    // Verify user exists
    if (!db.exists("users", "id = ?", {std::to_string(userId)})) {
        return std::nullopt;
    }

    std::unordered_map<std::string, std::string> data;
    if (updates.contains("realName") && !updates["realName"].is_null())
        data["real_name"] = updates["realName"].get<std::string>();
    if (updates.contains("email") && !updates["email"].is_null())
        data["email"] = updates["email"].get<std::string>();
    if (updates.contains("phone") && !updates["phone"].is_null())
        data["phone"] = updates["phone"].get<std::string>();
    if (updates.contains("role") && !updates["role"].is_null())
        data["role"] = updates["role"].get<std::string>();
    if (updates.contains("status") && !updates["status"].is_null())
        data["status"] = std::to_string(updates["status"].get<int>());

    if (data.empty()) {
        // Nothing to update, just return current user
        return getUser(userId);
    }

    auto qr = db.update("users", data, "id = ?", {std::to_string(userId)});
    if (!qr.success) {
        spdlog::error("AuthService::updateUser - DB error: {}", qr.errorMessage);
        return std::nullopt;
    }

    // Audit log
    db.insert("audit_logs", {
        {"action", "update"},
        {"resource_type", "user"},
        {"resource_id", std::to_string(userId)},
        {"detail", "Updated user profile"}
    });

    return getUser(userId);
}

bool AuthService::deleteUser(int userId) {
    auto& db = database::DbManager::getInstance();
    if (!db.exists("users", "id = ?", {std::to_string(userId)})) {
        return false;
    }

    auto qr = db.remove("users", "id = ?", {std::to_string(userId)});
    if (!qr.success) {
        spdlog::error("AuthService::deleteUser - DB error: {}", qr.errorMessage);
        return false;
    }

    db.insert("audit_logs", {
        {"action", "delete"},
        {"resource_type", "user"},
        {"resource_id", std::to_string(userId)},
        {"detail", "Deleted user"}
    });

    spdlog::info("AuthService::deleteUser - user id={} deleted", userId);
    return true;
}

// ── RBAC ────────────────────────────────────────────────────

bool AuthService::checkPermission(int userId, const std::string& resource, const std::string& action) {
    auto user = getUser(userId);
    if (!user) return false;
    if (user->status != 1) return false;
    return checkRolePermission(user->role, resource, action);
}

bool AuthService::checkRolePermission(const std::string& role,
                                      const std::string& resource,
                                      const std::string& action) {
    // Admin: all access
    if (role == "admin") return true;

    // Viewer: read-only
    if (role == "viewer") {
        return (action == "read" || action == "list");
    }

    // Operator: manage devices, tasks, algorithms; read alerts
    if (role == "operator") {
        if (action == "read" || action == "list") return true;
        if (resource == "device" || resource == "task" || resource == "algorithm") {
            return (action == "write" || action == "delete" || action == "manage" ||
                    action == "start" || action == "stop");
        }
        if (resource == "alert") return (action == "read" || action == "acknowledge");
        // No write access to users, system, plugins
        return false;
    }

    return false;
}

}  // namespace services
}  // namespace web
}  // namespace sophon_stream
