#pragma once

#include <string>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>
#include "models/Algorithm.h"

namespace sophon_stream {
namespace web {
namespace services {

/**
 * Algorithm management service.
 * Handles CRUD, model validation, and type listing.
 */
class AlgorithmService {
public:
    static AlgorithmService& getInstance();

    std::optional<models::Algorithm> createAlgorithm(const models::Algorithm& algo);
    std::optional<models::Algorithm> updateAlgorithm(int id, const nlohmann::json& updates);
    bool deleteAlgorithm(int id);
    std::optional<models::Algorithm> getAlgorithm(int id);
    nlohmann::json listAlgorithms(int page, int pageSize, const std::string& type = "");

    /**
     * Validate a model file at the given path.
     * @return JSON with valid (bool) and modelInfo.
     */
    nlohmann::json validateModel(const std::string& modelPath);

    /**
     * Get all supported algorithm types.
     */
    static std::vector<std::string> getAlgorithmTypes();

private:
    AlgorithmService() = default;
    ~AlgorithmService() = default;
    AlgorithmService(const AlgorithmService&) = delete;
    AlgorithmService& operator=(const AlgorithmService&) = delete;
};

}  // namespace services
}  // namespace web
}  // namespace sophon_stream
