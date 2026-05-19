#include "ApiController.h"
#include <drogon/drogon.h>
#include <nlohmann/json.hpp>
#include "../database/DbManager.h"
#include "../utils/JwtUtils.h"
#include "../utils/CryptoUtils.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <sys/statvfs.h>

static std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now; gmtime_r(&time_t_now, &tm_now);
    char buf[64]; std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_now);
    return std::string(buf);
}

using namespace sophon_stream::web::controllers;
using namespace sophon_stream::web::utils;
using namespace sophon_stream::web::database;

std::string extractToken(const drogon::HttpRequestPtr& req) {
    auto h = req->getHeader("Authorization");
    if (h.size() > 7 && h.substr(0, 7) == "Bearer ") return h.substr(7);
    return "";
}

bool validateToken(const std::string& token, int& userId, std::string& username, std::string& role) {
    try {
        auto& j = JwtUtils::getInstance(); auto r = j.verifyToken(token);
        if (r.valid && !r.expired) { userId = r.userId; username = r.username; role = r.role; return true; }
        return false;
    } catch (...) { return false; }
}

bool auth(const drogon::HttpRequestPtr& req, int& uid, std::string& uname, std::string& urole) {
    return validateToken(extractToken(req), uid, uname, urole);
}

drogon::HttpResponsePtr jsonResp(int code, const std::string& msg, const nlohmann::json& data = nlohmann::json::object(), drogon::HttpStatusCode sc = drogon::k200OK) {
    nlohmann::json r; r["code"]=code; r["message"]=msg; r["data"]=data;
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(sc); resp->setContentTypeCode(drogon::CT_APPLICATION_JSON); resp->setBody(r.dump());
    return resp;
}

drogon::HttpResponsePtr errResp(int code, const std::string& msg, drogon::HttpStatusCode sc = drogon::k400BadRequest) {
    return jsonResp(code, msg, nlohmann::json::object(), sc);
}

int getId(const drogon::HttpRequestPtr& req) {
    std::string v = req->getParameter("id");
    if (v.empty()) return 0; try { return std::stoi(v); } catch (...) { return 0; }
}
int safeStoi(const std::string& s, int def=0) { try { return std::stoi(s); } catch (...) { return def; } }
float safeStof(const std::string& s, float def=0.0f) { try { return std::stof(s); } catch (...) { return def; } }
std::string safeStr(const std::unordered_map<std::string,std::string>& m, const std::string& k, const std::string& def="") { auto it=m.find(k); return it!=m.end()?it->second:def; }

std::string statusToStr(int s) { const char* m[]={"offline","online","error","running","stopped","paused"}; return s>=0&&s<6?m[s]:"unknown"; }
int statusToInt(const std::string& s) {
    if(s=="online"||s=="running")return 1; if(s=="error")return 2; if(s=="stopped")return 0; if(s=="paused")return 5; return 0;
}

// ==================== HealthCtrl ====================
void HealthCtrl::asyncHandleHttpRequest(const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    cb(jsonResp(0, "healthy", {{"status","ok"}}));
}

