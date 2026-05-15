/**
 * @file main.cc
 * @brief Main entry point for the Sophon-Stream Web Management System.
 *
 * Uses cpp-httplib for HTTP server instead of Drogon.
 */

#include "httplib/httplib.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <nlohmann/json.hpp>

#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <thread>

#include "common/Version.h"
#include "database/DbManager.h"
#include "utils/JwtUtils.h"
#include "utils/CryptoUtils.h"
#include "utils/ConfigUtils.h"
#include "sophon_bridge/EngineBridge.h"
#include "Router.h"

using namespace sophon_stream::web;

namespace {

std::atomic<bool> g_shutdownRequested{false};

void initLogging(const nlohmann::json& config) {
    try {
        std::vector<spdlog::sink_ptr> sinks;

        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");
        sinks.push_back(consoleSink);

        std::string logPath = "./logs";
        if (config.contains("app") && config["app"].contains("log") &&
            config["app"]["log"].contains("log_path")) {
            logPath = config["app"]["log"]["log_path"].get<std::string>();
        }

        std::filesystem::create_directories(logPath);

        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            logPath + "/sophon-stream-web.log", 10485760, 5);
        fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] %v");
        sinks.push_back(fileSink);

        auto logger = std::make_shared<spdlog::logger>("main", sinks.begin(), sinks.end());

        std::string logLevel = "INFO";
        if (config.contains("app") && config["app"].contains("log") &&
            config["app"]["log"].contains("log_level")) {
            logLevel = config["app"]["log"]["log_level"].get<std::string>();
        }

        if (logLevel == "TRACE") logger->set_level(spdlog::level::trace);
        else if (logLevel == "DEBUG") logger->set_level(spdlog::level::debug);
        else if (logLevel == "INFO") logger->set_level(spdlog::level::info);
        else if (logLevel == "WARN" || logLevel == "WARNING") logger->set_level(spdlog::level::warn);
        else if (logLevel == "ERROR") logger->set_level(spdlog::level::err);
        else if (logLevel == "CRITICAL") logger->set_level(spdlog::level::critical);
        else logger->set_level(spdlog::level::info);

        logger->flush_on(spdlog::level::warn);
        spdlog::set_default_logger(logger);

        spdlog::info("Logging initialized (level: {}, path: {})", logLevel, logPath);
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize logging: " << e.what() << std::endl;
        auto logger = spdlog::stdout_color_mt("fallback");
        spdlog::set_default_logger(logger);
        spdlog::warn("Using fallback console logging");
    }
}

std::string getConfigPath(int argc, char* argv[]) {
    for (int i = 1; i < argc - 1; ++i) {
        std::string arg(argv[i]);
        if (arg == "--config" || arg == "-c") {
            return argv[i + 1];
        }
    }

    const char* envPath = std::getenv("SOPHON_STREAM_WEB_CONFIG");
    if (envPath != nullptr) {
        return envPath;
    }

    std::vector<std::string> defaultPaths = {
        "./config/config.json",
        "/etc/sophon-stream-web/config.json",
        "../config/config.json",
    };

    for (const auto& path : defaultPaths) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }

    return "./config/config.json";
}

bool initDatabase(const nlohmann::json& config) {
    std::string dbPath = "./data/sophon_stream.db";

    if (config.contains("app") && config["app"].contains("db_clients") &&
        config["app"]["db_clients"].is_array() && !config["app"]["db_clients"].empty()) {
        auto& dbConfig = config["app"]["db_clients"][0];
        if (dbConfig.contains("filename")) {
            dbPath = dbConfig["filename"].get<std::string>();
        }
    }

    spdlog::info("Initializing database at: {}", dbPath);

    auto& dbManager = database::DbManager::getInstance();
    if (!dbManager.init(dbPath)) {
        spdlog::error("Failed to initialize database");
        return false;
    }

    spdlog::info("Database initialized successfully");
    return true;
}

bool initJwt(const nlohmann::json& config) {
    std::string secret;
    std::string issuer = "sophon-stream-web";
    int64_t accessTokenExpire = 86400;
    int64_t refreshTokenExpire = 604800;

    if (config.contains("jwt")) {
        auto& jwtConfig = config["jwt"];
        if (jwtConfig.contains("secret")) {
            secret = jwtConfig["secret"].get<std::string>();
        }
        if (jwtConfig.contains("issuer")) {
            issuer = jwtConfig["issuer"].get<std::string>();
        }
        if (jwtConfig.contains("access_token_expire_seconds")) {
            accessTokenExpire = jwtConfig["access_token_expire_seconds"].get<int64_t>();
        }
        if (jwtConfig.contains("refresh_token_expire_seconds")) {
            refreshTokenExpire = jwtConfig["refresh_token_expire_seconds"].get<int64_t>();
        }
    }

    const char* envSecret = std::getenv("SOPHON_STREAM_JWT_SECRET");
    if (envSecret != nullptr && strlen(envSecret) > 0) {
        secret = envSecret;
        spdlog::info("Using JWT secret from environment variable");
    }

    if (secret.empty()) {
        spdlog::warn("JWT secret not configured, using default (INSECURE)");
        secret = "sophon-stream-web-default-secret-change-me";
    }

    auto& jwtUtils = utils::JwtUtils::getInstance();
    jwtUtils.init(secret, issuer, accessTokenExpire, refreshTokenExpire);

    spdlog::info("JWT authentication initialized (issuer: {}, access_expire: {}s)",
                 issuer, accessTokenExpire);
    return true;
}

