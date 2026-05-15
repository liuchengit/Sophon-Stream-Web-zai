// ============================================================================
// Sophon-Stream Web Management System - AuthService Unit Tests
// ============================================================================
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>

#include "services/AuthService.h"
#include "database/DbManager.h"
#include "utils/CryptoUtils.h"
#include "utils/JwtUtils.h"
#include "models/User.h"

using namespace sophon_stream::web::services;
using namespace sophon_stream::web::database;
using namespace sophon_stream::web::models;
using namespace sophon_stream::web::utils;
using json = nlohmann::json;

// ============================================================================
// Test Fixture
// ============================================================================
class AuthServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize database with a test file
        testDbPath = "/tmp/sophon_stream_test_auth_" +
                     std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) +
                     ".db";
        auto& db = DbManager::getInstance();
        db.init(testDbPath);
    }

    void TearDown() override {
        auto& db = DbManager::getInstance();
        db.close();
        std::filesystem::remove(testDbPath);
    }

    std::string testDbPath;
};

// ============================================================================
// Login Tests
// ============================================================================

TEST_F(AuthServiceTest, LoginWithValidCredentials) {
    // The seed data creates admin/admin123
    auto& authService = AuthService::getInstance();
    auto result = authService.login("admin", "admin123");

    // Verify login result structure
    EXPECT_TRUE(result.contains("access_token"));
    EXPECT_TRUE(result.contains("refresh_token"));
    EXPECT_TRUE(result.contains("user"));
    EXPECT_EQ(result["user"]["username"].get<std::string>(), "admin");
    EXPECT_EQ(result["user"]["role"].get<std::string>(), "admin");
}

TEST_F(AuthServiceTest, LoginWithInvalidPassword) {
    auto& authService = AuthService::getInstance();
    auto result = authService.login("admin", "wrongpassword");

    // Should return error response
    EXPECT_TRUE(result.contains("error"));
}

TEST_F(AuthServiceTest, LoginWithNonexistentUser) {
    auto& authService = AuthService::getInstance();
    auto result = authService.login("nonexistent", "password");

    // Should return error response
    EXPECT_TRUE(result.contains("error"));
}

TEST_F(AuthServiceTest, LoginWithEmptyCredentials) {
    auto& authService = AuthService::getInstance();

    // Empty username
    auto result1 = authService.login("", "password");
    EXPECT_TRUE(result1.contains("error"));

    // Empty password
    auto result2 = authService.login("admin", "");
    EXPECT_TRUE(result2.contains("error"));
}

// ============================================================================
// Token Tests
// ============================================================================

TEST_F(AuthServiceTest, TokenGenerationAndVerification) {
    // Generate a test token
    std::string token = JwtUtils::getInstance().generateToken(1, "admin", "admin");

    EXPECT_FALSE(token.empty());

    // Verify the token
    auto payload = JwtUtils::getInstance().verifyToken(token);
    EXPECT_TRUE(payload.has_value());

    auto claims = payload.value();
    EXPECT_EQ(claims["user_id"].get<int>(), 1);
    EXPECT_EQ(claims["username"].get<std::string>(), "admin");
    EXPECT_EQ(claims["role"].get<std::string>(), "admin");
}

TEST_F(AuthServiceTest, TokenRefreshFlow) {
    auto& authService = AuthService::getInstance();

    // Login first
    auto loginResult = authService.login("admin", "admin123");
    ASSERT_TRUE(loginResult.contains("refresh_token"));

    std::string refreshToken = loginResult["refresh_token"].get<std::string>();

    // Refresh the token
    auto refreshResult = authService.refreshToken(refreshToken);
    EXPECT_TRUE(refreshResult.contains("access_token"));
    EXPECT_TRUE(refreshResult.contains("refresh_token"));

    // New tokens should be different
    EXPECT_NE(refreshResult["access_token"].get<std::string>(),
              loginResult["access_token"].get<std::string>());
}