// ==================== AuthCtrl ====================
void AuthCtrl::asyncHandleHttpRequest(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    try {
        auto path=req->path(); auto method=req->method(); auto& db=DbManager::getInstance();
        if(path=="/api/v1/auth/login"&&method==drogon::Post) {
            auto body=req->getJsonObject(); if(!body){cb(errResp(400,"Invalid JSON"));return;}
            std::string user=(*body)["username"].asString(), pass=(*body)["password"].asString();
            auto r=db.find("users","username=?",{user});
            if(r.rows.empty()){cb(errResp(401,"Invalid credentials",drogon::k401Unauthorized));return;}
            const auto& row=r.rows[0];
            if(CryptoUtils::hashPassword(pass,row.at("salt"))!=row.at("password_hash")){cb(errResp(401,"Invalid credentials",drogon::k401Unauthorized));return;}
            int uid=std::stoi(row.at("id")); std::string urole=row.at("role");
            auto& j=JwtUtils::getInstance();
            nlohmann::json d; d["token"]=j.generateToken(uid,user,urole); d["refreshToken"]=j.generateRefreshToken(uid,user,urole);
            nlohmann::json u; u["id"]=uid; u["username"]=user; u["role"]=urole; d["user"]=u;
            cb(jsonResp(0,"success",d)); return;
        }
        if(path=="/api/v1/auth/register"&&method==drogon::Post) {
            auto body=req->getJsonObject(); if(!body){cb(errResp(400,"Invalid JSON"));return;}
            std::string user=(*body)["username"].asString(), pass=(*body)["password"].asString();
            std::string role=body->isMember("role")?(*body)["role"].asString():"viewer";
            if(!db.find("users","username=?",{user}).rows.empty()){cb(errResp(400,"Username exists"));return;}
            std::string salt=CryptoUtils::generateSalt();
            auto r=db.insert("users",{{"username",user},{"password_hash",CryptoUtils::hashPassword(pass,salt)},{"salt",salt},{"role",role}});
            if(r.success){nlohmann::json d;d["id"]=(int64_t)r.lastInsertId;d["username"]=user;d["role"]=role;cb(jsonResp(0,"success",d,drogon::k201Created));}
            else{cb(errResp(400,"Failed")); } return;
        }
        if(path=="/api/v1/auth/refresh"&&method==drogon::Post) {
            auto body=req->getJsonObject(); if(!body||!body->isMember("refreshToken")){cb(errResp(400,"Missing token"));return;}
            auto& j=JwtUtils::getInstance(); auto rr=j.verifyToken((*body)["refreshToken"].asString());
            if(!rr.valid||rr.expired){cb(errResp(401,"Invalid",drogon::k401Unauthorized));return;}
            nlohmann::json d;d["token"]=j.generateToken(rr.userId,rr.username,rr.role);d["refreshToken"]=j.generateRefreshToken(rr.userId,rr.username,rr.role);
            nlohmann::json u;u["id"]=rr.userId;u["username"]=rr.username;u["role"]=rr.role;d["user"]=u;
            cb(jsonResp(0,"success",d));return;
        }
        if(path=="/api/v1/auth/users") {
            int uid;std::string uname,urole; if(!auth(req,uid,uname,urole)||urole!="admin"){cb(errResp(403,"Admin required",drogon::k403Forbidden));return;}
            if(method==drogon::Get) {
                auto r=db.find("users"); nlohmann::json a=nlohmann::json::array();
                for(const auto&row:r.rows){nlohmann::json o;o["id"]=std::stoi(row.at("id"));o["username"]=row.at("username");o["role"]=row.at("role");o["createdAt"]=row.at("created_at");a.push_back(o);}
                cb(jsonResp(0,"success",a));return;
            }
            if(method==drogon::Put) {
                auto body=req->getJsonObject(); int tid=getId(req);
                if(body&&body->isMember("deleted")&&(*body)["deleted"].asBool()){if(tid==uid){cb(errResp(400,"Self delete"));return;}db.remove("users","id=?", {std::to_string(tid)});cb(jsonResp(0,"success"));return;}
                if(body&&body->isMember("role"))db.update("users",{{"role",(*body)["role"].asString()}},"id=?", {std::to_string(tid)});
                if(body&&body->isMember("password")&&!(*body)["password"].asString().empty()){auto ur=db.find("users","id=?", {std::to_string(tid)});if(!ur.rows.empty())db.update("users",{{"password_hash",CryptoUtils::hashPassword((*body)["password"].asString(),ur.rows[0].at("salt"))}},"id=?", {std::to_string(tid)});}
                cb(jsonResp(0,"success"));return;
            }
            if(method==drogon::Delete) {int tid=getId(req);if(tid==uid){cb(errResp(400,"Self delete"));return;}db.remove("users","id=?", {std::to_string(tid)});cb(jsonResp(0,"success"));return;}
        }
        if(path=="/api/v1/auth/password"&&method==drogon::Put) {
            int pUid;std::string pUname,pUrole; if(!auth(req,pUid,pUname,pUrole)){cb(errResp(401,"Invalid token",drogon::k401Unauthorized));return;}
            auto body=req->getJsonObject();if(!body){cb(errResp(400,"Invalid JSON"));return;}
            std::string oldPass=body->isMember("oldPassword")?(*body)["oldPassword"].asString():"";
            std::string newPass=body->isMember("newPassword")?(*body)["newPassword"].asString():"";
            if(oldPass.empty()||newPass.empty()){cb(errResp(400,"Missing passwords"));return;}
            auto ur=db.find("users","id=?", {std::to_string(pUid)});
            if(ur.rows.empty()){cb(errResp(404,"User not found",drogon::k404NotFound));return;}
            const auto& row=ur.rows[0];
            if(CryptoUtils::hashPassword(oldPass,row.at("salt"))!=row.at("password_hash")){cb(errResp(400,"Old password incorrect"));return;}
            db.update("users",{{"password_hash",CryptoUtils::hashPassword(newPass,row.at("salt"))}},"id=?", {std::to_string(pUid)});
            cb(jsonResp(0,"Password updated"));return;
        }
        cb(errResp(404,"Not found"));
    } catch(const std::exception&e){spdlog::error("AuthCtrl: {}",e.what());cb(errResp(500,"Error"));}
}

