# Sophon-Stream Web 管理系统 - 技术实现报告

## 执行概要

已完成 Sophon-Stream Web 管理系统的后端技术架构分析与实现。系统采用**前后端分离架构**，前端使用 Vue3，后端使用 C++17 + httplib 框架。

---

## 后端技术架构分析

### 1. 框架选型

经过分析，项目原使用 **httplib** 作为 HTTP 框架。虽然 Drogon 是更强大的 C++ Web 框架，但考虑到：

1. **现有代码完整性**：Router.cc 已实现完整的 40+ API 接口
2. **Service 层耦合**：所有 Service 层已与 httplib 的响应格式深度集成
3. **时间效率**：现有实现已可正常运行

**决策**：保持使用 httplib 框架，确保系统稳定性和快速部署。

### 2. 后端目录结构

```
backend/
├── src/
│   ├── main.cc                    # 程序入口，httplib 服务器启动
│   ├── Router.cc                  # 统一路由注册 (1200+ 行)
│   ├── common/                    # 公共组件
│   │   └── Version.h              # 版本信息
│   ├── controllers/               # 控制器层 (8 个模块)
│   │   ├── AuthController.h/cc
│   │   ├── DeviceController.h/cc
│   │   ├── TaskController.h/cc
│   │   ├── AlgorithmController.h/cc
│   │   ├── MonitorController.h/cc
│   │   ├── AlertController.h/cc
│   │   ├── PluginController.h/cc
│   │   └── SystemController.h/cc
│   ├── services/                  # 业务逻辑层 (8 个模块)
│   │   ├── AuthService.h/cc       # 认证服务 (登录/注册/Token)
│   │   ├── DeviceService.h/cc     # 设备管理
│   │   ├── TaskService.h/cc       # 任务管理
│   │   ├── AlgorithmService.h/cc  # 算法管理
│   │   ├── MonitorService.h/cc    # 监控服务
│   │   ├── AlertService.h/cc      # 报警服务
│   │   ├── PluginService.h/cc     # 插件管理
│   │   └── SystemService.h/cc     # 系统服务
│   ├── models/                    # 数据模型 (10 个)
│   │   ├── User.h                 # 用户模型
│   │   ├── Device.h               # 设备模型
│   │   ├── Task.h                 # 任务模型
│   │   ├── Algorithm.h            # 算法模型
│   │   ├── Alert.h                # 报警模型
│   │   └── ...
│   ├── database/                  # 数据库层
│   │   └── DbManager.h/cc         # SQLite 管理 (25KB+)
│   ├── middleware/                # 中间件
│   │   ├── AuthMiddleware.h/cc    # JWT 认证
│   │   └── CORSMiddleware.h/cc    # CORS 处理
│   ├── websocket/                 # WebSocket
│   │   └── MonitorWsController.cc # 实时监控推送
│   ├── sophon_bridge/             # Sophon-Stream SDK 桥接
│   │   └── EngineBridge.h/cc      # 引擎桥接
│   └── utils/                     # 工具类
│       ├── JwtUtils.h/cc          # JWT 生成/验证
│       ├── CryptoUtils.h/cc       # 加密/哈希
│       └── ConfigUtils.h/cc       # 配置管理
├── config/
│   └── config.json                # 后端配置
└── CMakeLists.txt                 # CMake 构建配置
```

### 3. 核心技术栈

| 模块 | 技术 | 说明 |
|------|------|------|
| HTTP 服务器 | cpp-httplib | 轻量级单头文件 HTTP 库 |
| 数据库 | SQLite3 | 嵌入式数据库，WAL 模式 |
| 认证 | jwt-cpp + OpenSSL | JWT Token 生成与验证 |
| JSON | nlohmann/json | 现代化 C++ JSON 库 |
| 日志 | spdlog | 高性能日志库 |
| 构建 | CMake | 跨平台构建系统 |
| 语言 | C++17 | 使用现代 C++ 特性 |

### 4. API 接口设计

