#include "services/PluginService.h"
#include "database/DbManager.h"
#include "utils/CryptoUtils.h"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <dlfcn.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>

namespace sophon_stream {
namespace web {
namespace services {

PluginService& PluginService::getInstance() {
    static PluginService instance;
    return instance;
}

PluginService::~PluginService() {
    // Unload all dynamic libraries
    for (auto& [id, handle] : loadedHandles_) {
        if (handle) {
            dlclose(handle);
            spdlog::info("PluginService::~PluginService - unloaded library for plugin id={}", id);
        }
    }
    loadedHandles_.clear();
}

std::optional<models::Plugin> PluginService::installPlugin(const std::string& name,
                                                           const std::string& soPath,
                                                           const std::string& config) {
    auto& db = database::DbManager::getInstance();

    if (name.empty() || soPath.empty()) {
        spdlog::warn("PluginService::installPlugin - name or soPath is empty");
        return std::nullopt;
    }

    // Check for duplicate name
    if (db.exists("plugins", "name = ?", {name})) {
        spdlog::warn("PluginService::installPlugin - name already exists: {}", name);
        return std::nullopt;
    }

    // Validate the .so file
    auto validation = validatePlugin(soPath);
    if (!validation.value("valid", false)) {
        spdlog::warn("PluginService::installPlugin - invalid plugin: {}", soPath);
        return std::nullopt;
    }

    // Parse config for version, author, description, type, config_schema
    std::string version = "1.0.0";
    std::string author;
    std::string description;
    std::string type = "element";
    std::string configSchema;

    if (!config.empty()) {
        try {
            auto j = nlohmann::json::parse(config);
            if (j.contains("version")) version = j["version"].get<std::string>();
            if (j.contains("author")) author = j["author"].get<std::string>();
            if (j.contains("description")) description = j["description"].get<std::string>();
            if (j.contains("type")) type = j["type"].get<std::string>();
            if (j.contains("configSchema"))
                configSchema = j["configSchema"].is_string()
                    ? j["configSchema"].get<std::string>() : j["configSchema"].dump();
        } catch (...) {}
    }

    auto qr = db.insert("plugins", {
        {"name",          name},
        {"version",       version},
        {"so_path",       soPath},
        {"config_schema", configSchema},
        {"description",   description},
        {"author",        author},
        {"type",          type},
        {"status",        "0"},
        {"checksum",      validation.value("checksum", "")}
    });

    if (!qr.success) {
        spdlog::error("PluginService::installPlugin - DB error: {}", qr.errorMessage);
        return std::nullopt;
    }

    db.insert("audit_logs", {
        {"action", "install"},
        {"resource_type", "plugin"},
        {"resource_id", std::to_string(qr.lastInsertId)},
        {"detail", "Installed plugin: " + name}
    });

    auto result = db.findById("plugins", qr.lastInsertId);
    if (!result.success || result.rows.empty()) return std::nullopt;
    return models::Plugin::fromDbRow(result.rows[0]);
}

bool PluginService::uninstallPlugin(int id) {
    auto& db = database::DbManager::getInstance();

    auto qr = db.findById("plugins", id);
    if (!qr.success || qr.rows.empty()) return false;

    // Deactivate first if active
    auto it = loadedHandles_.find(id);
    if (it != loadedHandles_.end()) {
        dlclose(it->second);
        loadedHandles_.erase(it);
    }

    auto delResult = db.remove("plugins", "id = ?", {std::to_string(id)});
    if (!delResult.success) {
        spdlog::error("PluginService::uninstallPlugin - DB error: {}", delResult.errorMessage);
        return false;
    }

    db.insert("audit_logs", {
        {"action", "uninstall"},
        {"resource_type", "plugin"},
        {"resource_id", std::to_string(id)},
        {"detail", "Uninstalled plugin"}
    });

    spdlog::info("PluginService::uninstallPlugin - plugin id={} uninstalled", id);
    return true;
}

bool PluginService::activatePlugin(int id) {
    auto& db = database::DbManager::getInstance();
    auto qr = db.findById("plugins", id);
    if (!qr.success || qr.rows.empty()) return false;

    auto plugin = models::Plugin::fromDbRow(qr.rows[0]);

    // Load the dynamic library
    void* handle = loadDynamicLibrary(plugin.soPath);
    if (!handle) return false;

    loadedHandles_[id] = handle;

    db.update("plugins", {{"status", "1"}}, "id = ?", {std::to_string(id)});

    db.insert("audit_logs", {
        {"action", "activate"},
        {"resource_type", "plugin"},
        {"resource_id", std::to_string(id)},
        {"detail", "Activated plugin: " + plugin.name}
    });

    spdlog::info("PluginService::activatePlugin - plugin id={} activated", id);
    return true;
}

bool PluginService::deactivatePlugin(int id) {
    auto& db = database::DbManager::getInstance();
    if (!db.exists("plugins", "id = ?", {std::to_string(id)})) return false;

    // Unload the library
    auto it = loadedHandles_.find(id);
    if (it != loadedHandles_.end()) {
        unloadDynamicLibrary(it->second);
        loadedHandles_.erase(it);
    }

    db.update("plugins", {{"status", "0"}}, "id = ?", {std::to_string(id)});

    db.insert("audit_logs", {
        {"action", "deactivate"},
        {"resource_type", "plugin"},
        {"resource_id", std::to_string(id)},
        {"detail", "Deactivated plugin"}
    });

    spdlog::info("PluginService::deactivatePlugin - plugin id={} deactivated", id);
    return true;
}

nlohmann::json PluginService::listPlugins(int page, int pageSize, const std::string& type) {
    auto& db = database::DbManager::getInstance();

    std::string whereClause;
    std::vector<std::string> params;
    if (!type.empty()) {
        whereClause = "type = ?";
        params.push_back(type);
    }

    int64_t total = db.count("plugins", whereClause, params);
    int offset = (page - 1) * pageSize;

    auto qr = db.find("plugins", whereClause.empty() ? "" : whereClause, params,
                       "installed_at DESC", pageSize, offset);

    nlohmann::json items = nlohmann::json::array();
    if (qr.success) {
        for (const auto& row : qr.rows) {
            items.push_back(models::Plugin::fromDbRow(row).toJson());
        }
    }

    return {{"items", items}, {"total", total}, {"page", page}, {"pageSize", pageSize}};
}

std::optional<models::Plugin> PluginService::getPlugin(int id) {
    auto& db = database::DbManager::getInstance();
    auto qr = db.findById("plugins", id);
    if (!qr.success || qr.rows.empty()) return std::nullopt;
    return models::Plugin::fromDbRow(qr.rows[0]);
}

nlohmann::json PluginService::validatePlugin(const std::string& soPath) {
    nlohmann::json result;
    result["valid"] = false;
    result["info"] = nlohmann::json::object();
    result["checksum"] = "";

    namespace fs = std::filesystem;

    if (soPath.empty()) {
        result["error"] = "Plugin path is empty";
        return result;
    }

    fs::path p(soPath);
    if (!fs::exists(p)) {
        result["error"] = "Plugin file does not exist: " + soPath;
        return result;
    }

    // Compute SHA-256 checksum
    std::ifstream file(soPath, std::ios::binary);
    if (!file.is_open()) {
        result["error"] = "Cannot open plugin file";
        return result;
    }

    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(file), {});
    result["checksum"] = utils::CryptoUtils::toHex(buffer.data(), buffer.size());