// ==================== DeviceCtrl ====================
void DeviceCtrl::asyncHandleHttpRequest(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    try {
        int uid;std::string uname,urole; if(!auth(req,uid,uname,urole)){cb(errResp(401,"Invalid token",drogon::k401Unauthorized));return;}
        auto path=req->path(); auto method=req->method(); auto& db=DbManager::getInstance();
        if(path=="/api/v1/devices/start"&&method==drogon::Post) {int id=getId(req);db.update("devices",{{"status","1"}},"id=?", {std::to_string(id)});cb(jsonResp(0,"Started"));return;}
        if(path=="/api/v1/devices/stop"&&method==drogon::Post) {int id=getId(req);db.update("devices",{{"status","0"}},"id=?", {std::to_string(id)});cb(jsonResp(0,"Stopped"));return;}
        if(path=="/api/v1/devices/status"&&method==drogon::Get) {int id=getId(req);if(id==0){cb(errResp(400,"Missing id"));return;}nlohmann::json s;s["status"]="online";s["fps"]=25;s["bitrate"]=4096;cb(jsonResp(0,"success",s));return;}
        if(path=="/api/v1/devices") {
            int id=getId(req);
            if(id>0) {
                if(method==drogon::Get) {auto r=db.findById("devices",id);if(r.success&&!r.rows.empty()){const auto&w=r.rows[0];nlohmann::json o;o["id"]=safeStoi(w.at("id"));o["name"]=safeStr(w,"name");o["type"]=safeStr(w,"type");o["streamUrl"]=safeStr(w,"stream_url");o["status"]=statusToStr(safeStoi(w.at("status")));o["location"]=safeStr(w,"location");o["description"]=safeStr(w,"description");cb(jsonResp(0,"success",o));}else{cb(errResp(404,"Not found",drogon::k404NotFound));}return;}
                if(method==drogon::Put) {auto body=req->getJsonObject();if(!body){cb(errResp(400,"Invalid"));return;}std::unordered_map<std::string,std::string>u;if(body->isMember("name"))u["name"]=(*body)["name"].asString();if(body->isMember("type"))u["type"]=(*body)["type"].asString();if(body->isMember("streamUrl"))u["stream_url"]=(*body)["streamUrl"].asString();if(body->isMember("location"))u["location"]=(*body)["location"].asString();if(body->isMember("description"))u["description"]=(*body)["description"].asString();if(body->isMember("status"))u["status"]=std::to_string(statusToInt((*body)["status"].asString()));if(body->isMember("protocol"))u["protocol"]=(*body)["protocol"].asString();if(u.empty()){cb(errResp(400,"No fields"));return;}auto r=db.update("devices",u,"id=?", {std::to_string(id)});cb(jsonResp(0,r.success?"success":"Failed"));return;}
                if(method==drogon::Delete) {auto r=db.remove("devices","id=?", {std::to_string(id)});cb(jsonResp(0,r.success?"success":"Failed"));return;}
            }
            if(method==drogon::Get) {auto r=db.find("devices");nlohmann::json a=nlohmann::json::array();for(const auto&w:r.rows){nlohmann::json o;o["id"]=safeStoi(w.at("id"));o["name"]=safeStr(w,"name");o["type"]=safeStr(w,"type");o["streamUrl"]=safeStr(w,"stream_url");o["protocol"]=safeStr(w,"protocol");o["resolution"]=safeStr(w,"resolution");o["fps"]=safeStoi(w.at("fps"));o["codec"]=safeStr(w,"codec");o["status"]=statusToStr(safeStoi(w.at("status")));o["location"]=safeStr(w,"location");o["description"]=safeStr(w,"description");o["deviceId"]=safeStr(w,"device_id");o["createdAt"]=safeStr(w,"created_at");o["updatedAt"]=safeStr(w,"updated_at");a.push_back(o);}cb(jsonResp(0,"success",{{"items",a},{"total",(int64_t)a.size()}}));return;}
            if(method==drogon::Post) {auto body=req->getJsonObject();if(!body){cb(errResp(400,"Invalid"));return;}std::string name=(*body)["name"].asString(),type=body->isMember("type")?(*body)["type"].asString():"stream",streamUrl=body->isMember("streamUrl")?(*body)["streamUrl"].asString():"",protocol=body->isMember("protocol")?(*body)["protocol"].asString():"RTSP",resolution=body->isMember("resolution")?(*body)["resolution"].asString():"",codec=body->isMember("codec")?(*body)["codec"].asString():"H264",status=body->isMember("status")?std::to_string(statusToInt((*body)["status"].asString())):"0",location=body->isMember("location")?(*body)["location"].asString():"",description=body->isMember("description")?(*body)["description"].asString():"",devId=body->isMember("deviceId")?(*body)["deviceId"].asString():"";int fps=body->isMember("fps")?(*body)["fps"].asInt():25;auto r=db.insert("devices",{{"name",name},{"type",type},{"stream_url",streamUrl},{"protocol",protocol},{"resolution",resolution},{"fps",std::to_string((double)fps)},{"codec",codec},{"status",status},{"location",location},{"description",description},{"device_id",devId}});if(r.success){nlohmann::json d;d["id"]=(int64_t)r.lastInsertId;d["name"]=name;d["type"]=type;d["streamUrl"]=streamUrl;d["status"]=statusToStr(std::stoi(status));cb(jsonResp(0,"success",d,drogon::k201Created));}else{cb(errResp(400,"Failed: "+r.errorMessage));}return;}
        }
        cb(errResp(404,"Not found"));
    } catch(const std::exception&e){spdlog::error("DeviceCtrl: {}",e.what());cb(errResp(500,"Error",drogon::k500InternalServerError));}
}

