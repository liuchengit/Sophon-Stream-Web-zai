# Sophon-Stream Web 管理系统

<div align="center">

**算能 Sophon-Stream 智能视频分析 Web 管理平台**

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-BM1684X%20%7C%20BM1688%20%7C%20x86-green.svg)]()
[![C++](https://img.shields.io/badge/C++-17-blue.svg)]()
[![Vue](https://img.shields.io/badge/Vue-3.4-brightgreen.svg)]()

</div>

---

## 1. 项目简介

Sophon-Stream Web 管理系统是针对算能（Sophon）AI 芯片平台的智能视频分析全流程管理系统。系统采用**控制面与数据面分离**架构，通过 Web 界面提供设备接入、算法管理、任务编排、实时监控、报警处理等全生命周期管理能力。

### 核心特性

- 🎯 **全流程管理**：设备接入 → 算法配置 → 任务编排 → 实时监控 → 报警处理
- 🖥️ **可视化编排**：DAG 管道编辑器，拖拽式构建分析流程
- 📐 **ROI 区域绘制**：可视化绘制检测区域，支持矩形/多边形
- 📊 **实时监控**：CPU/内存/TPU 资源监控，WebSocket 实时推送
- 🔐 **安全认证**：JWT + RBAC 权限模型，细粒度访问控制
- 🔌 **插件架构**：动态 .so 加载，灵活扩展算法能力
- 🐳 **容器化部署**：Docker 一键部署，支持 K8s 编排
- 🔧 **交叉编译**：支持 BM1684X/BM1688 SoC 交叉编译

### 适用场景

- 智慧城市：交通监控、人流统计、事件检测
- 工业质检：产品缺陷检测、安全合规监控
- 安防监控：周界入侵检测、异常行为分析
- 智慧园区：人员管理、车辆管理、环境监测

---

## 2. 系统架构

```
┌─────────────────────────────────────────────────────┐
│                   Web 管理界面                        │
│            (Vue3 + Element Plus + ECharts)           │
├─────────────────────────────────────────────────────┤
│                   API 网关层                          │
│         (Drogon HTTP Server + WebSocket)             │
├──────────┬──────────┬──────────┬────────────────────┤
│  认证授权  │  业务服务  │  监控服务  │   插件管理        │
│ AuthSvc   │ DeviceSvc│ MonitorSvc│ PluginSvc        │
│ RBAC      │ TaskSvc  │ AlertSvc  │ dlopen/dlclose   │
│ JWT       │ AlgoSvc  │ SystemSvc │ SHA-256 校验      │
├──────────┴──────────┴──────────┴────────────────────┤
│                数据持久层                              │
│           (SQLite + WAL Mode)                        │
├─────────────────────────────────────────────────────┤
│              Sophon-Stream 引擎桥接层                 │
│           (EngineBridge / SDK Integration)           │
├─────────────────────────────────────────────────────┤
│              算能 TPU 硬件加速层                       │
│         (BM1684X / BM1688 / BM1684 PCIe)            │
└─────────────────────────────────────────────────────┘
```

### 技术栈

| 层级 | 技术 |
|------|------|
| 前端 | Vue 3.4, TypeScript, Element Plus, ECharts, Pinia |
| 后端 | C++17, Drogon, SQLite3, jwt-cpp, spdlog |
| 通信 | REST API, WebSocket, JSON |
| 认证 | JWT (access + refresh token), RBAC |
| 构建 | CMake 3.14+, Vite 5, npm |
| 部署 | Docker, docker-compose, systemd, Nginx |
| CI/CD | GitHub Actions |
| 硬件 | BM1684X/BM1688 SoC, PCIe 加速卡 |

### 模块说明

| 模块 | 说明 |
|------|------|
| `backend/` | C++ 后端服务，基于 Drogon 框架 |
| `frontend/` | Vue3 前端，深色科技风管理界面 |
| `scripts/` | 构建、部署、开发脚本 |
| `config/` | 配置文件模板 |
| `cmake/` | 交叉编译工具链文件 |
| `.github/` | CI/CD 工作流 |

---

## 3. 功能特性

### 3.1 Web 管理界面
- 深色科技风 UI 主题，适合监控场景
- 响应式布局，适配不同屏幕尺寸
- 实时数据可视化（仪表盘、折线图、饼图）
- NProgress 页面加载进度条

### 3.2 设备管理
- 支持 IPC / NVR / RTSP / 文件 / GB28181 五种设备类型
- 设备在线状态监测与心跳
- 视频流启停控制
- 设备搜索过滤（类型、状态、关键字）

### 3.3 算法管理
- 算法模型注册与管理
- 支持 7 种算法类型：检测/跟踪/分类/分割/OCR/人脸/姿态
- 模型文件验证
- 算法配置 JSON 编辑

### 3.4 任务调度
- 任务全生命周期管理：创建 → 启动 → 暂停 → 恢复 → 停止
- 可视化 DAG 管道编排（7 种节点类型）
- 任务配置热更新（无需重启）
- ROI 区域可视化配置
- 运行时指标监控（FPS/延迟/吞吐量/检测数）

### 3.5 实时监控
- 系统资源监控：CPU / 内存 / TPU 利用率
- 任务级运行指标
- 设备级状态监控
- 仪表盘数据汇总
- WebSocket 实时推送（2s 间隔）

### 3.6 报警管理
- 报警规则引擎（类型/类别/置信度/计数/表达式）
- 防抖机制（可配置间隔）
- 报警事件确认与批量处理
- 报警统计（按级别/类型）
- Webhook / 邮件 / 短信通知

### 3.7 插件系统
- 动态 .so 插件加载（dlopen/dlsym/dlclose）
- SHA-256 校验和验证
- 插件激活/停用
- 配置 Schema 定义与编辑

### 3.8 系统运维
- 系统信息查看（版本/运行时间/资源）
- 系统配置管理（键值对）
- 审计日志（全操作追踪）
- 数据库备份与恢复
- 固件版本管理
- 系统重启

---

## 4. 快速开始

### 4.1 环境要求

| 项目 | 最低要求 |
|------|---------|
| 操作系统 | Ubuntu 20.04+ / Debian 10+ |
| C++ 编译器 | GCC 9+ / Clang 10+ |
| CMake | 3.14+ |
| Node.js | 18.x |
| npm | 9.x |
| 硬件（可选） | BM1684X/BM1688 SoC 或 PCIe 加速卡 |

### 4.2 安装依赖

```bash
# 一键安装所有依赖
sudo ./scripts/install-deps.sh

# 仅安装构建依赖
sudo ./scripts/install-deps.sh --build-only

# 含 sophon-stream SDK
sudo ./scripts/install-deps.sh --sdk

# 交叉编译工具链
sudo ./scripts/install-deps.sh --cross-compile BM1684X
```

### 4.3 编译构建

```bash
# 完整构建（后端 + 前端）
./scripts/build.sh

# 仅构建后端
./scripts/build.sh --backend-only

# 仅构建前端
./scripts/build.sh --frontend-only

# Debug 构建
./scripts/build.sh --type Debug

# 清理后构建
./scripts/build.sh --clean

# 交叉编译 BM1684X
./scripts/build.sh --target aarch64 --toolchain cmake/BM1684X-toolchain.cmake

# 指定并行数
./scripts/build.sh -j 8
```

### 4.4 运行服务

**方式一：开发模式**
```bash
./scripts/dev.sh
# 后端: http://localhost:8080
# 前端: http://localhost:5173
```

**方式二：生产运行**
```bash
cd dist
./bin/sophon-stream-web --config config/config.json
```

**方式三：Systemd 服务**
```bash
sudo cp scripts/sophon-stream-web.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable sophon-stream-web
sudo systemctl start sophon-stream-web
sudo systemctl status sophon-stream-web
```

**方式四：Docker**
```bash
docker-compose up -d
```

### 4.5 访问系统

- 访问地址：`http://localhost:8080`
- 默认账号：`admin`
- 默认密码：`admin123`
- ⚠️ **请务必在首次登录后修改默认密码**

---

## 5. 项目结构

```
sophon-stream-web/
├── backend/                        # C++ 后端
│   ├── CMakeLists.txt              # 后端构建配置
│   ├── config/
│   │   └── config.json             # 后端配置文件
│   └── src/
│       ├── main.cc                 # 入口文件
│       ├── controllers/            # 控制器层（8 个）
│       ├── services/               # 业务逻辑层（8 个）
│       ├── models/                 # 数据模型（10 个）
│       ├── database/               # 数据库管理
│       │   ├── DbManager.h/cc      # 数据库管理器
│       │   └── migrations/         # 数据库迁移脚本
│       ├── middleware/             # 中间件（认证、CORS）
│       ├── websocket/             # WebSocket 控制器
│       ├── sophon_bridge/         # Sophon-Stream SDK 桥接
│       ├── utils/                 # 工具类（JWT、加密、配置）
│       └── tests/                 # 单元测试
├── frontend/                       # Vue3 前端
│   ├── package.json
│   ├── vite.config.ts
│   ├── tsconfig.json
│   └── src/
│       ├── main.ts                # 入口文件
│       ├── api/                   # API 请求层（8 个模块）
│       ├── stores/                # Pinia 状态管理（8 个）
│       ├── router/                # 路由配置
│       ├── views/                 # 页面组件（14 个）
│       ├── components/            # 公共组件
│       │   ├── layout/            # 布局组件
│       │   ├── common/            # 通用组件
│       │   ├── chart/             # 图表组件
│       │   ├── roi/               # ROI 编辑器
│       │   └── dag/               # DAG 编辑器
│       └── assets/                # 静态资源与样式
├── scripts/                        # 运维脚本
│   ├── build.sh                   # 构建脚本
│   ├── deploy.sh                  # 部署脚本
│   ├── dev.sh                     # 开发环境脚本
│   ├── install-deps.sh            # 依赖安装脚本
│   └── sophon-stream-web.service  # Systemd 服务文件
├── config/                         # 配置模板
│   ├── config.template.json       # 配置模板（带注释）
│   └── nginx.conf                 # Nginx 反向代理配置
├── cmake/                          # CMake 工具链
│   ├── BM1684X-toolchain.cmake    # BM1684X 交叉编译
│   └── BM1688-toolchain.cmake     # BM1688 交叉编译
├── .github/workflows/              # CI/CD
│   ├── build.yml                  # 构建与测试工作流
│   └── release.yml                # 发布工作流
├── Dockerfile                      # 多阶段 Docker 构建
├── docker-compose.yml              # Docker Compose 编排
├── .dockerignore                   # Docker 忽略文件
├── .gitignore                      # Git 忽略文件
├── VERSION                         # 版本号
├── CHANGELOG.md                    # 更新日志
├── LICENSE                         # Apache 2.0 许可证
└── README.md                       # 项目说明（本文件）
```

---

## 6. API 文档

### 6.1 API 概述

所有 API 遵循 RESTful 规范，基础路径为 `/api/v1`，统一响应格式：

```json
{
  "code": 0,
  "message": "success",
  "data": { ... }
}
```

**错误码**：0=成功, 401=未授权, 403=禁止, 404=未找到, 500=内部错误, 1001=参数错误, 1002=已存在, 1003=未找到

### 6.2 认证接口

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/v1/auth/login` | 用户登录 |
| POST | `/api/v1/auth/register` | 用户注册 |
| POST | `/api/v1/auth/refresh` | 刷新令牌 |
| PUT | `/api/v1/auth/password` | 修改密码 |
| GET | `/api/v1/users` | 用户列表 |
| GET/PUT/DELETE | `/api/v1/users/{id}` | 用户详情/更新/删除 |

### 6.3 设备接口

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/v1/devices` | 设备列表 |
| POST | `/api/v1/devices` | 创建设备 |
| GET/PUT/DELETE | `/api/v1/devices/{id}` | 设备详情/更新/删除 |
| GET | `/api/v1/devices/{id}/status` | 设备状态 |
| POST | `/api/v1/devices/{id}/start` | 启动视频流 |
| POST | `/api/v1/devices/{id}/stop` | 停止视频流 |

### 6.4 任务接口

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/v1/tasks` | 任务列表 |
| POST | `/api/v1/tasks` | 创建任务 |
| GET/PUT/DELETE | `/api/v1/tasks/{id}` | 任务详情/更新/删除 |
| POST | `/api/v1/tasks/{id}/start` | 启动任务 |
| POST | `/api/v1/tasks/{id}/stop` | 停止任务 |
| POST | `/api/v1/tasks/{id}/pause` | 暂停任务 |
| POST | `/api/v1/tasks/{id}/resume` | 恢复任务 |
| PUT | `/api/v1/tasks/{id}/config` | 热更新配置 |
| PUT | `/api/v1/tasks/{id}/roi` | 更新 ROI |
| GET | `/api/v1/tasks/{id}/metrics` | 运行时指标 |

### 6.5 算法接口

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/v1/algorithms` | 算法列表 |
| POST | `/api/v1/algorithms` | 创建算法 |
| GET/PUT/DELETE | `/api/v1/algorithms/{id}` | 算法详情/更新/删除 |
| POST | `/api/v1/algorithms/{id}/validate` | 验证模型 |
| GET | `/api/v1/algorithms/types` | 算法类型列表 |

### 6.6 监控接口

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/v1/monitor/system` | 系统资源指标 |
| GET | `/api/v1/monitor/tasks/{id}` | 任务运行指标 |
| GET | `/api/v1/monitor/devices/{id}` | 设备运行指标 |
| GET | `/api/v1/monitor/dashboard` | 仪表盘数据 |
| WS | `/ws/monitor` | 实时监控 WebSocket |

### 6.7 报警接口

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/v1/alerts` | 报警事件列表 |
| PUT | `/api/v1/alerts/{id}/acknowledge` | 确认报警 |
| POST | `/api/v1/alerts/batch-acknowledge` | 批量确认 |
| GET | `/api/v1/alerts/stats` | 报警统计 |
| GET | `/api/v1/alerts/rules` | 报警规则列表 |
| POST | `/api/v1/alerts/rules` | 创建规则 |
| PUT/DELETE | `/api/v1/alerts/rules/{id}` | 更新/删除规则 |

### 6.8 插件接口

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/v1/plugins/install` | 安装插件 |
| GET | `/api/v1/plugins` | 插件列表 |
| GET/PUT/DELETE | `/api/v1/plugins/{id}` | 插件详情/更新/删除 |
| POST | `/api/v1/plugins/{id}/activate` | 激活插件 |
| POST | `/api/v1/plugins/{id}/deactivate` | 停用插件 |

### 6.9 系统接口

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/v1/system/info` | 系统信息 |
| GET/PUT | `/api/v1/system/config` | 系统配置 |
| GET | `/api/v1/system/audit-logs` | 审计日志 |
| POST | `/api/v1/system/backup` | 备份数据库 |
| POST | `/api/v1/system/restore` | 恢复数据库 |
| GET | `/api/v1/system/firmware` | 固件版本列表 |
| POST | `/api/v1/system/firmware/{id}/install` | 安装固件 |
| POST | `/api/v1/system/reboot` | 重启系统 |

---

## 7. 配置说明

### 7.1 后端配置

配置文件路径：`backend/config/config.json`

详见配置模板：[config/config.template.json](config/config.template.json)

关键配置项：

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| `listeners[0].port` | 8080 | HTTP 服务端口 |
| `app.threads_num` | 4 | 工作线程数 |
| `app.db_clients[0].filename` | ./data/sophon_stream.db | SQLite 数据库路径 |
| `jwt.secret` | (需修改) | JWT 签名密钥 |
| `jwt.access_token_expire_seconds` | 86400 | Access Token 有效期 |
| `jwt.refresh_token_expire_seconds` | 604800 | Refresh Token 有效期 |
| `cors.allowed_origins` | ["*"] | CORS 允许的来源 |
| `monitor.broadcast_interval_ms` | 2000 | WebSocket 推送间隔 |
| `sophon_stream.sdk_lib_path` | /opt/sophon-stream/lib | SDK 库路径 |

### 7.2 前端配置

前端通过 Vite 代理访问后端 API，配置在 `frontend/vite.config.ts`：

```typescript
proxy: {
  '/api': {
    target: 'http://localhost:8080',
    changeOrigin: true
  },
  '/ws': {
    target: 'ws://localhost:8080',
    ws: true
  }
}
```

### 7.3 Nginx 配置

详见：[config/nginx.conf](config/nginx.conf)

主要特性：
- `/api/*` 代理到后端（带限流）
- `/ws/*` WebSocket 代理
- 静态资源缓存与 Gzip 压缩
- HTTPS 配置（注释状态）
- 安全响应头

---

## 8. 部署指南

### 8.1 Docker 部署（推荐）

```bash
# 构建并启动
docker-compose up -d

# 查看日志
docker-compose logs -f sophon-stream-web

# 停止
docker-compose down

# 带自定义配置
JWT_SECRET=your-secret LOG_LEVEL=DEBUG docker-compose up -d
```

### 8.2 原生部署

```bash
# 1. 构建
./scripts/build.sh

# 2. 部署
sudo ./scripts/deploy.sh sophon-stream-web-1.0.0-x86_64.tar.gz

# 3. 查看服务状态
sudo systemctl status sophon-stream-web

# 4. 查看日志
sudo journalctl -u sophon-stream-web -f

# 5. 回滚
sudo ./scripts/deploy.sh --rollback
```

### 8.3 交叉编译 (BM1684X/BM1688)

**前提条件**：
- 安装 aarch64 交叉编译工具链
- 准备目标平台的 sysroot（含交叉编译的依赖库）
- 安装 sophon-stream SDK

```bash
# 安装交叉编译工具链
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# 构建目标平台
./scripts/build.sh --target aarch64 --toolchain cmake/BM1684X-toolchain.cmake

# 在目标设备上部署
scp sophon-stream-web-1.0.0-aarch64.tar.gz root@192.168.1.10:/tmp/
ssh root@192.168.1.10 "cd /tmp && tar -xzf sophon-stream-web-1.0.0-aarch64.tar.gz"
```

---

## 9. 开发指南

### 9.1 开发环境搭建

```bash
# 克隆项目
git clone https://github.com/your-org/sophon-stream-web.git
cd sophon-stream-web

# 安装依赖
sudo ./scripts/install-deps.sh --drogon-source

# 启动开发环境
./scripts/dev.sh
```

### 9.2 代码规范

- **C++**: 遵循 Google C++ Style Guide，使用 clang-format 格式化
- **TypeScript/Vue**: ESLint + Prettier
- **提交信息**: Conventional Commits (`feat:`, `fix:`, `docs:` 等)
- **命名**:
  - C++ 类/结构体: PascalCase
  - C++ 函数/变量: camelCase
  - C++ 文件: PascalCase (类名匹配)
  - Vue 组件: PascalCase
  - CSS 类: kebab-case
  - API 路径: kebab-case

### 9.3 添加新插件

1. 实现插件接口（C++ 动态库）：

```cpp
// my_plugin.cpp
#include <string>
#include <nlohmann/json.hpp>

extern "C" {
    const char* plugin_name() { return "my-plugin"; }
    const char* plugin_version() { return "1.0.0"; }
    int initialize(const char* config_json) { return 0; }
    int process(const char* input, char** output) { return 0; }
    void destroy() {}
}
```

2. 编译为 .so：
```bash
g++ -shared -fPIC -o libmy-plugin.so my_plugin.cpp
```

3. 通过 Web 界面安装插件，或放入 `plugins/` 目录

### 9.4 添加新算法

1. 准备 BModel 模型文件（使用 TPU-MLIR 编译）
2. 通过 Web 界面注册算法：
   - 名称、类型、模型路径
   - 输入尺寸、类别列表
   - 配置 JSON（置信度阈值、NMS 阈值等）
3. 在任务创建时选择该算法

---

## 10. 常见问题

### Q: 首次启动时数据库在哪里创建？
A: 数据库文件默认在 `./data/sophon_stream.db`，首次启动会自动创建表和种子数据。

### Q: 忘记管理员密码怎么办？
A: 删除 `data/sophon_stream.db` 文件，重启服务会自动创建默认 admin 用户。

### Q: 如何启用 HTTPS？
A: 在 `config.json` 中配置 `ssl.cert` 和 `ssl.key` 路径，或使用 Nginx 反向代理处理 HTTPS。

### Q: 交叉编译时找不到依赖库怎么办？
A: 需要为目标平台交叉编译所有 C++ 依赖（drogon, sqlite3, openssl 等），并放置在 sysroot 中。

### Q: Docker 部署如何使用 TPU？
A: 需要在主机安装 Sophon SDK，并在 docker-compose.yml 中挂载设备：
```yaml
devices:
  - /dev/bm-sophon0:/dev/bm-sophon0
volumes:
  - /opt/sophon:/opt/sophon:ro
```

### Q: WebSocket 连接失败怎么办？
A: 检查 Nginx 配置中 `/ws/` 路径是否正确代理，确保 `proxy_read_timeout` 足够长。

### Q: 如何查看后端日志？
A: 日志文件在 `./logs/` 目录，或使用 `journalctl -u sophon-stream-web -f` 查看系统日志。

### Q: 前端开发时 API 请求跨域怎么办？
A: Vite 开发服务器已配置代理，前端请求 `/api/*` 会自动代理到后端。如需独立部署，配置 Nginx 反向代理。

---

## 11. 更新日志

详见 [CHANGELOG.md](CHANGELOG.md)

### v1.0.0 (2026-05-13)
- 首个正式版本发布
- 完整的后端 API 和前端界面
- 支持 BM1684X/BM1688 交叉编译
- Docker 容器化部署

---

## 12. 许可证

本项目基于 [Apache License 2.0](LICENSE) 许可证发布。

---

## 13. 致谢

- [Drogon](https://github.com/drogonframework/drogon) - 高性能 C++ Web 框架
- [Vue.js](https://vuejs.org/) - 渐进式 JavaScript 框架
- [Element Plus](https://element-plus.org/) - Vue 3 UI 组件库
- [ECharts](https://echarts.apache.org/) - 数据可视化图表库
- [jwt-cpp](https://github.com/Thalhammer/jwt-cpp) - C++ JWT 库
- [spdlog](https://github.com/gabime/spdlog) - C++ 日志库
- [nlohmann/json](https://github.com/nlohmann/json) - C++ JSON 库
- [Sophon-Stream](https://github.com/sophon-stream/sophon-stream) - 算能视频分析框架
