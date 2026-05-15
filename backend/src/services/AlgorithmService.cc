#include "services/AlgorithmService.h"
#include "database/DbManager.h"
#include <spdlog/spdlog.h>
#include <filesystem>

namespace sophon_stream {
namespace web {
namespace services {

AlgorithmService& AlgorithmService::getInstance() {
    static AlgorithmService instance;
    return instance;
}

std::optional<models::Algorithm> AlgorithmService::createAlgorithm(const models::Algorithm& algo) {
    auto& db = database::DbManager::getInstance();

    if (algo.name.empty() || algo.modelPath.empty()) {
        spdlog::warn("AlgorithmService::createAlgorithm - name or modelPath is empty");
        return std::nullopt;
    }

    // Check for duplicate name
    if (db.exists("algorithms", "name = ?", {algo.name})) {
        spdlog::warn("AlgorithmService::createAlgorithm - name already exists: {}", algo.name);
        return std::nullopt;
    }

    auto qr = db.insert("algorithms", {
        {"name",        algo.name},
        {"type",        algo.type.empty() ? "detection" : algo.type},
        {"model_path",  algo.modelPath},
        {"config_json", algo.configJson},
        {"version",     algo.version},
        {"description", algo.description},
        {"input_size",  algo.inputSize},
        {"classes",     algo.classes},
        {"status",      "1"}
    });

    if (!qr.success) {
        spdlog::error("AlgorithmService::createAlgorithm - DB error: {}", qr.errorMessage);
        return std::nullopt;
    }

    db.insert("audit_logs", {
        {"action", "create"},
        {"resource_type", "algorithm"},
        {"resource_id", std::to_string(qr.lastInsertId)},
        {"detail", "Created algorithm: " + algo.name}
    });

    auto result = db.findById("algorithms", qr.lastInsertId);
    if (!result.success || result.rows.empty()) return std::nullopt;
    return models::Algorithm::fromDbRow(result.rows[0]);
}

std::optional<models::Algorithm> AlgorithmService::updateAlgorithm(int id, const nlohmann::json& updates) {
    auto& db = database::DbManager::getInstance();
    if (!db.exists("algorithms", "id = ?", {std::to_string(id)})) return std::nullopt;

    std::unordered_map<std::string, std::string> data;
    if (updates.contains("name") && !updates["name"].is_null())
        data["name"] = updates["name"].get<std::string>();
    if (updates.contains("type") && !updates["type"].is_null())
        data["type"] = updates["type"].get<std::string>();
    if (updates.contains("modelPath") && !updates["modelPath"].is_null())
        data["model_path"] = updates["modelPath"].get<std::string>();
    if (updates.contains("configJson") && !updates["configJson"].is_null())
        data["config_json"] = updates["configJson"].is_string()
            ? updates["configJson"].get<std::string>() : updates["configJson"].dump();
    if (updates.contains("version") && !updates["version"].is_null())
        data["version"] = updates["version"].get<std::string>();
    if (updates.contains("description") && !updates["description"].is_null())
        data["description"] = updates["description"].get<std::string>();
    if (updates.contains("inputSize") && !updates["inputSize"].is_null())
        data["input_size"] = updates["inputSize"].get<std::string>();
    if (updates.contains("classes") && !updates["classes"].is_null())
        data["classes"] = updates["classes"].is_string()
            ? updates["classes"].get<std::string>() : updates["classes"].dump();
    if (updates.contains("status") && !updates["status"].is_null())
        data["status"] = std::to_string(updates["status"].get<int>());

    if (data.empty()) {
        auto result = db.findById("algorithms", id);
        if (!result.success || result.rows.empty()) return std::nullopt;
        return models::Algorithm::fromDbRow(result.rows[0]);
    }

    auto qr = db.update("algorithms", data, "id = ?", {std::to_string(id)});
    if (!qr.success) {
        spdlog::error("AlgorithmService::updateAlgorithm - DB error: {}", qr.errorMessage);
        return std::nullopt;
    }

    db.insert("audit_logs", {
        {"action", "update"},
        {"resource_type", "algorithm"},
        {"resource_id", std::to_string(id)},
        {"detail", "Updated algorithm"}
    });

    auto result = db.findById("algorithms", id);
    if (!result.success || result.rows.empty()) return std::nullopt;
    return models::Algorithm::fromDbRow(result.rows[0]);
}

bool AlgorithmService::deleteAlgorithm(int id) {
    auto& db = database::DbManager::getInstance();
    if (!db.exists("algorithms", "id = ?", {std::to_string(id)})) return false;

    auto qr = db.remove("algorithms", "id = ?", {std::to_string(id)});
    if (!qr.success) {
        spdlog::error("AlgorithmService::deleteAlgorithm - DB error: {}", qr.errorMessage);
        return false;
    }

    db.insert("audit_logs", {
        {"action", "delete"},
        {"resource_type", "algorithm"},
        {"resource_id", std::to_string(id)},
        {"detail", "Deleted algorithm"}
    });

    spdlog::info("AlgorithmService::deleteAlgorithm - algorithm id={} deleted", id);
    return true;
}

std::optional<models::Algorithm> AlgorithmService::getAlgorithm(int id) {
    auto& db = database::DbManager::getInstance();
    auto qr = db.findById("algorithms", id);
    if (!qr.success || qr.rows.empty()) return std::nullopt;
    return models::Algorithm::fromDbRow(qr.rows[0]);
}

nlohmann::json AlgorithmService::listAlgorithms(int page, int pageSize, const std::string& type) {
    auto& db = database::DbManager::getInstance();

    std::string whereClause;
    std::vector<std::string> params;
    if (!type.empty()) {
        whereClause = "type = ?";
        params.push_back(type);
    }

    int64_t total = db.count("algorithms", whereClause, params);
    int offset = (page - 1) * pageSize;

    auto qr = db.find("algorithms", whereClause.empty() ? "" : whereClause, params,
                       "created_at DESC", pageSize, offset);

    nlohmann::json items = nlohmann::json::array();
    if (qr.success) {
        for (const auto& row : qr.rows) {
            items.push_back(models::Algorithm::fromDbRow(row).toJson());
        }
    }

    return {{"items", items}, {"total", total}, {"page", page}, {"pageSize", pageSize}};
}

nlohmann::json AlgorithmService::validateModel(const std::string& modelPath) {
    nlohmann::json result;
    result["valid"] = false;
    result["modelInfo"] = nlohmann::json::object();

    namespace fs = std::filesystem;

    if (modelPath.empty()) {
        result["error"] = "Model path is empty";
        return result;
    }

    fs::path p(modelPath);
    if (!fs::exists(p)) {
        result["error"] = "Model file does not exist: " + modelPath;
        return result;
    }

    auto fileSize = fs::file_size(p);
    result["modelInfo"]["path"] = modelPath;
    result["modelInfo"]["size"] = fileSize;
    result["modelInfo"]["extension"] = p.extension().string();

    // Check for known model file extensions
    std::string ext = p.extension().string();
    if (ext == ".bmodel" || ext == ".onnx" || ext == ".pt" ||
        ext == ".pth" || ext == ".tflite" || ext == ".engine" || ext == ".om") {
        result["valid"] = true;
        result["modelInfo"]["format"] = ext.substr(1); // Remove leading dot
    } else {
        result["error"] = "Unsupported model format: " + ext;
    }

    return result;
}

std::vector<std::string> AlgorithmService::getAlgorithmTypes() {
    return {
        "detection",
        "classification",
        "segmentation",
        "pose",
        "ocr",
        "face",
        "tracking",
        "behavior",
        "custom"
    };
}

}  // namespace services
}  // namespace web
}  // namespace sophon_stream