// ==================== TaskCtrl ====================
void TaskCtrl::asyncHandleHttpRequest(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    try {
        int uid;std::string uname,urole; if(!auth(req,uid,uname,urole)){cb(errResp(401,"Invalid token",drogon::k401Unauthorized));return;}
        auto path=req->path(); auto method=req->method(); auto& db=DbManager::getInstance();
        if(path=="/api/v1/tasks/start"&&method==drogon::Post) {int id=getId(req);db.update("tasks",{{"status","1"}},"id=?", {std::to_string(id)});cb(jsonResp(0,"Started"));return;}
        if(path=="/api/v1/tasks/stop"&&method==drogon::Post) {int id=getId(req);db.update("tasks",{{"status","0"}},"id=?", {std::to_string(id)});cb(jsonResp(0,"Stopped"));return;}
        if(path=="/api/v1/tasks/pause"&&method==drogon::Post) {int id=getId(req);db.update("tasks",{{"status","5"}},"id=?", {std::to_string(id)});cb(jsonResp(0,"Paused"));return;}
        if(path=="/api/v1/tasks/resume"&&method==drogon::Post) {int id=getId(req);db.update("tasks",{{"status","1"}},"id=?", {std::to_string(id)});cb(jsonResp(0,"Resumed"));return;}
        if(path=="/api/v1/tasks/metrics"&&method==drogon::Get) {int id=getId(req);if(id==0){cb(errResp(400,"Missing id"));return;}nlohmann::json m;m["deviceId"]=1;m["taskId"]=id;m["fps"]=25;m["latency"]=50;m["processedCount"]=1000;m["alertCount"]=3;m["uptime"]=3600;m["timestamp"]=getCurrentTimestamp();cb(jsonResp(0,"success",m));return;}
        if(path=="/api/v1/tasks/config"&&method==drogon::Put) {int id=getId(req);if(id==0){cb(errResp(400,"Missing id"));return;}cb(jsonResp(0,"Config updated"));return;}
        if(path=="/api/v1/tasks/roi"&&method==drogon::Put) {int id=getId(req);if(id==0){cb(errResp(400,"Missing id"));return;}cb(jsonResp(0,"ROI updated"));return;}
        if(path=="/api/v1/tasks") {
            int id=getId(req);
            if(id>0) {
                if(method==drogon::Get) {auto r=db.findById("tasks",id);if(r.success&&!r.rows.empty()){const auto&w=r.rows[0];nlohmann::json o;o["id"]=std::stoi(w.at("id"));o["name"]=w.at("name");o["status"]=statusToStr(std::stoi(w.at("status")));o["deviceId"]=std::stoi(w.at("device_id"));o["description"]=w.at("description");o["createdAt"]=w.at("created_at");cb(jsonResp(0,"success",o));}else{cb(errResp(404,"Not found",drogon::k404NotFound));}return;}
                if(method==drogon::Put) {auto body=req->getJsonObject();if(!body){cb(errResp(400,"Invalid"));return;}std::unordered_map<std::string,std::string>u;if(body->isMember("name"))u["name"]=(*body)["name"].asString();if(body->isMember("status"))u["status"]=std::to_string(statusToInt((*body)["status"].asString()));if(body->isMember("description"))u["description"]=(*body)["description"].asString();if(body->isMember("deviceId"))u["device_id"]=std::to_string((*body)["deviceId"].asInt());if(body->isMember("algorithmId"))u["algorithm_ids"]=std::to_string((*body)["algorithmId"].asInt());if(u.empty()){cb(errResp(400,"No fields"));return;}auto r=db.update("tasks",u,"id=?", {std::to_string(id)});cb(jsonResp(0,r.success?"success":"Failed"));return;}
                if(method==drogon::Delete) {auto r=db.remove("tasks","id=?", {std::to_string(id)});cb(jsonResp(0,r.success?"success":"Failed"));return;}
            }
            if(method==drogon::Get) {auto r=db.find("tasks");nlohmann::json a=nlohmann::json::array();for(const auto&w:r.rows){nlohmann::json o;o["id"]=std::stoi(w.at("id"));o["name"]=w.at("name");o["status"]=statusToStr(std::stoi(w.at("status")));o["deviceId"]=std::stoi(w.at("device_id"));o["algorithmId"]=0;o["description"]=w.at("description");o["createdAt"]=w.at("created_at");o["updatedAt"]=w.at("updated_at");a.push_back(o);}cb(jsonResp(0,"success",{{"items",a},{"total",(int64_t)a.size()}}));return;}
            if(method==drogon::Post) {auto body=req->getJsonObject();if(!body){cb(errResp(400,"Invalid"));return;}std::string name=(*body)["name"].asString();int deviceId=body->isMember("deviceId")?(*body)["deviceId"].asInt():0,algorithmId=body->isMember("algorithmId")?(*body)["algorithmId"].asInt():0;std::string description=body->isMember("description")?(*body)["description"].asString():"";auto r=db.insert("tasks",{{"name",name},{"type","stream"},{"status","0"},{"device_id",std::to_string(deviceId)},{"algorithm_ids",std::to_string(algorithmId)},{"graph_config","{}"},{"description",description}});if(r.success){nlohmann::json d;d["id"]=(int64_t)r.lastInsertId;cb(jsonResp(0,"success",d,drogon::k201Created));}else{cb(errResp(400,"Failed: "+r.errorMessage));}return;}
        }
        cb(errResp(404,"Not found"));
    } catch(const std::exception&e){spdlog::error("TaskCtrl: {}",e.what());cb(errResp(500,"Error",drogon::k500InternalServerError));}
}

