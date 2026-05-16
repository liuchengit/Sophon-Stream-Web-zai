#pragma once

#include <drogon/WebSocketController.h>
#include <drogon/drogon.h>

using namespace drogon;

namespace sophon_stream {
namespace web {
namespace controllers {

class MonitorWsCtrl {
  public:
    static void handleNewMessage(const WebSocketConnectionPtr& wsConnPtr,
                                 std::string&& message,
                                 const WebSocketMessageType& type);
    static void handleConnectionClosed(const WebSocketConnectionPtr& wsConnPtr,
                                       int code,
                                       std::string&& reason);
};

}  // namespace controllers
}  // namespace web
}  // namespace sophon_stream