TEST_F(AuthServiceTest, InvalidRefreshToken) {
    auto& authService = AuthService::getInstance();
    auto result = authService.refreshToken("invalid.refresh.token");

    EXPECT_TRUE(result.contains("error"));
}

// ============================================================================
// Registration Tests
// ============================================================================

TEST_F(AuthServiceTest, RegisterNewUser) {
    auto& authService = AuthService::getInstance();
    auto result = authService.registerUser("testuser", "TestPass123!", "operator", "Test User", "test@example.com");

    // Should return the created user
    EXPECT_TRUE(result.contains("username"));
    EXPECT_EQ(result["username"].get<std::string>(), "testuser");
    EXPECT_EQ(result["role"].get<std::string>(), "operator");
}

TEST_F(AuthServiceTest, RegisterDuplicateUser) {
    auto& authService = AuthService::getInstance();

    // Register first user
    authService.registerUser("dupuser", "Password123!", "viewer");

    // Try to register with same username
    auto result = authService.registerUser("dupuser", "Password456!", "viewer");
    EXPECT_TRUE(result.contains("error"));
}

TEST_F(AuthServiceTest, RegisterWithInvalidRole) {
    auto& authService = AuthService::getInstance();
    auto result = authService.registerUser("badroleuser", "Password123!", "superadmin");

    // Should reject invalid role
    EXPECT_TRUE(result.contains("error"));
}

// ============================================================================
// Password Tests
// ============================================================================

TEST_F(AuthServiceTest, ChangePassword) {
    auto& authService = AuthService::getInstance();

    // Register a test user
    authService.registerUser("pwuser", "OldPassword123!", "operator");

    // Login to get user ID
    auto loginResult = authService.login("pwuser", "OldPassword123!");
    int userId = loginResult["user"]["id"].get<int>();

    // Change password
    bool changed = authService.changePassword(userId, "OldPassword123!", "NewPassword456!");
    EXPECT_TRUE(changed);

    // Login with new password should work
    auto newLoginResult = authService.login("pwuser", "NewPassword456!");
    EXPECT_TRUE(newLoginResult.contains("access_token"));

    // Login with old password should fail
    auto oldLoginResult = authService.login("pwuser", "OldPassword123!");
    EXPECT_TRUE(oldLoginResult.contains("error"));
}

TEST_F(AuthServiceTest, ChangePasswordWithWrongOldPassword) {
    auto& authService = AuthService::getInstance();
    bool changed = authService.changePassword(1, "wrong_old_password", "new_password");
    EXPECT_FALSE(changed);
}

// ============================================================================
// RBAC Tests
// ============================================================================

TEST_F(AuthServiceTest, AdminHasFullPermissions) {
    // Admin should have access to all resources
    EXPECT_TRUE(AuthService::checkRolePermission("admin", "device", "read"));
    EXPECT_TRUE(AuthService::checkRolePermission("admin", "device", "write"));
    EXPECT_TRUE(AuthService::checkRolePermission("admin", "device", "delete"));
    EXPECT_TRUE(AuthService::checkRolePermission("admin", "task", "read"));
    EXPECT_TRUE(AuthService::checkRolePermission("admin", "task", "write"));
    EXPECT_TRUE(AuthService::checkRolePermission("admin", "user", "manage"));
    EXPECT_TRUE(AuthService::checkRolePermission("admin", "system", "manage"));
}

TEST_F(AuthServiceTest, OperatorHasLimitedPermissions) {
    // Operator can manage devices, tasks, algorithms
    EXPECT_TRUE(AuthService::checkRolePermission("operator", "device", "read"));
    EXPECT_TRUE(AuthService::checkRolePermission("operator", "device", "write"));
    EXPECT_TRUE(AuthService::checkRolePermission("operator", "task", "read"));
    EXPECT_TRUE(AuthService::checkRolePermission("operator", "task", "write"));
    EXPECT_TRUE(AuthService::checkRolePermission("operator", "algorithm", "read"));

    // Operator cannot manage users or system
    EXPECT_FALSE(AuthService::checkRolePermission("operator", "user", "manage"));
    EXPECT_FALSE(AuthService::checkRolePermission("operator", "system", "manage"));
}

