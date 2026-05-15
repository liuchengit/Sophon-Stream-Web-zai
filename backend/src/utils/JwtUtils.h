#pragma once

#include <string>
#include <cstdint>
#include <chrono>

namespace sophon_stream {
namespace web {
namespace utils {

/**
 * JWT token information structure.
 */
struct JwtTokenInfo {
    bool valid = false;
    int userId = 0;
    std::string username;
    std::string role;
    bool expired = false;
    int64_t expiresAt = 0;  // Unix timestamp
};

/**
 * JWT utility class for generating and verifying JSON Web Tokens.
 * Uses the jwt-cpp library with HS256 algorithm.
 */
class JwtUtils {
public:
    /**
     * Get the singleton instance.
     */
    static JwtUtils& getInstance();

    /**
     * Initialize JWT with secret key and configuration.
     * @param secret The HS256 secret key.
     * @param issuer The token issuer string.
     * @param accessTokenExpireSeconds Access token expiry in seconds (default: 24h).
     * @param refreshTokenExpireSeconds Refresh token expiry in seconds (default: 7d).
     */
    void init(const std::string& secret,
              const std::string& issuer,
              int64_t accessTokenExpireSeconds = 86400,
              int64_t refreshTokenExpireSeconds = 604800);

    /**
     * Generate an access token for the given user.
     * @param userId User ID.
     * @param username Username.
     * @param role User role.
     * @return JWT token string.
     */
    std::string generateToken(int userId, const std::string& username, const std::string& role);

    /**
     * Generate a refresh token for the given user.
     * @param userId User ID.
     * @param username Username.
     * @param role User role.
     * @return JWT refresh token string.
     */
    std::string generateRefreshToken(int userId, const std::string& username, const std::string& role);

    /**
     * Verify a token and extract claims.
     * @param token The JWT token string.
     * @return TokenInfo with verification result and claims.
     */
    JwtTokenInfo verifyToken(const std::string& token);

    /**
     * Check if the JWT utility has been initialized.
     * @return true if initialized.
     */
    bool isInitialized() const { return initialized_; }

private:
    JwtUtils() = default;
    ~JwtUtils() = default;
    JwtUtils(const JwtUtils&) = delete;
    JwtUtils& operator=(const JwtUtils&) = delete;

    /**
     * Internal token generation with custom expiry.
     */
    std::string generateTokenInternal(int userId, const std::string& username,
                                       const std::string& role, int64_t expireSeconds,
                                       const std::string& tokenType);

    std::string secret_;
    std::string issuer_;
    int64_t accessTokenExpireSeconds_ = 86400;
    int64_t refreshTokenExpireSeconds_ = 604800;
    bool initialized_ = false;
};

}  // namespace utils
}  // namespace web
}  // namespace sophon_stream
