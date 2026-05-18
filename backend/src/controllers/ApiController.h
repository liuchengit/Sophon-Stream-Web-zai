#pragma once

#include <drogon/HttpSimpleController.h>
#include <string>

using namespace drogon;

namespace sophon_stream {
namespace web {
namespace controllers {

class HealthCtrl : public drogon::HttpSimpleController<HealthCtrl> {
  public:
    virtual void asyncHandleHttpRequest(const HttpRequestPtr& req,
                                        std::function<void(const HttpResponsePtr&)>&& callback) override;
    PATH_LIST_BEGIN
    PATH_ADD("/health", Get);
    PATH_ADD("/api/v1/system/info", Get);
    PATH_LIST_END
};

class AuthCtrl : public drogon::HttpSimpleController<AuthCtrl> {
  public:
    virtual void asyncHandleHttpRequest(const HttpRequestPtr& req,
                                        std::function<void(const HttpResponsePtr&)>&& callback) override;
    PATH_LIST_BEGIN
    PATH_ADD("/api/v1/auth/login", Post);
    PATH_ADD("/api/v1/auth/register", Post);
    PATH_ADD("/api/v1/auth/refresh", Post);
    PATH_ADD("/api/v1/auth/users", Get);
    PATH_ADD("/api/v1/auth/users/:id", Put, Delete);
    PATH_LIST_END
};

class DeviceCtrl : public drogon::HttpSimpleController<DeviceCtrl> {
  public:
    virtual void asyncHandleHttpRequest(const HttpRequestPtr& req,
                                        std::function<void(const HttpResponsePtr&)>&& callback) override;
    PATH_LIST_BEGIN
    PATH_ADD("/api/v1/devices", Get, Post, Put, Delete);
    PATH_ADD("/api/v1/devices/start", Post);
    PATH_ADD("/api/v1/devices/stop", Post);
    PATH_LIST_END
};

class TaskCtrl : public drogon::HttpSimpleController<TaskCtrl> {
  public:
    virtual void asyncHandleHttpRequest(const HttpRequestPtr& req,
                                        std::function<void(const HttpResponsePtr&)>&& callback) override;
    PATH_LIST_BEGIN
    PATH_ADD("/api/v1/tasks", Get, Post, Put, Delete);
    PATH_ADD("/api/v1/tasks/start", Post);
    PATH_ADD("/api/v1/tasks/stop", Post);
    PATH_ADD("/api/v1/tasks/pause", Post);
    PATH_ADD("/api/v1/tasks/resume", Post);
    PATH_LIST_END
};

class AlgorithmCtrl : public drogon::HttpSimpleController<AlgorithmCtrl> {
  public:
    virtual void asyncHandleHttpRequest(const HttpRequestPtr& req,
                                        std::function<void(const HttpResponsePtr&)>&& callback) override;
    PATH_LIST_BEGIN
    PATH_ADD("/api/v1/algorithms", Get);
    PATH_LIST_END
};

class AlertCtrl : public drogon::HttpSimpleController<AlertCtrl> {
  public:
    virtual void asyncHandleHttpRequest(const HttpRequestPtr& req,
                                        std::function<void(const HttpResponsePtr&)>&& callback) override;
    PATH_LIST_BEGIN
    PATH_ADD("/api/v1/alerts", Get);
    PATH_ADD("/api/v1/alerts/stats", Post);
    PATH_ADD("/api/v1/alert-rules", Get);
    PATH_LIST_END
};

class MonitorCtrl : public drogon::HttpSimpleController<MonitorCtrl> {
  public:
    virtual void asyncHandleHttpRequest(const HttpRequestPtr& req,
                                        std::function<void(const HttpResponsePtr&)>&& callback) override;
    PATH_LIST_BEGIN
    PATH_ADD("/api/v1/monitor/system", Get);
    PATH_ADD("/api/v1/dashboard", Get);
    PATH_LIST_END
};

class PluginCtrl : public drogon::HttpSimpleController<PluginCtrl> {
  public:
    virtual void asyncHandleHttpRequest(const HttpRequestPtr& req,
                                        std::function<void(const HttpResponsePtr&)>&& callback) override;
    PATH_LIST_BEGIN
    PATH_ADD("/api/v1/plugins", Get);
    PATH_LIST_END
};

}  // namespace controllers
}  // namespace web
}  // namespace sophon_stream
