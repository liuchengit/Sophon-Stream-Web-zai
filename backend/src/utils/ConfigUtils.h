#pragma once

#include <string>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <nlohmann/json.hpp>

namespace sophon_stream {
namespace web {
namespace utils {

/**
 * Thread-safe configuration utility with hot-reload support.
 * Loads and manages JSON configuration with dot-notation access.
 */
class ConfigUtils {
public:
    /**
     * Get the singleton instance.
     */
    static ConfigUtils& getInstance();

    /**
     * Load configuration from a JSON file.
     * @param filePath Path to the JSON config file.
     * @return true on success, false on failure.
     */
    bool loadFromFile(const std::string& filePath);

    /**
     * Hot-reload configuration from the previously loaded file.
     * @return true on success, false on failure.
     */
    bool reload();

    /**
     * Save current configuration to the file.
     * @return true on success, false on failure.
     */
    bool saveToFile();

    /**
     * Get a value using dot notation (e.g., "app.log_level").
     * @tparam T The type to return.
     * @param key Dot-notation key.
     * @param defaultValue Default value if key not found.
     * @return The value or defaultValue.
     */
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T{}) const {
        std::shared_lock<std::shared_mutex> lock(configMutex_);
        try {
            nlohmann::json current = config_;
            auto parts = splitKey(key);
            for (const auto& part : parts) {
                if (current.is_object() && current.contains(part)) {
                    current = current[part];
                } else {
                    return defaultValue;
                }
            }
            return current.get<T>();
        } catch (const std::exception& e) {
            return defaultValue;
        }
    }

    /**
     * Set a value using dot notation.
     * @tparam T The type of the value.
     * @param key Dot-notation key.
     * @param value The value to set.
     */
    template<typename T>
    void set(const std::string& key, const T& value) {
        std::unique_lock<std::shared_mutex> lock(configMutex_);
        auto parts = splitKey(key);
        nlohmann::json* current = &config_;
        for (size_t i = 0; i < parts.size() - 1; ++i) {
            if (!current->contains(parts[i])) {
                (*current)[parts[i]] = nlohmann::json::object();
            }
            current = &(*current)[parts[i]];
        }
        (*current)[parts.back()] = value;
    }

    /**
     * Check if a key exists.
     * @param key Dot-notation key.
     * @return true if the key exists.
     */
    bool has(const std::string& key) const;

    /**
     * Get the full configuration as JSON.
     * @return Copy of the configuration.
     */
    nlohmann::json getConfig() const;

    /**
     * Set the full configuration from JSON.
     * @param config The configuration to set.
     */
    void setConfig(const nlohmann::json& config);

    /**
     * Register a callback for configuration changes.
     * @param callback Function called when config is reloaded.
     */
    using ConfigChangeCallback = std::function<void(const nlohmann::json& oldConfig,
                                                     const nlohmann::json& newConfig)>;
    void registerChangeCallback(ConfigChangeCallback callback);

    /**
     * Get the loaded file path.
     * @return The file path or empty string if not loaded.
     */
    std::string getFilePath() const;

private:
    ConfigUtils() = default;
    ~ConfigUtils() = default;
    ConfigUtils(const ConfigUtils&) = delete;
    ConfigUtils& operator=(const ConfigUtils&) = delete;

    /**
     * Split a dot-notation key into parts.
     */
    static std::vector<std::string> splitKey(const std::string& key);

    mutable std::shared_mutex configMutex_;
    nlohmann::json config_;
    std::string filePath_;
    std::vector<ConfigChangeCallback> changeCallbacks_;
};

}  // namespace utils
}  // namespace web
}  // namespace sophon_stream
