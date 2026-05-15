#include "utils/ConfigUtils.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>

namespace sophon_stream {
namespace web {
namespace utils {

ConfigUtils& ConfigUtils::getInstance() {
    static ConfigUtils instance;
    return instance;
}

std::vector<std::string> ConfigUtils::splitKey(const std::string& key) {
    std::vector<std::string> parts;
    std::istringstream iss(key);
    std::string part;
    while (std::getline(iss, part, '.')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    return parts;
}

bool ConfigUtils::loadFromFile(const std::string& filePath) {
    std::unique_lock<std::shared_mutex> lock(configMutex_);

    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            spdlog::error("ConfigUtils: Failed to open config file: {}", filePath);
            return false;
        }

        nlohmann::json newConfig;
        file >> newConfig;

        nlohmann::json oldConfig = config_;
        config_ = newConfig;
        filePath_ = filePath;

        // Notify callbacks after releasing the lock
        lock.unlock();

        for (const auto& cb : changeCallbacks_) {
            try {
                cb(oldConfig, newConfig);
            } catch (const std::exception& e) {
                spdlog::error("ConfigUtils: Change callback error: {}", e.what());
            }
        }

        spdlog::info("ConfigUtils: Loaded config from {}", filePath);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("ConfigUtils: Failed to parse config file {}: {}", filePath, e.what());
        return false;
    }
}

bool ConfigUtils::reload() {
    std::shared_lock<std::shared_mutex> lock(configMutex_);
    if (filePath_.empty()) {
        spdlog::warn("ConfigUtils: No file path set for reload");
        return false;
    }
    std::string path = filePath_;
    lock.unlock();

    return loadFromFile(path);
}

bool ConfigUtils::saveToFile() {
    std::unique_lock<std::shared_mutex> lock(configMutex_);

    if (filePath_.empty()) {
        spdlog::warn("ConfigUtils: No file path set for save");
        return false;
    }

    try {
        std::ofstream file(filePath_);
        if (!file.is_open()) {
            spdlog::error("ConfigUtils: Failed to open config file for writing: {}", filePath_);
            return false;
        }

        file << config_.dump(2);
        spdlog::info("ConfigUtils: Saved config to {}", filePath_);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("ConfigUtils: Failed to save config: {}", e.what());
        return false;
    }
}

bool ConfigUtils::has(const std::string& key) const {
    std::shared_lock<std::shared_mutex> lock(configMutex_);
    try {
        nlohmann::json current = config_;
        auto parts = splitKey(key);
        for (const auto& part : parts) {
            if (current.is_object() && current.contains(part)) {
                current = current[part];
            } else {
                return false;
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

nlohmann::json ConfigUtils::getConfig() const {
    std::shared_lock<std::shared_mutex> lock(configMutex_);
    return config_;
}

void ConfigUtils::setConfig(const nlohmann::json& config) {
    std::unique_lock<std::shared_mutex> lock(configMutex_);
    nlohmann::json oldConfig = config_;
    config_ = config;

    lock.unlock();

    for (const auto& cb : changeCallbacks_) {
        try {
            cb(oldConfig, config);
        } catch (const std::exception& e) {
            spdlog::error("ConfigUtils: Change callback error: {}", e.what());
        }
    }
}

void ConfigUtils::registerChangeCallback(ConfigChangeCallback callback) {
    std::unique_lock<std::shared_mutex> lock(configMutex_);
    changeCallbacks_.push_back(std::move(callback));
}

std::string ConfigUtils::getFilePath() const {
    std::shared_lock<std::shared_mutex> lock(configMutex_);
    return filePath_;
}

}  // namespace utils
}  // namespace web
}  // namespace sophon_stream
