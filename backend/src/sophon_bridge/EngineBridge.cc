#include "sophon_bridge/EngineBridge.h"
#include "common/Version.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <algorithm>
#include <cmath>

namespace sophon_stream {
namespace web {
namespace sophon_bridge {

EngineBridge& EngineBridge::getInstance() {
    static EngineBridge instance;
    return instance;
}

EngineBridge::~EngineBridge() {
    shutdown();
}

bool EngineBridge::init(const nlohmann::json& config) {
    std::unique_lock<std::shared_mutex> lock(engineMutex_);

    if (initialized_.load()) {
        spdlog::warn("EngineBridge: Already initialized");
        return true;
    }

    engineConfig_ = config;

    // Check if the sophon-stream SDK is available
#ifdef HAS_SOPHON_STREAM_SDK
    sdkAvailable_ = true;
    spdlog::info("EngineBridge: Sophon-Stream SDK detected");

    // In production with SDK, we would initialize the actual Engine here:
    // auto engine = sophon_stream::framework::Singleton::getInstance();
    // engine->init(config);
#else
    sdkAvailable_ = false;
    spdlog::info("EngineBridge: Running in simulation mode (no SDK)");
#endif

    initialized_.store(true);
    spdlog::info("EngineBridge: Initialized successfully");

    return true;
}

void EngineBridge::shutdown() {
    std::unique_lock<std::shared_mutex> lock(engineMutex_);

    if (!initialized_.load()) {
        return;
    }

    // Stop and destroy all graphs
    for (auto& [graphId, state] : graphs_) {
        if (state.status == GraphStatus::RUNNING ||
            state.status == GraphStatus::PAUSED) {
            spdlog::info("EngineBridge: Stopping graph {} during shutdown", graphId);
            state.status = GraphStatus::STOPPED;
        }
    }

#ifdef HAS_SOPHON_STREAM_SDK
    // In production with SDK:
    // auto engine = sophon_stream::framework::Singleton::getInstance();
    // for (auto& [graphId, state] : graphs_) {
    //     engine->stop(graphId);
    //     engine->destoryGraph(graphId);
    // }
#endif

    graphs_.clear();
    initialized_.store(false);

    spdlog::info("EngineBridge: Shutdown complete");
}

bool EngineBridge::isInitialized() const {
    return initialized_.load();
}

bool EngineBridge::validateGraphConfig(const nlohmann::json& config) const {
    // A valid graph config must have either elements or a DAG configuration
    if (!config.is_object()) {
        return false;
    }

    // Must have a name or graph structure
    if (!config.contains("name") && !config.contains("elements") && !config.contains("dag")) {
        spdlog::warn("EngineBridge: Graph config must contain 'name', 'elements', or 'dag'");
        return false;
    }

    return true;
}

int64_t EngineBridge::createGraph(const nlohmann::json& config) {
    std::unique_lock<std::shared_mutex> lock(engineMutex_);

    if (!initialized_.load()) {
        spdlog::error("EngineBridge: Cannot create graph - not initialized");
        return -1;
    }

    if (!validateGraphConfig(config)) {
        spdlog::error("EngineBridge: Invalid graph configuration");
        return -1;
    }

    int64_t graphId = nextGraphId_++;

    GraphState state;
    state.config.graphId = graphId;
    state.config.name = config.value("name", "graph_" + std::to_string(graphId));
    state.config.dagConfig = config.value("dag", nlohmann::json::object());
    state.status = GraphStatus::STOPPED;
    state.metrics.graphId = graphId;
    state.metrics.status = GraphStatus::STOPPED;
    state.startTime = std::chrono::system_clock::now();
    state.frameCounter = 0;

    // Parse element configurations
    if (config.contains("elements") && config["elements"].is_array()) {
        for (const auto& elemJson : config["elements"]) {
            ElementConfig elem;
            elem.elementId = elemJson.value("id", 0);
            elem.elementName = elemJson.value("name", "");
            elem.elementType = elemJson.value("type", "");
            elem.config = elemJson.value("config", nlohmann::json::object());
            state.config.elements.push_back(std::move(elem));
        }
    }

#ifdef HAS_SOPHON_STREAM_SDK
    // In production with SDK:
    // auto engine = sophon_stream::framework::Singleton::getInstance();
    // auto jsonStr = config.dump();
    // bool ok = engine->createGraph(jsonStr);
    // if (!ok) {
    //     spdlog::error("EngineBridge: SDK failed to create graph");
    //     return -1;
    // }
    // int64_t sdkGraphId = engine->getGraphIdByName(config["name"].get<std::string>());
    // graphId = sdkGraphId;
    spdlog::info("EngineBridge: Graph {} created via SDK", graphId);
#endif

    graphs_[graphId] = std::move(state);

    spdlog::info("EngineBridge: Created graph {} ('{}') with {} elements",
                 graphId, config.value("name", "unnamed"),
                 graphs_[graphId].config.elements.size());

    return graphId;
}

bool EngineBridge::destroyGraph(int64_t graphId) {
    std::unique_lock<std::shared_mutex> lock(engineMutex_);

    if (!initialized_.load()) {
        spdlog::error("EngineBridge: Cannot destroy graph - not initialized");
        return false;
    }

    auto it = graphs_.find(graphId);
    if (it == graphs_.end()) {
        spdlog::warn("EngineBridge: Graph {} not found", graphId);
        return false;
    }

    bool wasActive = it->second.status == GraphStatus::RUNNING ||
                     it->second.status == GraphStatus::PAUSED;

    // Stop the graph first if running
    if (wasActive) {
#ifdef HAS_SOPHON_STREAM_SDK
        // auto engine = sophon_stream::framework::Singleton::getInstance();
        // engine->stop(graphId);
        // engine->destoryGraph(graphId);
#endif
        it->second.status = GraphStatus::STOPPED;
    }

    graphs_.erase(it);
    spdlog::info("EngineBridge: Destroyed graph {}", graphId);

    // Notify callbacks outside the lock to prevent deadlock
    if (wasActive) {
        auto callbacks = statusCallbacks_;
        lock.unlock();

        for (const auto& cb : callbacks) {
            try {
                cb(graphId, GraphStatus::STOPPED);
            } catch (const std::exception& e) {
                spdlog::error("EngineBridge: Status callback error: {}", e.what());
            }
        }
    }

    return true;
}

bool EngineBridge::startGraph(int64_t graphId) {
    std::unique_lock<std::shared_mutex> lock(engineMutex_);

    if (!initialized_.load()) {
        spdlog::error("EngineBridge: Cannot start graph - not initialized");
        return false;
    }

    auto it = graphs_.find(graphId);
    if (it == graphs_.end()) {
        spdlog::warn("EngineBridge: Graph {} not found", graphId);
        return false;
    }

    if (it->second.status == GraphStatus::RUNNING) {
        spdlog::warn("EngineBridge: Graph {} is already running", graphId);
        return true;
    }

#ifdef HAS_SOPHON_STREAM_SDK
    // auto engine = sophon_stream::framework::Singleton::getInstance();
    // engine->start(graphId);
#endif

    it->second.status = GraphStatus::RUNNING;
    it->second.metrics.status = GraphStatus::RUNNING;
    it->second.startTime = std::chrono::system_clock::now();
    it->second.frameCounter = 0;

    spdlog::info("EngineBridge: Started graph {}", graphId);

    // Notify callbacks (outside the lock)
    auto callbacks = statusCallbacks_;
    GraphStatus status = GraphStatus::RUNNING;
    lock.unlock();

    for (const auto& cb : callbacks) {
        try {
            cb(graphId, status);
        } catch (const std::exception& e) {
            spdlog::error("EngineBridge: Status callback error: {}", e.what());
        }
    }

    return true;
}

bool EngineBridge::stopGraph(int64_t graphId) {
    std::unique_lock<std::shared_mutex> lock(engineMutex_);

    if (!initialized_.load()) {
        spdlog::error("EngineBridge: Cannot stop graph - not initialized");
        return false;
    }

    auto it = graphs_.find(graphId);
    if (it == graphs_.end()) {
        spdlog::warn("EngineBridge: Graph {} not found", graphId);
        return false;
    }

    if (it->second.status == GraphStatus::STOPPED) {
        spdlog::warn("EngineBridge: Graph {} is already stopped", graphId);
        return true;
    }

#ifdef HAS_SOPHON_STREAM_SDK
    // auto engine = sophon_stream::framework::Singleton::getInstance();
    // engine->stop(graphId);
#endif

    it->second.status = GraphStatus::STOPPED;
    it->second.metrics.status = GraphStatus::STOPPED;
    it->second.metrics.fps = 0.0;
    it->second.metrics.throughput = 0;

    spdlog::info("EngineBridge: Stopped graph {}", graphId);

    auto callbacks = statusCallbacks_;
    GraphStatus status = GraphStatus::STOPPED;
    lock.unlock();

    for (const auto& cb : callbacks) {
        try {
            cb(graphId, status);
        } catch (const std::exception& e) {
            spdlog::error("EngineBridge: Status callback error: {}", e.what());
        }
    }

    return true;
}

bool EngineBridge::pauseGraph(int64_t graphId) {
    std::unique_lock<std::shared_mutex> lock(engineMutex_);

    if (!initialized_.load()) {
        spdlog::error("EngineBridge: Cannot pause graph - not initialized");
        return false;
    }

    auto it = graphs_.find(graphId);
    if (it == graphs_.end()) {
        spdlog::warn("EngineBridge: Graph {} not found", graphId);
        return false;
    }

    if (it->second.status != GraphStatus::RUNNING) {
        spdlog::warn("EngineBridge: Graph {} is not running (status: {})",
                     graphId, static_cast<int>(it->second.status));
        return false;
    }

#ifdef HAS_SOPHON_STREAM_SDK
    // Pause is typically achieved by pausing the source element
    // auto engine = sophon_stream::framework::Singleton::getInstance();
    // engine->pause(graphId);
#endif

    it->second.status = GraphStatus::PAUSED;
    it->second.metrics.status = GraphStatus::PAUSED;

    spdlog::info("EngineBridge: Paused graph {}", graphId);

    auto callbacks = statusCallbacks_;
    GraphStatus status = GraphStatus::PAUSED;
    lock.unlock();

    for (const auto& cb : callbacks) {
        try {
            cb(graphId, status);
        } catch (const std::exception& e) {
            spdlog::error("EngineBridge: Status callback error: {}", e.what());
        }
    }

    return true;
}

bool EngineBridge::resumeGraph(int64_t graphId) {
    std::unique_lock<std::shared_mutex> lock(engineMutex_);

    if (!initialized_.load()) {
        spdlog::error("EngineBridge: Cannot resume graph - not initialized");
        return false;
    }

    auto it = graphs_.find(graphId);
    if (it == graphs_.end()) {
        spdlog::warn("EngineBridge: Graph {} not found", graphId);
        return false;
    }

    if (it->second.status != GraphStatus::PAUSED) {
        spdlog::warn("EngineBridge: Graph {} is not paused (status: {})",
                     graphId, static_cast<int>(it->second.status));
        return false;
    }

#ifdef HAS_SOPHON_STREAM_SDK
    // auto engine = sophon_stream::framework::Singleton::getInstance();
    // engine->resume(graphId);
#endif

    it->second.status = GraphStatus::RUNNING;
    it->second.metrics.status = GraphStatus::RUNNING;

    spdlog::info("EngineBridge: Resumed graph {}", graphId);

    auto callbacks = statusCallbacks_;
    GraphStatus status = GraphStatus::RUNNING;
    lock.unlock();

    for (const auto& cb : callbacks) {
        try {
            cb(graphId, status);
        } catch (const std::exception& e) {
            spdlog::error("EngineBridge: Status callback error: {}", e.what());
        }
    }

    return true;
}

bool EngineBridge::updateElementConfig(int64_t graphId, int64_t elementId,
                                        const nlohmann::json& config) {
    std::unique_lock<std::shared_mutex> lock(engineMutex_);

    if (!initialized_.load()) {
        spdlog::error("EngineBridge: Cannot update element - not initialized");
        return false;
    }

    auto it = graphs_.find(graphId);
    if (it == graphs_.end()) {
        spdlog::warn("EngineBridge: Graph {} not found", graphId);
        return false;
    }

    // Find and update the element configuration
    bool found = false;
    for (auto& elem : it->second.config.elements) {
        if (elem.elementId == elementId) {
            // Merge the new config into the existing one
            elem.config.merge_patch(config);
            found = true;
            break;
        }
    }

    if (!found) {
        spdlog::warn("EngineBridge: Element {} not found in graph {}", elementId, graphId);
        return false;
    }

#ifdef HAS_SOPHON_STREAM_SDK
    // In production with SDK:
    // auto engine = sophon_stream::framework::Singleton::getInstance();
    // auto jsonStr = config.dump();
    // engine->setElementConfig(graphId, elementId, jsonStr);
#endif

    spdlog::info("EngineBridge: Updated element {} in graph {}", elementId, graphId);
    return true;
}

EngineStatus EngineBridge::getEngineStatus() const {
    std::shared_lock<std::shared_mutex> lock(engineMutex_);

    EngineStatus status;
    status.initialized = initialized_.load();
    status.version = PROJECT_VERSION;

    if (!initialized_.load()) {
        return status;
    }

    status.totalGraphs = static_cast<int>(graphs_.size());
    for (const auto& [id, state] : graphs_) {
        if (state.status == GraphStatus::RUNNING) {
            status.runningGraphs++;
        }
    }

    return status;
}

GraphStatus EngineBridge::getGraphStatus(int64_t graphId) const {
    std::shared_lock<std::shared_mutex> lock(engineMutex_);

    auto it = graphs_.find(graphId);
    if (it == graphs_.end()) {
        return GraphStatus::ERROR;
    }

    return it->second.status;
}

GraphMetrics EngineBridge::getGraphMetrics(int64_t graphId) const {
    std::shared_lock<std::shared_mutex> lock(engineMutex_);

    auto it = graphs_.find(graphId);
    if (it == graphs_.end()) {
        GraphMetrics metrics;
        metrics.graphId = graphId;
        metrics.status = GraphStatus::ERROR;
        metrics.errorMessage = "Graph not found";
        return metrics;
    }

    // Update metrics for simulation mode
    const auto& state = it->second;
    GraphMetrics metrics = state.metrics;

    if (state.status == GraphStatus::RUNNING) {
        // Simulate realistic metrics
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - state.startTime).count();

        if (elapsed > 0) {
            // Simulate FPS with slight variation
            double baseFps = 25.0;
            double variation = std::sin(static_cast<double>(elapsed) * 0.1) * 2.0;
            metrics.fps = std::max(0.0, baseFps + variation);
            metrics.throughput = static_cast<int64_t>(metrics.fps * elapsed);
            metrics.totalFrames = state.frameCounter + metrics.throughput;

            // Simulate latency (1-3 frames worth)
            metrics.latencyMs = 1000.0 * (1.0 + std::fmod(static_cast<double>(elapsed), 2.0)) / metrics.fps;

            // Minimal dropped frames
            metrics.droppedFrames = static_cast<int64_t>(elapsed * 0.01);
        }
    }

#ifdef HAS_SOPHON_STREAM_SDK
    // In production with SDK, get actual metrics from the engine
    // auto engine = sophon_stream::framework::Singleton::getInstance();
    // auto metricsJson = engine->getGraphMetrics(graphId);
    // Parse actual metrics from the SDK response
#endif

