#include "MonitorWsHandler.h"
#include <spdlog/spdlog.h>
#include <ctime>

using namespace sophon_stream::web::websocket;

std::set<WebSocketConnectionPtr> MonitorWsHandler::connections_;
std::mutex MonitorWsHandler::mutex_;

void MonitorWsHandler::handleNewConnection(const HttpRequestPtr& req, const WebSocketConnectionPtr& wsConnPtr) {
    spdlog::info("New WebSocket connection from {}", req->peerAddr().toIp());
    
    std::lock_guard<std::mutex> lock(mutex_);
    connections_.insert(wsConnPtr);
    
    nlohmann::json welcomeMsg;
    welcomeMsg["type"] = "connected";
    welcomeMsg["message"] = "Connected to monitor";
    wsConnPtr->send(welcomeMsg.dump());
}

void MonitorWsHandler::handleNewMessage(const WebSocketConnectionPtr& wsConnPtr, std::string&& message, const WebSocketMessageType& type) {
    (void)type;
    try {
        auto msg = nlohmann::json::parse(message);
        std::string msgType = msg.value("type", "");
        
        if (msgType == "ping") {
            nlohmann::json pong;
            pong["type"] = "pong";
            pong["timestamp"] = std::time(nullptr);
            wsConnPtr->send(pong.dump());
        }
    } catch (const std::exception& e) {
        spdlog::error("WebSocket message error: {}", e.what());
    }
}

void MonitorWsHandler::handleConnectionClosed(const WebSocketConnectionPtr& wsConnPtr) {
    spdlog::info("WebSocket connection closed");
    
    std::lock_guard<std::mutex> lock(mutex_);
    connections_.erase(wsConnPtr);
}
