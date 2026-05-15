#pragma once

/**
 * @file Auth.h
 * @brief JWT authentication helper for httplib-based routes.
 *
 * Replaces the old Drogon AuthMiddleware with simple inline helpers
 * that can be called directly from route handlers.
 */

#include "httplib/httplib.h"
#include <string>
#include "utils/JwtUtils.h"

namespace sophon_stream {
namespace web {
namespace middleware {

/**
 * Auth information extracted from a request's Authorization header.
 */
struct AuthInfo {
    bool valid = false;
    int userId = 0;
    std::string username;
    std::string role;
};

/**
 * Extract and verify Bearer token from the request's Authorization header.
 * @param req The incoming HTTP request.
 * @return AuthInfo with valid=true if token is verified.
 */
inline AuthInfo checkAuth(const httplib::Request& req) {
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

/**
 * Check if a path is whitelisted (doesn't require authentication).
 * @param path The request path.
 * @return true if the path can be accessed without auth.
 */
inline bool isWhitelistedPath(const std::string& path) {
    if (path == "/api/v1/auth/login") return true;
    if (path == "/api/v1/auth/register") return true;
    if (path == "/api/v1/auth/refresh") return true;
    if (path == "/api/v1/system/info") return true;
    if (path == "/health") return true;
    if (path == "/") return true;
    if (path.find("/ws/") == 0) return true;
    return false;
}

}  // namespace middleware
}  // namespace web
}  // namespace sophon_stream