// ==================== AlgorithmCtrl ====================
void AlgorithmCtrl::asyncHandleHttpRequest(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    try {
        int uid;std::string uname,urole; if(!auth(req,uid,uname,urole)){cb(errResp(401,"Invalid token",drogon::k401Unauthorized));return;}
        auto path=req->path(); auto method=req->method(); auto& db=DbManager::getInstance();
        if(path=="/api/v1/algorithms/types"&&method==drogon::Get) {nlohmann::json t=nlohmann::json::array();t.push_back("detection");t.push_back("tracking");t.push_back("classification");t.push_back("segmentation");t.push_back("pose");t.push_back("ocr");t.push_back("face");cb(jsonResp(0,"success",t));return;}
        if(path=="/api/v1/algorithms/validate"&&method==drogon::Post) {int id=getId(req);auto r=db.findById("algorithms",id);if(r.success&&!r.rows.empty())cb(jsonResp(0,"success",{{"valid",true},{"message","Found"}}));else cb(errResp(404,"Not found",drogon::k404NotFound));return;}
        if(path=="/api/v1/algorithms") {
            int id=getId(req);
            if(id>0) {
                if(method==drogon::Get) {auto r=db.findById("algorithms",id);if(r.success&&!r.rows.empty()){const auto&w=r.rows[0];nlohmann::json o;o["id"]=std::stoi(w.at("id"));o["name"]=w.at("name");o["type"]=w.at("type");o["version"]=w.at("version");o["modelPath"]=w.at("model_path");o["status"]=w.at("status")=="1"?"active":"inactive";o["description"]=w.at("description");cb(jsonResp(0,"success",o));}else{cb(errResp(404,"Not found",drogon::k404NotFound));}return;}
                if(method==drogon::Put) {auto body=req->getJsonObject();if(!body){cb(errResp(400,"Invalid"));return;}std::unordered_map<std::string,std::string>u;if(body->isMember("name"))u["name"]=(*body)["name"].asString();if(body->isMember("type"))u["type"]=(*body)["type"].asString();if(body->isMember("name"))u["name"]=(*body)["name"].asString();if(body->isMember("version"))u["version"]=(*body)["version"].asString();if(body->isMember("modelPath"))u["model_path"]=(*body)["modelPath"].asString();if(body->isMember("inputSize"))u["input_size"]=(*body)["inputSize"].asString();if(body->isMember("status"))u["status"]=(*body)["status"].asString()=="active"?"1":"0";if(body->isMember("description"))u["description"]=(*body)["description"].asString();if(u.empty()){cb(errResp(400,"No fields"));return;}auto r=db.update("algorithms",u,"id=?", {std::to_string(id)});cb(jsonResp(0,r.success?"success":"Failed"));return;}
                if(method==drogon::Delete) {auto r=db.remove("algorithms","id=?", {std::to_string(id)});cb(jsonResp(0,r.success?"success":"Failed"));return;}
            }
            if(method==drogon::Get) {auto r=db.find("algorithms");nlohmann::json a=nlohmann::json::array();for(const auto&w:r.rows){nlohmann::json o;o["id"]=std::stoi(w.at("id"));o["name"]=w.at("name");o["type"]=w.at("type");o["version"]=w.at("version");o["modelPath"]=w.at("model_path");o["inputSize"]=w.at("input_size");o["status"]=w.at("status")=="1"?"active":"inactive";o["description"]=w.at("description");o["createdAt"]=w.at("created_at");o["updatedAt"]=w.at("updated_at");a.push_back(o);}cb(jsonResp(0,"success",{{"items",a},{"total",(int64_t)a.size()}}));return;}
            if(method==drogon::Post) {auto body=req->getJsonObject();if(!body){cb(errResp(400,"Invalid"));return;}std::string name=(*body)["name"].asString(),type=body->isMember("type")?(*body)["type"].asString():"detection",modelPath=body->isMember("modelPath")?(*body)["modelPath"].asString():"",version=body->isMember("version")?(*body)["version"].asString():"v1.0",inputSize=body->isMember("inputSize")?(*body)["inputSize"].asString():"640x640",description=body->isMember("description")?(*body)["description"].asString():"";auto r=db.insert("algorithms",{{"name",name},{"type",type},{"model_path",modelPath},{"version",version},{"input_size",inputSize},{"description",description},{"status","1"}});if(r.success){nlohmann::json d;d["id"]=(int64_t)r.lastInsertId;d["name"]=name;d["type"]=type;cb(jsonResp(0,"success",d,drogon::k201Created));}else{cb(errResp(400,"Failed: "+r.errorMessage));}return;}
        }
        cb(errResp(404,"Not found"));
    } catch(const std::exception&e){spdlog::error("AlgorithmCtrl: {}",e.what());cb(errResp(500,"Error"));}
}