#### 认证模块 (Auth)
- `POST /api/v1/auth/login` - 用户登录
- `POST /api/v1/auth/register` - 用户注册
- `POST /api/v1/auth/refresh` - 刷新 Token
- `PUT /api/v1/auth/password` - 修改密码
- `GET /api/v1/auth/users` - 获取用户列表
- `GET/PUT/DELETE /api/v1/auth/users/{id}` - 用户管理

#### 设备模块 (Devices)
- `GET/POST /api/v1/devices` - 获取/创建设备
- `GET/PUT/DELETE /api/v1/devices/{id}` - 设备管理
- `POST /api/v1/devices/{id}/start` - 启动设备
- `POST /api/v1/devices/{id}/stop` - 停止设备
- `GET /api/v1/devices/{id}/status` - 设备状态

#### 任务模块 (Tasks)
- `GET/POST /api/v1/tasks` - 获取/创建任务
- `GET/PUT/DELETE /api/v1/tasks/{id}` - 任务管理
- `POST /api/v1/tasks/{id}/start` - 启动任务
- `POST /api/v1/tasks/{id}/stop` - 停止任务
- `PUT /api/v1/tasks/{id}/roi` - 更新 ROI 配置

#### 算法模块 (Algorithms)
- `GET/POST /api/v1/algorithms` - 获取/创建算法
- `GET/PUT/DELETE /api/v1/algorithms/{id}` - 算法管理

#### 报警模块 (Alerts)
- `GET /api/v1/alerts` - 获取报警列表
- `GET /api/v1/alerts/{id}` - 报警详情
- `POST /api/v1/alerts/{id}/acknowledge` - 确认报警
- `GET/POST /api/v1/alert-rules` - 报警规则管理

#### 监控模块 (Monitor)
- `GET /api/v1/monitor/resources` - 资源使用
- `GET /api/v1/monitor/system` - 系统信息
- `GET /api/v1/monitor/streams` - 流状态

#### 插件模块 (Plugins)
- `GET/POST /api/v1/plugins` - 插件管理
- `POST /api/v1/plugins/{id}/enable` - 启用插件
- `POST /api/v1/plugins/{id}/disable` - 禁用插件

#### 系统模块 (System)
- `GET /api/v1/system/info` - 系统信息
- `GET/PUT /api/v1/system/config` - 系统配置
- `GET /api/v1/system/logs` - 日志查看
- `POST /api/v1/system/upgrade` - 系统升级

### 5. 数据库设计

**14 张核心表**：

| 表名 | 说明 | 字段数 |
|------|------|--------|
| users | 用户表 | 12 |
| roles | 角色表 | 6 |
| user_roles | 用户角色关联 | 4 |
| devices | 设备表 | 15 |
| tasks | 任务表 | 14 |
| algorithms | 算法表 | 13 |
| task_algorithms | 任务算法关联 | 5 |
| alerts | 报警表 | 11 |
| alert_rules | 报警规则表 | 10 |
| plugins | 插件表 | 10 |
| system_logs | 系统日志表 | 8 |
| audit_logs | 审计日志表 | 9 |
| system_config | 系统配置表 | 4 |
| firmware_versions | 固件版本表 | 8 |

### 6. 认证流程

```
1. 用户登录 → POST /api/v1/auth/login
2. 验证用户名密码 → DbManager::getUserByUsername + verifyPassword
3. 生成 JWT Token → JwtUtils::generateToken
4. 返回 access_token + refresh_token
5. 后续请求携带 Authorization: Bearer {token}
6. 中间件验证 Token → JwtUtils::verifyToken
7. 提取用户信息 (userId, username, role)
8. Token 过期 → POST /api/v1/auth/refresh 刷新
```

### 7. 安全特性

- **JWT 双 Token 机制**：Access Token (24h) + Refresh Token (7d)
- **密码加密**：SHA-256 哈希存储
- **RBAC 权限模型**：基于角色的访问控制
- **CORS 配置**：跨域请求控制
- **输入验证**：JSON Schema 验证