TEST_F(AuthServiceTest, ViewerHasReadOnlyPermissions) {
    // Viewer can only read
    EXPECT_TRUE(AuthService::checkRolePermission("viewer", "device", "read"));
    EXPECT_TRUE(AuthService::checkRolePermission("viewer", "task", "read"));
    EXPECT_TRUE(AuthService::checkRolePermission("viewer", "algorithm", "read"));
    EXPECT_TRUE(AuthService::checkRolePermission("viewer", "alert", "read"));

    // Viewer cannot write
    EXPECT_FALSE(AuthService::checkRolePermission("viewer", "device", "write"));
    EXPECT_FALSE(AuthService::checkRolePermission("viewer", "task", "write"));
    EXPECT_FALSE(AuthService::checkRolePermission("viewer", "device", "delete"));
}

// ============================================================================
// User CRUD Tests
// ============================================================================

TEST_F(AuthServiceTest, ListUsers) {
    auto& authService = AuthService::getInstance();
    auto result = authService.listUsers(1, 10);

    // Should have at least the admin user
    EXPECT_TRUE(result.is_array());
    EXPECT_GE(result.size(), 1);
}

TEST_F(AuthServiceTest, GetUserById) {
    auto& authService = AuthService::getInstance();
    auto user = authService.getUser(1);

    ASSERT_TRUE(user.has_value());
    EXPECT_EQ(user->username, "admin");
    EXPECT_EQ(user->role, "admin");
}

TEST_F(AuthServiceTest, GetUserByNonexistentId) {
    auto& authService = AuthService::getInstance();
    auto user = authService.getUser(99999);
    EXPECT_FALSE(user.has_value());
}

TEST_F(AuthServiceTest, UpdateUser) {
    auto& authService = AuthService::getInstance();

    json updates;
    updates["real_name"] = "Updated Name";
    updates["email"] = "updated@example.com";

    auto updated = authService.updateUser(1, updates);
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->realName, "Updated Name");
    EXPECT_EQ(updated->email, "updated@example.com");
}

TEST_F(AuthServiceTest, DeleteUser) {
    auto& authService = AuthService::getInstance();

    // Register a user to delete
    authService.registerUser("deleteuser", "Password123!", "viewer");
    auto loginResult = authService.login("deleteuser", "Password123!");
    int userId = loginResult["user"]["id"].get<int>();

    // Delete
    bool deleted = authService.deleteUser(userId);
    EXPECT_TRUE(deleted);

    // Should no longer exist
    auto user = authService.getUser(userId);
    EXPECT_FALSE(user.has_value());
}

// ============================================================================
// Crypto Utils Tests
// ============================================================================

TEST_F(AuthServiceTest, PasswordHashingConsistency) {
    std::string salt = CryptoUtils::generateSalt(32);
    std::string hash1 = CryptoUtils::hashPassword("testpassword", salt);
    std::string hash2 = CryptoUtils::hashPassword("testpassword", salt);

    // Same password + same salt should produce same hash
    EXPECT_EQ(hash1, hash2);
}

TEST_F(AuthServiceTest, PasswordHashingDifferentSalts) {
    std::string salt1 = CryptoUtils::generateSalt(32);
    std::string salt2 = CryptoUtils::generateSalt(32);
    std::string hash1 = CryptoUtils::hashPassword("testpassword", salt1);
    std::string hash2 = CryptoUtils::hashPassword("testpassword", salt2);

    // Same password + different salts should produce different hashes
    EXPECT_NE(hash1, hash2);
}

TEST_F(AuthServiceTest, SaltGeneration) {
    std::string salt1 = CryptoUtils::generateSalt(32);
    std::string salt2 = CryptoUtils::generateSalt(32);

    EXPECT_EQ(salt1.size(), 64);  // 32 bytes = 64 hex chars
    EXPECT_NE(salt1, salt2);      // Should be unique
}
