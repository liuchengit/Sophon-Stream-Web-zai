#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>
#include "models/User.h"

namespace sophon_stream {
namespace web {
namespace services {

/**
 * Authentication and user management service.
 * Handles login, registration, token refresh, password changes,
 * user CRUD, and RBAC permission checks.
 */
class AuthService {
public:
    static AuthService& getInstance();

    // ── Authentication ──────────────────────────────────────

    /**
     * Authenticate a user and return tokens.
     * @return JSON with access_token, refresh_token, and user object.
     */
    nlohmann::json login(const std::string& username, const std::string& password);

    /**
     * Register a new user.
     * @return Created User (JSON).
     */
    nlohmann::json registerUser(const std::string& username,
                                const std::string& password,
                                const std::string& role,
                                const std::string& realName = "",
                                const std::string& email = "",
                                const std::string& phone = "");

    /**
     * Refresh an access token using a valid refresh token.
     * @return JSON with new access_token and refresh_token.
     */
    nlohmann::json refreshToken(const std::string& oldRefreshToken);

    /**
     * Change a user's password.
     * @return true on success.
     */
    bool changePassword(int userId, const std::string& oldPassword, const std::string& newPassword);

    // ── User management ─────────────────────────────────────

    /**
     * List users with pagination and optional keyword search.
     */
    nlohmann::json listUsers(int page, int pageSize, const std::string& keyword = "");

    /**
     * Get a user by ID.
     */
    std::optional<models::User> getUser(int userId);

    /**
     * Update a user's profile.
     */
    std::optional<models::User> updateUser(int userId, const nlohmann::json& updates);

    /**
     * Delete a user by ID.
     */
    bool deleteUser(int userId);

    // ── RBAC ────────────────────────────────────────────────

    /**
     * Check if a user has permission to perform an action on a resource.
     * @param userId   User ID.
     * @param resource Resource type (e.g. "device", "task", "algorithm").
     * @param action   Action (e.g. "read", "write", "delete", "manage").
     * @return true if permitted.
     */
    bool checkPermission(int userId, const std::string& resource, const std::string& action);

    /**
     * Convenience: check permission by role string.
     */
    static bool checkRolePermission(const std::string& role,
                                    const std::string& resource,
                                    const std::string& action);

private:
    AuthService() = default;
    ~AuthService() = default;
    AuthService(const AuthService&) = delete;
    AuthService& operator=(const AuthService&) = delete;
};

}  // namespace services
}  // namespace web
}  // namespace sophon_stream
