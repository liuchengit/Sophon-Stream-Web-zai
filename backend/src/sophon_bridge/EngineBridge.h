#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <atomic>
#include <optional>
#include <nlohmann/json.hpp>

namespace sophon_stream {
namespace web {
namespace sophon_bridge {

/**
 * Graph status enumeration.
 */
enum class GraphStatus {
    STOPPED = 0,
    RUNNING = 1,
    PAUSED = 2,
    ERROR = 3
};

/**
 * Engine status structure.
 */
struct EngineStatus {
    bool initialized = false;
    int runningGraphs = 0;
    int totalGraphs = 0;
    std::string version;
    std::string errorMessage;
};

/**
 * Graph metrics structure.
 */
struct GraphMetrics {
    int64_t graphId = 0;
    double fps = 0.0;
    double latencyMs = 0.0;
    int64_t throughput = 0;
    int64_t totalFrames = 0;
    int64_t droppedFrames = 0;
    GraphStatus status = GraphStatus::STOPPED;
    std::string errorMessage;
};

/**
 * Element configuration structure.
 */
struct ElementConfig {
    int64_t elementId = 0;
    std::string elementName;
    std::string elementType;
    nlohmann::json config;
};

/**
 * Graph configuration structure.
 */
struct GraphConfig {
    int64_t graphId = 0;
    std::string name;
    nlohmann::json dagConfig;       ///< Directed acyclic graph configuration
    std::vector<ElementConfig> elements;
};

/**
 * Callback type for graph status changes.
 */
using GraphStatusCallback = std::function<void(int64_t graphId, GraphStatus status)>;

/**
 * Callback type for element data output.
 */
using ElementDataCallback = std::function<void(int64_t graphId, int64_t elementId,
                                                const nlohmann::json& data)>;

/**
 * Bridge to the sophon-stream Engine.
 * Provides a C++ interface to sophon-stream's Engine/Graph/Element API.
 * Thread-safe singleton that manages the lifecycle of analysis graphs.
 *
 * When the sophon-stream SDK is available, this uses the actual SDK.
 * Otherwise, it operates in simulation mode for development and testing.
 */
class EngineBridge {
public:
    /**
     * Get the singleton instance.
     */
    static EngineBridge& getInstance();

    /**
     * Initialize the Engine with configuration.
     * @param config Engine configuration JSON.
     * @return true on success.
     */
    bool init(const nlohmann::json& config);

    /**
     * Shutdown the Engine and release all resources.
     */
    void shutdown();

    /**
     * Check if the Engine is initialized.
     */
    bool isInitialized() const;

    /**
     * Create a new graph with the given configuration.
     * @param config Graph configuration (DAG with elements).
     * @return Graph ID on success, or -1 on failure.
     */
    int64_t createGraph(const nlohmann::json& config);

    /**
     * Destroy a graph by ID.
     * @param graphId The graph ID to destroy.
     * @return true on success.
     */
    bool destroyGraph(int64_t graphId);

    /**
     * Start a graph.
     * @param graphId The graph ID to start.
     * @return true on success.
     */
    bool startGraph(int64_t graphId);

    /**
     * Stop a graph.
     * @param graphId The graph ID to stop.
     * @return true on success.
     */
    bool stopGraph(int64_t graphId);

    /**
     * Pause a graph.
     * @param graphId The graph ID to pause.
     * @return true on success.
     */
    bool pauseGraph(int64_t graphId);

    /**
     * Resume a paused graph.
     * @param graphId The graph ID to resume.
     * @return true on success.
     */
    bool resumeGraph(int64_t graphId);

    /**
     * Update an element's configuration (hot-update).
     * @param graphId The graph containing the element.
     * @param elementId The element ID to update.
     * @param config New configuration for the element.
     * @return true on success.
     */
    bool updateElementConfig(int64_t graphId, int64_t elementId, const nlohmann::json& config);

    /**
     * Get the current status of the Engine.
     * @return EngineStatus structure.
     */
    EngineStatus getEngineStatus() const;

    /**
     * Get the status of a specific graph.
     * @param graphId The graph ID.
     * @return GraphStatus, or ERROR if not found.
     */
    GraphStatus getGraphStatus(int64_t graphId) const;

    /**
     * Get metrics for a specific graph.
     * @param graphId The graph ID.
     * @return GraphMetrics structure.
     */
    GraphMetrics getGraphMetrics(int64_t graphId) const;

    /**
     * Get all graph IDs.
     * @return Vector of graph IDs.
     */
    std::vector<int64_t> getGraphIds() const;

    /**
     * Get graph configuration.
     * @param graphId The graph ID.
     * @return Graph configuration JSON, or empty if not found.
     */
    nlohmann::json getGraphConfig(int64_t graphId) const;

    /**
     * Register a callback for graph status changes.
     * @param callback The callback function.
     */
    void registerStatusCallback(GraphStatusCallback callback);

    /**
     * Register a callback for element data output.
     * @param callback The callback function.
     */
    void registerDataCallback(ElementDataCallback callback);

    /**
     * Set the source URL for a graph (e.g., RTSP stream URL).
     * @param graphId The graph ID.
     * @param url The source URL.
     * @return true on success.
     */
    bool setSourceUrl(int64_t graphId, const std::string& url);

    /**
     * Set ROI configuration for a graph.
     * @param graphId The graph ID.
     * @param roiConfig ROI configuration JSON.
     * @return true on success.
     */
    bool setRoiConfig(int64_t graphId, const nlohmann::json& roiConfig);

private:
    EngineBridge() = default;
    ~EngineBridge();
    EngineBridge(const EngineBridge&) = delete;
    EngineBridge& operator=(const EngineBridge&) = delete;

    /**
     * Internal method to notify status change callbacks.
     */
    void notifyStatusChange(int64_t graphId, GraphStatus status);

    /**
     * Internal method to validate graph configuration.
     */
    bool validateGraphConfig(const nlohmann::json& config) const;

    /**
     * Internal method to update graph metrics.
     */
    void updateGraphMetrics(int64_t graphId);

    mutable std::shared_mutex engineMutex_;
    std::atomic<bool> initialized_{false};

    /// Map of graph ID to graph configuration and state
    struct GraphState {
        GraphConfig config;
        GraphStatus status = GraphStatus::STOPPED;
        GraphMetrics metrics;
        nlohmann::json sourceConfig;
        nlohmann::json roiConfig;
        std::chrono::system_clock::time_point startTime;
        int64_t frameCounter = 0;
    };

    std::unordered_map<int64_t, GraphState> graphs_;
    int64_t nextGraphId_ = 1;

    /// Callbacks
    std::vector<GraphStatusCallback> statusCallbacks_;
    std::vector<ElementDataCallback> dataCallbacks_;

    /// Engine configuration
    nlohmann::json engineConfig_;

    /// Whether the actual SDK is available
    bool sdkAvailable_ = false;

#ifdef HAS_SOPHON_STREAM_SDK
    // When the actual SDK is available, these would hold SDK objects
    // void* engineHandle_ = nullptr;
#endif
};

}  // namespace sophon_bridge
}  // namespace web
}  // namespace sophon_stream