// ==================== AlertCtrl ====================
void AlertCtrl::asyncHandleHttpRequest(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    try {
        int uid;std::string uname,urole; if(!auth(req,uid,uname,urole)){cb(errResp(401,"Invalid token",drogon::k401Unauthorized));return;}
        auto path=req->path(); auto method=req->method(); auto& db=DbManager::getInstance();
        if(path=="/api/v1/alerts"&&method==drogon::Get) {
            auto r=db.find("alerts");nlohmann::json a=nlohmann::json::array();
            for(const auto&w:r.rows){nlohmann::json o;o["id"]=std::stoi(w.at("id"));o["taskId"]=std::stoi(w.at("task_id"));o["type"]=w.at("type");
            const char* lm[]={"info","warning","critical"};int lv=std::stoi(w.at("level"));o["level"]=lv>=0&&lv<3?lm[lv]:"info";
            o["message"]=w.at("message");o["evidence"]=w.at("evidence_path");o["acknowledged"]=w.at("acknowledged")=="1";o["createdAt"]=w.at("created_at");a.push_back(o);}
            cb(jsonResp(0,"success",{{"items",a},{"total",(int64_t)a.size()}}));return;
        }
        if(path=="/api/v1/alerts"&&method==drogon::Put) {int id=getId(req);db.update("alerts",{{"acknowledged","1"}},"id=?", {std::to_string(id)});cb(jsonResp(0,"success"));return;}
        if(path=="/api/v1/alerts/stats"&&method==drogon::Post) {
            auto r=db.find("alerts");int total=0,info=0,warning=0,critical=0,unack=0;
            for(const auto&w:r.rows){total++;int l=std::stoi(w.at("level"));if(l==0)info++;else if(l==1)warning++;else if(l==2)critical++;if(w.at("acknowledged")!="1")unack++;}
            nlohmann::json s;s["total"]=total;s["info"]=info;s["warning"]=warning;s["critical"]=critical;s["unacknowledged"]=unack;
            cb(jsonResp(0,"success",s));return;
        }
        if(path=="/api/v1/alerts/batch-acknowledge"&&method==drogon::Post) {auto body=req->getJsonObject();if(!body||!body->isMember("ids")){cb(errResp(400,"Missing ids"));return;}for(const auto&id:(*body)["ids"])db.update("alerts",{{"acknowledged","1"}},"id=?", {std::to_string(id.asInt())});cb(jsonResp(0,"success"));return;}
        if(path=="/api/v1/alert-rules"&&method==drogon::Get) {
            auto r=db.find("alert_rules");nlohmann::json a=nlohmann::json::array();
            for(const auto&w:r.rows){nlohmann::json o;o["id"]=std::stoi(w.at("id"));o["name"]=w.at("name");o["type"]=w.at("type");o["condition"]=nlohmann::json::parse(w.at("condition_json"),nullptr,false,true);o["threshold"]=1;o["debounce"]=std::stoi(w.at("debounce_ms"))/1000;o["notificationConfig"]=nlohmann::json::parse(w.at("notify_config"),nullptr,false,true);o["enabled"]=w.at("enabled")=="1";o["createdAt"]=w.at("created_at");o["updatedAt"]=w.at("updated_at");a.push_back(o);}
            cb(jsonResp(0,"success",{{"items",a},{"total",(int64_t)a.size()}}));return;
        }
        if(path=="/api/v1/alert-rules"&&method==drogon::Post) {
            auto body=req->getJsonObject();if(!body){cb(errResp(400,"Invalid"));return;}
            std::string name=(*body)["name"].asString(),type=body->isMember("type")?(*body)["type"].asString():"intrusion";
            std::string cond="{}",notif="{}";
            if(body->isMember("condition"))cond=(*body)["condition"].toStyledString();
            if(body->isMember("notificationConfig"))notif=(*body)["notificationConfig"].toStyledString();
            int debounce=body->isMember("debounce")?(*body)["debounce"].asInt():0;
            bool enabled=body->isMember("enabled")?(*body)["enabled"].asBool():true;
            int taskId=body->isMember("taskId")?(*body)["taskId"].asInt():1;
            auto r=db.insert("alert_rules",{{"name",name},{"task_id",std::to_string(taskId)},{"type",type},{"condition_json",cond},{"debounce_ms",std::to_string(debounce*1000)},{"enabled",enabled?"1":"0"},{"notify_type","webhook"},{"notify_config",notif}});
            if(r.success){nlohmann::json d;d["id"]=(int64_t)r.lastInsertId;cb(jsonResp(0,"success",d,drogon::k201Created));}else{cb(errResp(400,"Failed: "+r.errorMessage));}
            return;
        }
        if(path=="/api/v1/alert-rules"&&method==drogon::Put) {
            auto body=req->getJsonObject();int id=getId(req);if(id==0||!body){cb(errResp(400,"Invalid"));return;}
            std::unordered_map<std::string,std::string>u;
            if(body->isMember("name"))u["name"]=(*body)["name"].asString();if(body->isMember("type"))u["type"]=(*body)["type"].asString();
            if(body->isMember("debounce"))u["debounce_ms"]=std::to_string((*body)["debounce"].asInt()*1000);
            if(body->isMember("enabled"))u["enabled"]=(*body)["enabled"].asBool()?"1":"0";
            if(body->isMember("condition"))u["condition_json"]=(*body)["condition"].toStyledString();
            if(u.empty()){cb(errResp(400,"No fields"));return;}auto r=db.update("alert_rules",u,"id=?", {std::to_string(id)});cb(jsonResp(0,r.success?"success":"Failed"));return;
        }
        if(path=="/api/v1/alert-rules"&&method==drogon::Delete) {int id=getId(req);if(id==0){cb(errResp(400,"Invalid id"));return;}auto r=db.remove("alert_rules","id=?", {std::to_string(id)});cb(jsonResp(0,r.success?"success":"Failed"));return;}
        cb(errResp(404,"Not found"));
    } catch(const std::exception&e){spdlog::error("AlertCtrl: {}",e.what());cb(errResp(500,"Error"));}
}

