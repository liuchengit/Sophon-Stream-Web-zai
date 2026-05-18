#pragma once

#include <drogon/WebSocketController.h>
#include <drogon/HttpRequest.h>
#include <nlohmann/json.hpp>
#include <mutex>
#include <set>
#include <string>

using namespace drogon;

namespace sophon_stream {
namespace web {
namespace websocket {

class MonitorWsHandler : public drogon::WebSocketController<MonitorWsHandler> {
public:
    void handleNewMessage(const WebSocketConnectionPtr& wsConnPtr, std::string&& message, const WebSocketMessageType& type) override;
    void handleNewConnection(const HttpRequestPtr& req, const WebSocketConnectionPtr& wsConnPtr) override;
    void handleConnectionClosed(const WebSocketConnectionPtr& wsConnPtr) override;
    
    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/ws/monitor");
    WS_PATH_LIST_END
    
private:
    static std::set<WebSocketConnectionPtr> connections_;
    static std::mutex mutex_;
};

}  // namespace websocket
}  // namespace web
}  // namespace sophon_stream
