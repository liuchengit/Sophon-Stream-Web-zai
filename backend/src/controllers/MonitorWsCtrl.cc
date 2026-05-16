#include "MonitorWsCtrl.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using namespace sophon_stream::web::controllers;

void MonitorWsCtrl::handleNewMessage(const WebSocketConnectionPtr& wsConnPtr,
                                     std::string&& message,
                                     const WebSocketMessageType& type) {
    if (type == WebSocketMessageType::Text) {
        try {
            auto req = nlohmann::json::parse(message);
            std::string action = req.value("action", std::string());

            if (action == "subscribe") {
                std::string channel = req.value("channel", std::string());
                spdlog::info("WebSocket client subscribed to: {}", channel);

                nlohmann::json resp;
                resp["type"] = "subscribed";
                resp["channel"] = channel;
                wsConnPtr->send(resp.dump());
            }

        } catch (const std::exception& e) {
            spdlog::warn("Invalid WebSocket message: {}", e.what());
        }
    }
}

void MonitorWsCtrl::handleConnectionClosed(const WebSocketConnectionPtr& wsConnPtr,
                                           int code,
                                           std::string&& reason) {
    spdlog::info("WebSocket connection closed: {} - {}", code, reason);
}