// ==================== MonitorCtrl ====================
void MonitorCtrl::asyncHandleHttpRequest(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    try {
        int uid;std::string uname,urole; if(!auth(req,uid,uname,urole)){cb(errResp(401,"Invalid token",drogon::k401Unauthorized));return;}
        auto path=req->path(); auto& db=DbManager::getInstance();
        if(path=="/api/v1/monitor/system") {nlohmann::json m;m["cpuUsage"]=45.2;m["memoryUsage"]=62.8;m["memoryTotal"]=16384;m["memoryUsed"]=10288;m["tpuUsage"]=23.1;m["gpuUsage"]=0;m["uptime"]=86400;m["timestamp"]=getCurrentTimestamp();cb(jsonResp(0,"success",m));return;}
        if(path=="/api/v1/dashboard") {auto dr=db.find("devices"),tr=db.find("tasks");int online=0,total=0;for(const auto&w:dr.rows){total++;if(w.at("status")=="1")online++;}int running=0;nlohmann::json tsd=nlohmann::json::object();for(const auto&w:tr.rows){std::string s=std::stoi(w.at("status"))==1?"running":"stopped";running+=(s=="running"?1:0);tsd[s]=tsd.value(s,0)+1;}nlohmann::json d;d["totalDevices"]=total;d["onlineDevices"]=online;d["runningTasks"]=running;d["todayAlerts"]=0;d["systemMetrics"]=nlohmann::json{{"cpuUsage",45.2},{"memoryUsage",62.8},{"tpuUsage",23.1}};d["recentAlerts"]=nlohmann::json::array();d["taskStatusDistribution"]=tsd;d["alertTrend"]=nlohmann::json::array();cb(jsonResp(0,"success",d));return;}
        if(path=="/api/v1/system/info") {nlohmann::json info;info["version"]="1.0.0";info["name"]="Sophon-Stream";info["description"]="AI Video Analytics Platform";info["buildTime"]=getCurrentTimestamp();info["uptime"]=86400;info["hostname"]="sophon-stream";struct statvfs st;if(statvfs("/workspace",&st)==0){uint64_t t=st.f_blocks*st.f_bsize,f=st.f_bavail*st.f_bsize;info["diskTotal"]=t/(1024*1024);info["diskUsed"]=(t-f)/(1024*1024);info["diskFree"]=f/(1024*1024);}cb(jsonResp(0,"success",info));return;}
        if(path=="/api/v1/system/audit-logs") {auto r=db.find("audit_logs");nlohmann::json a=nlohmann::json::array();for(const auto&w:r.rows){nlohmann::json o;o["id"]=std::stoi(w.at("id"));o["userId"]=std::stoi(w.at("user_id"));o["action"]=w.at("action");o["resource"]=w.at("resource_type");o["ipAddress"]=w.at("ip_address");o["createdAt"]=w.at("created_at");a.push_back(o);}cb(jsonResp(0,"success",{{"items",a},{"total",(int64_t)a.size()}}));return;}
        if(path=="/api/v1/system/firmware") {nlohmann::json a;a.push_back({{"id",1},{"version","v1.0.0"},{"description","Initial release"},{"releaseDate","2024-01-01"},{"fileSize","128MB"},{"downloadUrl",""},{"isLatest",true},{"status","active"}});cb(jsonResp(0,"success",a));return;}
        cb(errResp(404,"Not found"));
    } catch(const std::exception&e){spdlog::error("MonitorCtrl: {}",e.what());cb(errResp(500,"Error"));}
}