    // Compute SHA-256 checksum using EVP
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen = 0;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (ctx && EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1 &&
        EVP_DigestUpdate(ctx, buffer.data(), buffer.size()) == 1 &&
        EVP_DigestFinal_ex(ctx, hash, &hashLen) == 1) {
        result["checksum"] = utils::CryptoUtils::toHex(hash, hashLen);
    } else {
        spdlog::warn("PluginService::validatePlugin - SHA-256 computation failed");
    }
    if (ctx) EVP_MD_CTX_free(ctx);

    result["info"]["path"] = soPath;
    result["info"]["size"] = buffer.size();
    result["info"]["extension"] = p.extension().string();

    // Try to dlopen the library to validate it
    void* handle = dlopen(soPath.c_str(), RTLD_LAZY);
    if (!handle) {
        result["error"] = "dlopen failed: " + std::string(dlerror());
        return result;
    }

    // Look for required symbol (e.g., plugin entry point)
    void* entryPoint = dlsym(handle, "sophon_stream_plugin_init");
    if (!entryPoint) {
        result["error"] = "Missing required symbol: sophon_stream_plugin_init";
        dlclose(handle);
        return result;
    }

    dlclose(handle);
    result["valid"] = true;

    return result;
}

void* PluginService::loadDynamicLibrary(const std::string& soPath) {
    void* handle = dlopen(soPath.c_str(), RTLD_NOW);
    if (!handle) {
        spdlog::error("PluginService::loadDynamicLibrary - dlopen failed: {}", dlerror());
        return nullptr;
    }
    spdlog::info("PluginService::loadDynamicLibrary - loaded {}", soPath);
    return handle;
}

void PluginService::unloadDynamicLibrary(void* handle) {
    if (handle) {
        dlclose(handle);
        spdlog::info("PluginService::unloadDynamicLibrary - library unloaded");
    }
}

}  // namespace services
}  // namespace web
}  // namespace sophon_stream
