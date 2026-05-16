#include <drogon/drogon.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "common/Version.h"
#include "database/DbManager.h"
#include "utils/JwtUtils.h"

using namespace drogon;

int main() {
    try {
        auto console = spdlog::stdout_color_mt("console");
        spdlog::set_default_logger(console);
        spdlog::set_level(spdlog::level::info);

        spdlog::info("Sophon-Stream Web Management System v{}", PROJECT_VERSION);
        spdlog::info("Starting Drogon server...");

        sophon_stream::web::database::DbManager::getInstance().init("/workspace/backend/build/data/sophon_stream.db");
        spdlog::info("Database initialized");

        sophon_stream::web::utils::JwtUtils::getInstance().init("sophon-stream-secret-key-2024", "sophon-stream", 3600, 86400);
        spdlog::info("JWT utilities initialized");

        drogon::app().loadConfigFile("/workspace/backend/config/drogon_config.json");

        spdlog::info("Starting server on 0.0.0.0:8080");
        app().run();

    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        return 1;
    }

    return 0;
}
