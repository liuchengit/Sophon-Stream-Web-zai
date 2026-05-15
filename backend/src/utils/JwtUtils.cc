#include "utils/JwtUtils.h"
#include <spdlog/spdlog.h>
#include "jwt-cpp/traits/nlohmann-json/traits.h"

namespace sophon_stream {
namespace web {
namespace utils {

JwtUtils& JwtUtils::getInstance() {
    static JwtUtils instance;
    return instance;
}

void JwtUtils::init(const std::string& secret,
                     const std::string& issuer,
                     int64_t accessTokenExpireSeconds,
                     int64_t refreshTokenExpireSeconds) {
    secret_ = secret;
    issuer_ = issuer;
    accessTokenExpireSeconds_ = accessTokenExpireSeconds;
    refreshTokenExpireSeconds_ = refreshTokenExpireSeconds;
    initialized_ = true;

    spdlog::info("JwtUtils: Initialized with issuer='{}', access_expire={}s, refresh_expire={}s",
                 issuer_, accessTokenExpireSeconds_, refreshTokenExpireSeconds_);
}

std::string JwtUtils::generateTokenInternal(int userId, const std::string& username,
                                              const std::string& role, int64_t expireSeconds,
                                              const std::string& tokenType) {
    if (!initialized_) {
        spdlog::error("JwtUtils: Not initialized, cannot generate token");
        return "";
    }

    try {
        auto now = std::chrono::system_clock::now();
        auto expire = now + std::chrono::seconds(expireSeconds);

        auto token = jwt::create<jwt::traits::nlohmann_json>()
            .set_issuer(issuer_)
            .set_type("JWT")
            .set_subject(std::to_string(userId))
            .set_issued_at(now)
            .set_expires_at(expire)
            .set_payload_claim("user_id", std::to_string(userId))
            .set_payload_claim("username", username)
            .set_payload_claim("role", role)
            .set_payload_claim("token_type", tokenType)
            .sign(jwt::algorithm::hs256{secret_});

        return token;
    } catch (const std::exception& e) {
        spdlog::error("JwtUtils: Failed to generate token: {}", e.what());
        return "";
    }
}

std::string JwtUtils::generateToken(int userId, const std::string& username, const std::string& role) {
    return generateTokenInternal(userId, username, role, accessTokenExpireSeconds_, "access");
}

std::string JwtUtils::generateRefreshToken(int userId, const std::string& username, const std::string& role) {
    return generateTokenInternal(userId, username, role, refreshTokenExpireSeconds_, "refresh");
}

JwtTokenInfo JwtUtils::verifyToken(const std::string& token) {
    JwtTokenInfo info;

    if (!initialized_) {
        spdlog::error("JwtUtils: Not initialized, cannot verify token");
        info.valid = false;
        info.expired = false;
        return info;
    }

    if (token.empty()) {
        spdlog::warn("JwtUtils: Empty token provided");
        info.valid = false;
        return info;
    }

    try {
        auto decoded = jwt::decode<jwt::traits::nlohmann_json>(token);

        // Verify signature
        auto verifier = jwt::verify<jwt::traits::nlohmann_json>()
            .allow_algorithm(jwt::algorithm::hs256{secret_})
            .with_issuer(issuer_);

        verifier.verify(decoded);

        // Extract claims
        info.valid = true;
        info.expired = false;

        if (decoded.has_payload_claim("user_id")) {
            auto claim = decoded.get_payload_claim("user_id");
            info.userId = std::stoi(claim.as_string());
        }
        if (decoded.has_payload_claim("username")) {
            info.username = decoded.get_payload_claim("username").as_string();
        }
        if (decoded.has_payload_claim("role")) {
            info.role = decoded.get_payload_claim("role").as_string();
        }
        if (decoded.has_expires_at()) {
            auto exp = decoded.get_expires_at();
            info.expiresAt = std::chrono::system_clock::to_time_t(exp);
        }

        return info;

    } catch (const jwt::error::token_verification_exception& e) {
        // Check if it's an expired token
        std::string what = e.what();
        if (what.find("expired") != std::string::npos ||
            what.find("token_expired") != std::string::npos) {
            spdlog::warn("JwtUtils: Token expired");
            info.expired = true;
            info.valid = false;
        } else {
            spdlog::warn("JwtUtils: Token verification failed: {}", what);
            info.valid = false;
            info.expired = false;
        }

        // Try to extract claims from expired token
        try {
            auto decoded = jwt::decode<jwt::traits::nlohmann_json>(token);
            if (decoded.has_payload_claim("user_id")) {
                info.userId = std::stoi(decoded.get_payload_claim("user_id").as_string());
            }
            if (decoded.has_payload_claim("username")) {
                info.username = decoded.get_payload_claim("username").as_string();
            }
            if (decoded.has_payload_claim("role")) {
                info.role = decoded.get_payload_claim("role").as_string();
            }
        } catch (...) {
            // Ignore decoding errors for expired tokens
        }

        return info;
    } catch (const std::exception& e) {
        spdlog::warn("JwtUtils: Token verification failed: {}", e.what());
        info.valid = false;
        info.expired = false;
        return info;
    }
}

}  // namespace utils
}  // namespace web
}  // namespace sophon_stream