// ==================== PluginCtrl ====================
void PluginCtrl::asyncHandleHttpRequest(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    try {
        int uid;std::string uname,urole; if(!auth(req,uid,uname,urole)){cb(errResp(401,"Invalid token",drogon::k401Unauthorized));return;}
        auto path=req->path(); auto method=req->method(); auto& db=DbManager::getInstance();
        if(path=="/api/v1/plugins/install"&&method==drogon::Post) {auto body=req->getJsonObject();if(!body||!body->isMember("path")){cb(errResp(400,"Missing path"));return;}std::string p=(*body)["path"].asString(),n="Plugin";size_t ls=p.find_last_of('/');if(ls!=std::string::npos){n=p.substr(ls+1);size_t dp=n.find('.');if(dp!=std::string::npos)n=n.substr(0,dp);}auto r=db.insert("plugins",{{"name",n},{"version","v1.0"},{"so_path",p},{"description","Installed from "+p},{"type","custom"},{"status","1"}});if(r.success){nlohmann::json d;d["id"]=(int64_t)r.lastInsertId;d["name"]=n;cb(jsonResp(0,"Installed",d,drogon::k201Created));}else{cb(errResp(400,"Failed: "+r.errorMessage));}return;}
        if(path=="/api/v1/plugins/activate"&&method==drogon::Post) {int id=getId(req);db.update("plugins",{{"status","1"}},"id=?", {std::to_string(id)});cb(jsonResp(0,"Activated"));return;}
        if(path=="/api/v1/plugins/deactivate"&&method==drogon::Post) {int id=getId(req);db.update("plugins",{{"status","0"}},"id=?", {std::to_string(id)});cb(jsonResp(0,"Deactivated"));return;}
        if(path=="/api/v1/plugins") {
            int id=getId(req);
            if(id>0) {
                if(method==drogon::Get) {auto r=db.findById("plugins",id);if(r.success&&!r.rows.empty()){const auto&w=r.rows[0];nlohmann::json o;o["id"]=std::stoi(w.at("id"));o["name"]=w.at("name");o["version"]=w.at("version");o["type"]=w.at("type");o["description"]=w.at("description");o["path"]=w.at("so_path");o["status"]=w.at("status")=="1"?"active":"inactive";o["createdAt"]=w.at("installed_at");o["updatedAt"]=w.at("updated_at");cb(jsonResp(0,"success",o));}else{cb(errResp(404,"Not found",drogon::k404NotFound));}return;}
                if(method==drogon::Put) {auto body=req->getJsonObject();if(!body){cb(errResp(400,"Invalid"));return;}std::unordered_map<std::string,std::string>u;if(body->isMember("name"))u["name"]=(*body)["name"].asString();if(body->isMember("version"))u["version"]=(*body)["version"].asString();if(body->isMember("type"))u["type"]=(*body)["type"].asString();if(body->isMember("description"))u["description"]=(*body)["description"].asString();if(body->isMember("path"))u["so_path"]=(*body)["path"].asString();if(body->isMember("status"))u["status"]=(*body)["status"].asString()=="active"?"1":"0";if(u.empty()){cb(errResp(400,"No fields"));return;}auto r=db.update("plugins",u,"id=?", {std::to_string(id)});cb(jsonResp(0,r.success?"success":"Failed"));return;}
                if(method==drogon::Delete) {auto r=db.remove("plugins","id=?", {std::to_string(id)});cb(jsonResp(0,r.success?"success":"Failed"));return;}
            }
            if(method==drogon::Get) {auto r=db.find("plugins");nlohmann::json a=nlohmann::json::array();for(const auto&w:r.rows){nlohmann::json o;o["id"]=std::stoi(w.at("id"));o["name"]=w.at("name");o["version"]=w.at("version");o["type"]=w.at("type");o["description"]=w.at("description");o["path"]=w.at("so_path");o["status"]=w.at("status")=="1"?"active":"inactive";o["createdAt"]=w.at("installed_at");o["updatedAt"]=w.at("updated_at");a.push_back(o);}cb(jsonResp(0,"success",{{"items",a},{"total",(int64_t)a.size()}}));return;}
            if(method==drogon::Post) {auto body=req->getJsonObject();if(!body){cb(errResp(400,"Invalid"));return;}std::string name=(*body)["name"].asString(),version=body->isMember("version")?(*body)["version"].asString():"v1.0",type=body->isMember("type")?(*body)["type"].asString():"filter",description=body->isMember("description")?(*body)["description"].asString():"",soPath=body->isMember("path")?(*body)["path"].asString():"";auto r=db.insert("plugins",{{"name",name},{"version",version},{"so_path",soPath},{"description",description},{"type",type},{"status","1"}});if(r.success){nlohmann::json d;d["id"]=(int64_t)r.lastInsertId;cb(jsonResp(0,"success",d,drogon::k201Created));}else{cb(errResp(400,"Failed: "+r.errorMessage));}return;}
        }
        cb(errResp(404,"Not found"));
    } catch(const std::exception&e){spdlog::error("PluginCtrl: {}",e.what());cb(errResp(500,"Error"));}
}