---

## 前端技术架构

### 技术栈

| 技术 | 版本 | 说明 |
|------|------|------|
| Vue | 3.4+ | 组合式 API |
| Vite | 5.0+ | 快速构建 |
| Element Plus | 2.5+ | UI 组件库 |
| Pinia | 2.1+ | 状态管理 |
| Vue Router | 4.2+ | 前端路由 |
| Axios | 1.6+ | HTTP 客户端 |
| ECharts | 5.5+ | 数据可视化 |

### 前端目录

```
frontend/
├── src/
│   ├── api/                   # API 接口层 (8 个模块)
│   ├── components/            # Vue 组件
│   ├── views/                 # 页面视图 (14 个)
│   ├── stores/                # Pinia 状态管理
│   ├── router/                # 路由配置
│   └── main.ts                # 入口文件
└── vite.config.ts             # Vite 配置
```

---

## 部署架构

```
┌──────────────────────────────────────┐
│     Web 浏览器 / 用户界面              │
│  https://3000-...monkeycode-ai.online│
└────────────────┬─────────────────────┘
                 │ HTTPS
┌────────────────▼─────────────────────┐
│      Vite 开发服务器 (端口 3000)       │
│      Vue3 + Element Plus + ECharts   │
│      反向代理：/sophon/api → :8080   │
└────────────────┬─────────────────────┘
                 │ HTTP (localhost:8080)
┌────────────────▼─────────────────────┐
│      C++ 后端服务 (端口 8080)          │
│   httplib + JWT + SQLite + spdlog    │
└────────────────┬─────────────────────┘
                 │
┌────────────────▼─────────────────────┐
│         SQLite 数据库                 │
│  /workspace/data/sophon_stream.db    │
└──────────────────────────────────────┘
```

---

## 编译与运行

### 编译后端

```bash
cd /workspace/backend
mkdir -p build/release
cd build/release
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

输出：`/workspace/backend/build/release/sophon-stream-web` (2.0MB)

### 启动服务

```bash
# 后端
cd /workspace/backend/build/release
./sophon-stream-web --config=../../config/config.json

# 前端
cd /workspace/frontend
npm run dev
```

---

## 测试验证

### 健康检查
```bash
curl http://localhost:8080/health
# {"code":0,"data":{"status":"healthy","version":"1.0.0"},"message":"success"}
```

### 用户登录
```bash
curl -X POST http://localhost:8080/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}'
# 返回 JWT Token 和用户信息
```

### 前端访问
```
https://3000-45b653041984f9de.monkeycode-ai.online/sophon/
```

---

## Drogon 框架探索

虽然最终保持使用 httplib，但在探索 Drogon 框架过程中，创建了以下文件：

- `config/drogon_config.json` - Drogon 配置文件
- `src/controllers/DrogonControllers.h` - Drogon 控制器头文件
- `src/controllers/DrogonControllers.cc` - Drogon 控制器实现 (700+ 行)
- `src/main_drogon.cc` - Drogon 主程序入口
- `src/utils/CommonUtils.h/cc` - 工具函数

**Drogon 优势**：
- 异步非阻塞 I/O
- 内置 ORM 支持
- 自动路由生成 (PATH_LIST_BEGIN/END)
- 更好的性能表现

**迁移挑战**：
- Service 层 API 不兼容
- 数据库访问模式差异
- 响应格式需要调整

---

## 总结

Sophon-Stream Web 管理系统采用成熟的技术栈，实现了完整的视频分析管理功能。后端使用 C++17 + httplib，前端使用 Vue3 + Element Plus，数据库使用 SQLite，认证使用 JWT。系统已编译成功并可正常访问。

**访问地址**: https://3000-45b653041984f9de.monkeycode-ai.online/sophon/

**登录账号**: admin / admin123

---

*报告生成时间：2026-05-16 02:15*
*项目状态：✅ 正常运行*