    return metrics;
}

std::vector<int64_t> EngineBridge::getGraphIds() const {
    std::shared_lock<std::shared_mutex> lock(engineMutex_);

    std::vector<int64_t> ids;
    ids.reserve(graphs_.size());
    for (const auto& [id, state] : graphs_) {
        ids.push_back(id);
    }
    return ids;
}

nlohmann::json EngineBridge::getGraphConfig(int64_t graphId) const {
    std::shared_lock<std::shared_mutex> lock(engineMutex_);

    auto it = graphs_.find(graphId);
    if (it == graphs_.end()) {
        return nlohmann::json();
    }

    nlohmann::json config;
    config["graph_id"] = it->second.config.graphId;
    config["name"] = it->second.config.name;
    config["dag"] = it->second.config.dagConfig;

    nlohmann::json elements = nlohmann::json::array();
    for (const auto& elem : it->second.config.elements) {
        nlohmann::json elemJson;
        elemJson["id"] = elem.elementId;
        elemJson["name"] = elem.elementName;
        elemJson["type"] = elem.elementType;
        elemJson["config"] = elem.config;
        elements.push_back(elemJson);
    }
    config["elements"] = elements;

    return config;
}

void EngineBridge::registerStatusCallback(GraphStatusCallback callback) {
    std::unique_lock<std::shared_mutex> lock(engineMutex_);
    statusCallbacks_.push_back(std::move(callback));
}

void EngineBridge::registerDataCallback(ElementDataCallback callback) {
    std::unique_lock<std::shared_mutex> lock(engineMutex_);
    dataCallbacks_.push_back(std::move(callback));
}

bool EngineBridge::setSourceUrl(int64_t graphId, const std::string& url) {
    std::unique_lock<std::shared_mutex> lock(engineMutex_);

    auto it = graphs_.find(graphId);
    if (it == graphs_.end()) {
        spdlog::warn("EngineBridge: Graph {} not found", graphId);
        return false;
    }

    it->second.sourceConfig["url"] = url;

#ifdef HAS_SOPHON_STREAM_SDK
    // Update the source element's URL configuration
    // auto engine = sophon_stream::framework::Singleton::getInstance();
    // Find the source element and update its URL
    // for (const auto& elem : it->second.config.elements) {
    //     if (elem.elementType == "source" || elem.elementType == "decode") {
    //         nlohmann::json updateConfig;
    //         updateConfig["url"] = url;
    //         engine->setElementConfig(graphId, elem.elementId, updateConfig.dump());
    //         break;
    //     }
    // }
#endif

    spdlog::info("EngineBridge: Set source URL for graph {}: {}", graphId, url);
    return true;
}

bool EngineBridge::setRoiConfig(int64_t graphId, const nlohmann::json& roiConfig) {
    std::unique_lock<std::shared_mutex> lock(engineMutex_);

    auto it = graphs_.find(graphId);
    if (it == graphs_.end()) {
        spdlog::warn("EngineBridge: Graph {} not found", graphId);
        return false;
    }

    it->second.roiConfig = roiConfig;

#ifdef HAS_SOPHON_STREAM_SDK
    // Update ROI configuration for detection/tracking elements
    // auto engine = sophon_stream::framework::Singleton::getInstance();
    // for (const auto& elem : it->second.config.elements) {
    //     if (elem.elementType == "detect" || elem.elementType == "track") {
    //         nlohmann::json updateConfig;
    //         updateConfig["roi"] = roiConfig;
    //         engine->setElementConfig(graphId, elem.elementId, updateConfig.dump());
    //     }
    // }
#endif

    spdlog::info("EngineBridge: Set ROI config for graph {}", graphId);
    return true;
}

void EngineBridge::notifyStatusChange(int64_t graphId, GraphStatus status) {
    // Note: Caller must NOT hold engineMutex_ when calling this.
    // For methods that call this, unlock the mutex first.
    for (const auto& cb : statusCallbacks_) {
        try {
            cb(graphId, status);
        } catch (const std::exception& e) {
            spdlog::error("EngineBridge: Status callback error: {}", e.what());
        }
    }
}

void EngineBridge::updateGraphMetrics(int64_t graphId) {
    // Note: Caller must hold engineMutex_
    auto it = graphs_.find(graphId);
    if (it == graphs_.end()) {
        return;
    }

    if (it->second.status == GraphStatus::RUNNING) {
        it->second.frameCounter++;
    }
}

}  // namespace sophon_bridge
}  // namespace web
}  // namespace sophon_stream
