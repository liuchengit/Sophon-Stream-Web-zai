#pragma once

#include <string>
#include <optional>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <nlohmann/json.hpp>
#include "models/Plugin.h"

namespace sophon_stream {
namespace web {
namespace services {

/**
 * Plugin management service.
 * Handles installation, activation, dynamic library loading, and validation.
 */
class PluginService {
public:
    static PluginService& getInstance();

    std::optional<models::Plugin> installPlugin(const std::string& name,
                                                 const std::string& soPath,
                                                 const std::string& config);
    bool uninstallPlugin(int id);
    bool activatePlugin(int id);
    bool deactivatePlugin(int id);
    nlohmann::json listPlugins(int page, int pageSize, const std::string& type = "");
    std::optional<models::Plugin> getPlugin(int id);

    /**
     * Validate a .so plugin file.
     * @return JSON with valid, info, and checksum.
     */
    nlohmann::json validatePlugin(const std::string& soPath);

    /**
     * Load a dynamic library via dlopen.
     * @return Handle on success, nullptr on failure.
     */
    void* loadDynamicLibrary(const std::string& soPath);

    /**
     * Unload a dynamic library via dlclose.
     */
    void unloadDynamicLibrary(void* handle);

private:
    PluginService() = default;
    ~PluginService();
    PluginService(const PluginService&) = delete;
    PluginService& operator=(const PluginService&) = delete;

    /// Track loaded library handles for cleanup
    std::unordered_map<int, void*> loadedHandles_;
    std::mutex handlesMutex_;
};

}  // namespace services
}  // namespace web
}  // namespace sophon_stream