bool initEngineBridge(const nlohmann::json& config) {
    nlohmann::json engineConfig;
    if (config.contains("sophon_stream")) {
        engineConfig = config["sophon_stream"];
    }

    auto& bridge = sophon_bridge::EngineBridge::getInstance();
    if (!bridge.init(engineConfig)) {
        spdlog::error("Failed to initialize Engine Bridge");
        return false;
    }

    spdlog::info("Sophon-Stream Engine Bridge initialized");
    return true;
}

void signalHandler(int signal) {
    const char* signalName = (signal == SIGINT) ? "SIGINT" : "SIGTERM";
    spdlog::info("Received {} (signal {}), initiating graceful shutdown...", signalName, signal);
    g_shutdownRequested.store(true);
}

void registerSignalHandlers() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGPIPE, SIG_IGN);
    spdlog::info("Signal handlers registered (SIGINT, SIGTERM, SIGPIPE)");
}

void gracefulShutdown() {
    spdlog::info("Performing graceful shutdown...");

    try {
        auto& bridge = sophon_bridge::EngineBridge::getInstance();
        bridge.shutdown();
        spdlog::info("Engine Bridge shutdown complete");
    } catch (const std::exception& e) {
        spdlog::error("Error shutting down Engine Bridge: {}", e.what());
    }

    try {
        auto& db = database::DbManager::getInstance();
        db.close();
        spdlog::info("Database connection closed");
    } catch (const std::exception& e) {
        spdlog::error("Error closing database: {}", e.what());
    }

    spdlog::info("Graceful shutdown complete. Goodbye!");
    spdlog::shutdown();
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [OPTIONS]\n"
              << "Options:\n"
              << "  -c, --config <path>  Path to configuration file (default: ./config/config.json)\n"
              << "  -h, --help           Show this help message\n"
              << "  -v, --version        Show version information\n"
              << "\nEnvironment Variables:\n"
              << "  SOPHON_STREAM_WEB_CONFIG   Path to configuration file\n"
              << "  SOPHON_STREAM_JWT_SECRET   JWT secret key\n";
}

}  // anonymous namespace

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        if (arg == "--version" || arg == "-v") {
            std::cout << PROJECT_NAME << " version " << PROJECT_VERSION << std::endl;
            return 0;
        }
    }

    // Load configuration
    std::string configPath = getConfigPath(argc, argv);
    auto& configUtils = utils::ConfigUtils::getInstance();

    if (!configUtils.loadFromFile(configPath)) {
        std::cerr << "Warning: Could not load config from " << configPath
                  << ", using defaults" << std::endl;
    }

    auto config = configUtils.getConfig();

    // Initialize logging
    initLogging(config);

    spdlog::info("========================================");
    spdlog::info("{} v{} starting up", PROJECT_NAME, PROJECT_VERSION);
    spdlog::info("========================================");
    spdlog::info("Config file: {}", configPath);

    // Register signal handlers
    registerSignalHandlers();

    // Initialize database
    if (!initDatabase(config)) {
        spdlog::error("Failed to initialize database, exiting");
        return 1;
    }

    // Initialize JWT
    if (!initJwt(config)) {
        spdlog::error("Failed to initialize JWT, exiting");
        return 1;
    }

    // Initialize Engine Bridge
    if (!initEngineBridge(config)) {
        spdlog::warn("Engine Bridge initialization failed (continuing without SDK)");
    }

    // Determine server configuration
    std::string host = "0.0.0.0";
    int port = 8080;
    int threads = 4;

    if (config.contains("listeners") && config["listeners"].is_array() &&
        !config["listeners"].empty()) {
        auto& listener = config["listeners"][0];
        if (listener.contains("address")) {
            host = listener["address"].get<std::string>();
        }
        if (listener.contains("port")) {
            port = listener["port"].get<int>();
        }
    }

    if (config.contains("app")) {
        auto& appConfig = config["app"];
        if (appConfig.contains("threads_num")) {
            threads = appConfig["threads_num"].get<int>();
        }
    }

    // Create HTTP server
    httplib::Server server;

    // Register all API routes
    registerRoutes(server);

    // Static file serving from ./frontend/dist
    std::string docRoot = "./frontend/dist";
    if (config.contains("app") && config["app"].contains("document_root")) {
        docRoot = config["app"]["document_root"].get<std::string>();
    }

    if (std::filesystem::exists(docRoot)) {
        server.set_mount_point("/", docRoot);
        spdlog::info("Static files served from: {}", docRoot);
    } else {
        spdlog::warn("Frontend directory not found: {} (static serving disabled)", docRoot);
    }

    spdlog::info("========================================");
    spdlog::info("Server starting on {}:{}", host, port);
    spdlog::info("========================================");

    // Run the server
    try {
        if (!server.listen(host, port)) {
            spdlog::error("Failed to start server on {}:{}", host, port);
            gracefulShutdown();
            return 1;
        }
    } catch (const std::exception& e) {
        spdlog::error("Server runtime error: {}", e.what());
    }

    // Perform graceful shutdown
    gracefulShutdown();

    return 0;
}
