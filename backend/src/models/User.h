#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace sophon_stream {
namespace web {
namespace models {

/**
 * User data model representing a system user.
 * Maps to the 'users' database table.
 */
struct User {
    int id = 0;
    std::string username;
    std::string passwordHash;
    std::string salt;
    std::string role;       // admin / operator / viewer
    std::string realName;
    std::string email;
    std::string phone;
    int status = 1;         // 1=active, 0=disabled
    std::string lastLoginAt;
    std::string createdAt;
    std::string updatedAt;

    /**
     * Serialize to JSON (excludes passwordHash and salt for security).
     */
    nlohmann::json toJson() const {
        nlohmann::json j;
        j["id"] = id;
        j["username"] = username;
        j["role"] = role;
        j["realName"] = realName;
        j["email"] = email;
        j["phone"] = phone;
        j["status"] = status;
        j["lastLoginAt"] = lastLoginAt;
        j["createdAt"] = createdAt;
        j["updatedAt"] = updatedAt;
        return j;
    }

    /**
     * Deserialize from JSON (for create/update operations).
     */
    static User fromJson(const nlohmann::json& j) {
        User u;
        if (j.contains("id") && !j["id"].is_null()) u.id = j["id"].get<int>();
        if (j.contains("username") && !j["username"].is_null()) u.username = j["username"].get<std::string>();
        if (j.contains("password") && !j["password"].is_null()) {
            // Plaintext password supplied — caller must hash
        }
        if (j.contains("role") && !j["role"].is_null()) u.role = j["role"].get<std::string>();
        if (j.contains("realName") && !j["realName"].is_null()) u.realName = j["realName"].get<std::string>();
        if (j.contains("email") && !j["email"].is_null()) u.email = j["email"].get<std::string>();
        if (j.contains("phone") && !j["phone"].is_null()) u.phone = j["phone"].get<std::string>();
        if (j.contains("status") && !j["status"].is_null()) u.status = j["status"].get<int>();
        return u;
    }

    /**
     * Construct from a database Row (unordered_map<string,string>).
     * Column names use snake_case.
     */
    static User fromDbRow(const std::unordered_map<std::string, std::string>& row) {
        User u;
        auto get = [&](const std::string& key) -> std::string {
            auto it = row.find(key);
            return it != row.end() ? it->second : "";
        };
        u.id          = !get("id").empty() ? std::stoi(get("id")) : 0;
        u.username    = get("username");
        u.passwordHash = get("password_hash");
        u.salt        = get("salt");
        u.role        = get("role");
        u.realName    = get("real_name");
        u.email       = get("email");
        u.phone       = get("phone");
        u.status      = !get("status").empty() ? std::stoi(get("status")) : 0;
        u.lastLoginAt = get("last_login_at");
        u.createdAt   = get("created_at");
        u.updatedAt   = get("updated_at");
        return u;
    }
};

}  // namespace models
}  // namespace web
}  // namespace sophon_stream
