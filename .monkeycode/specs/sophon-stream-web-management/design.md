# Sophon-Stream Web 管理系统技术设计文档

## 文档信息

- **版本号**: 1.0.0
- **创建日期**: 2026-05-16
- **关联需求文档**: requirements.md
- **目标硬件**: Sophgo BM1684/BM1684X/BM1688

---

## 1. 系统架构

### 1.1 整体架构

系统采用前后端分离架构，前端通过反向代理与后端通信。

```
┌─────────────────────────────────────────────────────────────┐
│                      Client Browser                          │
│                      (Vue3 Application)                      │
└────────────────────┬────────────────────────────────────────┘
                     │ HTTPS
                     ▼
┌─────────────────────────────────────────────────────────────┐
│                  Frontend Dev Server                         │
│                  (Vite + Reverse Proxy)                      │
└────────────────────┬────────────────────────────────────────┘
                     │ /api/* → http://localhost:8080
                     ▼
┌─────────────────────────────────────────────────────────────┐
│                    Backend Server                            │
│              (Drogon C++ Web Framework)                      │
│  ┌─────────────┬─────────────┬─────────────┬─────────────┐ │
│  │   Auth      │   Config    │   Task      │   Monitor   │ │
│  │   Module    │   Module    │   Module    │   Module    │ │
│  └─────────────┴─────────────┴─────────────┴─────────────┘ │
│  ┌─────────────┬─────────────┬─────────────┬─────────────┐ │
│  │   Alarm     │  Protocol   │   System    │   Plugin    │ │
│  │   Module    │   Module    │   Module    │   Module    │ │
│  └─────────────┴─────────────┴─────────────┴─────────────┘ │
└────────────────────┬────────────────────────────────────────┘
                     │
        ┌────────────┼────────────┬────────────────┐
        │            │            │                │
        ▼            ▼            ▼                ▼
┌──────────────┐ ┌────────┐ ┌─────────┐ ┌──────────────────┐
│   SQLite     │ │Sophon- │ │  TPU/   │ │   External       │
│  Database    │ │ Stream │ │  CPU    │ │   Services       │
│  (数据持久化) │ │ Frame  │ │ 监控    │ │ (MQTT/GB28181)   │
└──────────────┘ └────────┘ └─────────┘ └──────────────────┘
```

### 1.2 技术栈

| 层次 | 技术选型 | 版本 | 说明 |
|------|----------|------|------|
| 前端框架 | Vue 3 | 3.4+ | 组合式 API |
| 构建工具 | Vite | 5.0+ | 快速开发和构建 |
| UI 组件库 | Element Plus | 2.4+ | 企业级 UI 组件 |
| 状态管理 | Pinia | 2.1+ | Vue3 状态管理 |
| 路由 | Vue Router | 4.2+ | 前端路由 |
| HTTP 客户端 | Axios | 1.6+ | HTTP 请求 |
| DAG 可视化 | Vue Flow / X6 | 1.3+ | 流程图编辑 |
| 图表 | ECharts | 5.4+ | 数据可视化 |
| 后端框架 | Drogon | 1.9+ | 高性能 C++ Web 框架 |
| 数据库 | SQLite | 3.40+ | 嵌入式数据库 |
| 认证 | JWT | - | Token 认证 |
| 加密 | OpenSSL | 3.0+ | 密码学和安全通信 |
| 协议 | libmosquitto | 2.0+ | MQTT 客户端 |
| 协议 | libgb28181 | - | GB28181 协议栈 |

### 1.3 目录结构

```
sophon-stream-web/
├── frontend/                      # 前端项目
│   ├── src/
│   │   ├── api/                   # API 接口定义
│   │   │   ├── auth.ts
│   │   │   ├── config.ts
│   │   │   ├── task.ts
│   │   │   ├── monitor.ts
│   │   │   ├── alarm.ts
│   │   │   ├── protocol.ts
│   │   │   └── system.ts
│   │   ├── components/            # 可复用组件
│   │   │   ├── common/            # 通用组件
│   │   │   │   ├── PageHeader.vue
│   │   │   │   ├── DataTable.vue
│   │   │   │   └── FileUploader.vue
│   │   │   ├── config/            # 配置相关组件
│   │   │   │   ├── ConfigEditor.vue
│   │   │   │   ├── ConfigHistory.vue
│   │   │   │   └── JsonValidator.vue
│   │   │   ├── dag/               # DAG 编排组件
│   │   │   │   ├── DagCanvas.vue
│   │   │   │   ├── DagNode.vue
│   │   │   │   ├── DagToolbar.vue
│   │   │   │   └── DagProperties.vue
│   │   │   ├── roi/               # ROI 绘制组件
│   │   │   │   ├── RoiDrawer.vue
│   │   │   │   ├── RoiLayer.vue
│   │   │   │   └── RoiList.vue
│   │   │   ├── monitor/           # 监控组件
│   │   │   │   ├── ResourceChart.vue
│   │   │   │   ├── TpuGauge.vue
│   │   │   │   └── StreamStatus.vue
│   │   │   └── alarm/             # 报警组件
│   │   │       ├── AlarmRuleForm.vue
│   │   │       ├── AlarmList.vue
│   │   │       └── AlarmHistory.vue
│   │   ├── views/                 # 页面视图
│   │   │   ├── Login.vue
│   │   │   ├── Dashboard.vue
│   │   │   ├── ConfigManage.vue
│   │   │   ├── TaskManage.vue
│   │   │   ├── DagDesigner.vue
│   │   │   ├── RoiDesigner.vue
│   │   │   ├── ResourceMonitor.vue
│   │   │   ├── AlarmRule.vue
│   │   │   ├── ProtocolConfig.vue
│   │   │   ├── SystemUpgrade.vue
│   │   │   ├── PluginMarket.vue
│   │   │   ├── UserManage.vue
│   │   │   └── LogView.vue
│   │   ├── stores/                # Pinia 状态管理
│   │   │   ├── user.ts
│   │   │   ├── config.ts
│   │   │   ├── task.ts
│   │   │   └── alarm.ts
│   │   ├── router/                # 路由配置
│   │   │   └── index.ts
│   │   ├── utils/                 # 工具函数
│   │   │   ├── request.ts         # Axios 封装
│   │   │   ├── auth.ts            # 认证工具
│   │   │   └── validator.ts       # 数据验证
│   │   ├── types/                 # TypeScript 类型定义
│   │   │   ├── api.ts
│   │   │   ├── config.ts
│   │   │   └── task.ts
│   │   ├── App.vue
│   │   └── main.ts
│   ├── public/
│   ├── package.json
│   ├── tsconfig.json
│   ├── vite.config.ts
│   └── index.html
│
├── backend/                       # 后端项目
│   ├── src/
│   │   ├── main.cc                # 入口程序
│   │   ├── config/                # 配置相关
│   │   │   ├── server_config.h
│   │   │   └── server_config.cc
│   │   ├── db/                    # 数据库相关
│   │   │   ├── Database.h
│   │   │   ├── Database.cc
│   │   │   ├── models/            # 数据模型
│   │   │   │   ├── User.h
│   │   │   │   ├── Role.h
│   │   │   │   ├── Config.h
│   │   │   │   ├── Task.h
│   │   │   │   ├── AlarmRule.h
│   │   │   │   └── SystemLog.h
│   │   │   └── migrations/        # 数据库迁移脚本
│   │   ├── controllers/           # 控制器
│   │   │   ├── AuthController.h
│   │   │   ├── AuthController.cc
│   │   │   ├── ConfigController.h
│   │   │   ├── ConfigController.cc
│   │   │   ├── TaskController.h
│   │   │   ├── TaskController.cc
│   │   │   ├── MonitorController.h
│   │   │   ├── MonitorController.cc
│   │   │   ├── AlarmController.h
│   │   │   ├── AlarmController.cc
│   │   │   ├── ProtocolController.h
│   │   │   ├── ProtocolController.cc
│   │   │   ├── SystemController.h
│   │   │   ├── SystemController.cc
│   │   │   └── PluginController.h
│   │   │   └── PluginController.cc
│   │   ├── services/              # 业务逻辑层
│   │   │   ├── AuthService.h
│   │   │   ├── AuthService.cc
│   │   │   ├── ConfigService.h
│   │   │   ├── ConfigService.cc
│   │   │   ├── TaskService.h
│   │   │   ├── TaskService.cc
│   │   │   ├── MonitorService.h
│   │   │   ├── MonitorService.cc
│   │   │   ├── AlarmService.h
│   │   │   ├── AlarmService.cc
│   │   │   ├── ProtocolService.h
│   │   │   ├── ProtocolService.cc
│   │   │   ├── SystemService.h
│   │   │   ├── SystemService.cc
│   │   │   ├── PluginService.h
│   │   │   └── PluginService.cc
│   │   ├── middleware/            # 中间件
│   │   │   ├── AuthMiddleware.h
│   │   │   ├── AuthMiddleware.cc
│   │   │   ├── CorsMiddleware.h
│   │   │   ├── CorsMiddleware.cc
│   │   │   └── RateLimitMiddleware.h
│   │   │   └── RateLimitMiddleware.cc
│   │   ├── utils/                 # 工具类
│   │   │   ├── JwtHelper.h
│   │   │   ├── JwtHelper.cc
│   │   │   ├── PasswordHasher.h
│   │   │   ├── PasswordHasher.cc
│   │   │   ├── JsonValidator.h
│   │   │   ├── JsonValidator.cc
│   │   │   ├── FileUploader.h
│   │   │   └── FileUploader.cc
│   │   ├── sophon/                # Sophon-Stream 集成
│   │   │   ├── StreamEngine.h
│   │   │   ├── StreamEngine.cc
│   │   │   ├── GraphManager.h
│   │   │   ├── GraphManager.cc
│   │   │   ├── ElementWrapper.h
│   │   │   └── ElementWrapper.cc
│   │   ├── protocol/              # 协议实现
│   │   │   ├── MqttClient.h
│   │   │   ├── MqttClient.cc
│   │   │   ├──Gb28181Client.h
│   │   │   └──Gb28181Client.cc
│   │   └── monitor/               # 监控模块
│   │       ├── ResourceMonitor.h
│   │       ├── ResourceMonitor.cc
│   │       ├── TpuMonitor.h
│   │       └── TpuMonitor.cc
│   ├── CMakeLists.txt
│   └── config.json                # 服务器配置
│
├── scripts/                       # 脚本工具
│   ├── build.sh                   # 构建脚本
│   ├── deploy.sh                  # 部署脚本
│   └── init_db.sql                # 数据库初始化脚本
│
├── docs/                          # 文档
│   ├── API.md                     # API 文档
│   ├── DEPLOY.md                  # 部署文档
│   └── DEVELOP.md                 # 开发文档
│
└── README.md
```

---

## 2. 模块设计

### 2.1 认证授权模块 (Auth Module)

#### 2.1.1 架构

```
┌─────────────────────────────────────────┐
│           AuthController                │
│  - login()                              │
│  - logout()                             │
│  - refreshToken()                       │
│  - getUserInfo()                        │
└────────────────┬────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────┐
│           AuthService                   │
│  - authenticate(username, password)     │
│  - generateJwtToken(user)               │
│  - validateJwtToken(token)              │
│  - checkPermission(user, resource)      │
└────────────────┬────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────┐
│           Database (SQLite)             │
│  - users 表                             │
│  - roles 表                             │
│  - permissions 表                       │
│  - user_roles 关联表                    │
│  - role_permissions 关联表              │
└─────────────────────────────────────────┘
```

#### 2.1.2 数据模型

**User 表**:
```sql
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    email VARCHAR(100),
    status INTEGER DEFAULT 1,  -- 1: active, 0: disabled
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);
```

**Role 表**:
```sql
CREATE TABLE roles (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name VARCHAR(50) UNIQUE NOT NULL,
    description TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);
```

**Permission 表**:
```sql
CREATE TABLE permissions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name VARCHAR(100) UNIQUE NOT NULL,
    resource VARCHAR(100) NOT NULL,
    action VARCHAR(50) NOT NULL,  -- create, read, update, delete
    description TEXT
);
```

#### 2.1.3 JWT Token 结构

```json
{
  "sub": "user_id",
  "username": "username",
  "roles": ["role1", "role2"],
  "permissions": ["perm1", "perm2"],
  "iat": 1234567890,
  "exp": 1234571490,
  "iss": "sophon-stream-web"
}
```

---

### 2.2 配置管理模块 (Config Module)

#### 2.2.1 架构

```
┌─────────────────────────────────────────┐
│         ConfigController                │
│  - listConfigs()                        │
│  - getConfig(id)                        │
│  - createConfig(config)                 │
│  - updateConfig(id, config)             │
│  - deleteConfig(id)                     │
│  - applyConfig(id)                      │
│  - getHistory(id)                       │
└────────────────┬────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────┐
│         ConfigService                   │
│  - validateJson(json)                   │
│  - saveConfig(config)                   │
│  - hotReload(config)                    │
│  - rollback(version)                    │
│  - getHistory(configId)                 │
└────────────────┬────────────────────────┘
                 │
        ┌────────┴────────┐
        │                 │
        ▼                 ▼
┌──────────────┐  ┌──────────────────┐
│  Database    │  │  Sophon-Stream   │
│  (持久化)    │  │  Engine API      │
└──────────────┘  └──────────────────┘
```

#### 2.2.2 数据模型

**configs 表**:
```sql
CREATE TABLE configs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name VARCHAR(100) NOT NULL,
    description TEXT,
    content TEXT NOT NULL,  -- JSON 配置内容
    status INTEGER DEFAULT 0,  -- 0: draft, 1: active, 2: archived
    version INTEGER DEFAULT 1,
    created_by INTEGER,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (created_by) REFERENCES users(id)
);
```

**config_history 表**:
```sql
CREATE TABLE config_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    config_id INTEGER NOT NULL,
    content TEXT NOT NULL,
    version INTEGER NOT NULL,
    change_type VARCHAR(20),  -- create, update, rollback
    changed_by INTEGER,
    changed_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (config_id) REFERENCES configs(id),
    FOREIGN KEY (changed_by) REFERENCES users(id)
);
```

#### 2.2.3 配置热更新流程

```
1. 用户调用 applyConfig API
2. ConfigService 验证配置有效性
3. 备份当前活跃配置
4. 调用 StreamEngine::reloadConfig()
5. 等待 Engine 确认
6. 如果成功：更新数据库状态
7. 如果失败：回滚到备份配置
```

---

### 2.3 DAG 任务编排模块 (DAG Module)

#### 2.3.1 架构

```
┌─────────────────────────────────────────┐
│          DagDesigner (Vue)              │
│  - 画布组件                             │
│  - 节点工具栏                           │
│  - 连线编辑器                           │
│  - 属性面板                             │
└────────────────┬────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────┐
│          DagTransformer                 │
│  - dagToJson(dag)                       │
│  - jsonToDag(json)                      │
│  - validateDag(dag)                     │
└────────────────┬────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────┐
│         TaskService                     │
│  - createTask(dag, config)              │
│  - startTask(taskId)                    │
│  - stopTask(taskId)                     │
│  - pauseTask(taskId)                    │
│  - resumeTask(taskId)                   │
└────────────────┬────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────┐
│         StreamEngine                    │
│  - addGraph(json)                       │
│  - start(graphId)                       │
│  - stop(graphId)                        │
│  - pushSourceData(...)                  │
└─────────────────────────────────────────┘
```

#### 2.3.2 DAG 数据结构

```typescript
interface DagNode {
  id: string;
  type: 'decode' | 'inference' | 'track' | 'osd' | 'encode' | 'distributor' | 'converger';
  position: { x: number; y: number };
  config: Record<string, any>;
}

interface DagEdge {
  id: string;
  source: string;
  target: string;
  sourcePort: number;
  targetPort: number;
}

interface Dag {
  nodes: DagNode[];
  edges: DagEdge[];
  metadata: {
    name: string;
    description: string;
    createdAt: string;
    updatedAt: string;
  };
}
```

#### 2.3.3 DAG 转 JSON 算法

```
function dagToJson(dag: Dag): string {
  const elements = dag.nodes.map(node => ({
    element_id: generateElementId(node),
    element_config: node.config,
    ports: inferPortsFromType(node.type)
  }));
  
  const connections = dag.edges.map(edge => ({
    src_element_id: edge.source,
    src_port: edge.sourcePort,
    dst_element_id: edge.target,
    dst_port: edge.targetPort
  }));
  
  return JSON.stringify({
    graph_id: 0,
    device_id: 0,
    graph_name: dag.metadata.name,
    elements,
    connections
  });
}
```

---

### 2.4 ROI 绘制模块 (ROI Module)

#### 2.4.1 架构

```
┌─────────────────────────────────────────┐
│          RoiDrawer (Vue)                │
│  - Canvas 画布                          │
│  - 矩形绘制工具                         │
│  - 多边形绘制工具                       │
│  - 顶点编辑模式                         │
└────────────────┬────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────┐
│          RoiManager                     │
│  - addRoi(region)                       │
│  - updateRoi(id, region)                │
│  - deleteRoi(id)                        │
│  - exportToJson()                       │
└────────────────┬────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────┐
│          ConfigService                  │
│  - injectRoiToConfig(config, roiJson)   │
└─────────────────────────────────────────┘
```

#### 2.4.2 ROI 数据结构

```typescript
interface RoiPoint {
  x: number;  // 归一化坐标 (0-1)
  y: number;
}

interface RoiRegion {
  id: string;
  name: string;
  type: 'rectangle' | 'polygon';
  points: RoiPoint[];
  color: string;
  description?: string;
}

interface RoiConfig {
  regions: RoiRegion[];
  videoWidth: number;
  videoHeight: number;
}
```

---

### 2.5 资源监控模块 (Monitor Module)

#### 2.5.1 架构

```
┌─────────────────────────────────────────┐
│        ResourceMonitor (C++)            │
│  - TpuMonitor                           │
│  - CpuMonitor                           │
│  - MemoryMonitor                        │
└────────────────┬────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────┐
│        MonitorService                   │
│  - getRealtimeMetrics()                 │
│  - getHistoryMetrics(timeRange)         │
│  - getGraphMetrics(graphId)             │
└────────────────┬────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────┐
│        MonitorController                │
│  - GET /api/monitor/tpu                 │
│  - GET /api/monitor/cpu                 │
│  - GET /api/monitor/memory              │
│  - GET /api/monitor/history             │
└─────────────────────────────────────────┘
```

#### 2.5.2 TPU 监控实现

```cpp
class TpuMonitor {
public:
    struct TpuMetrics {
        float utilization;      // 利用率 0-100%
        uint64_t memoryUsed;    // 已用内存 (bytes)
        uint64_t memoryTotal;   // 总内存 (bytes)
        float temperature;      // 温度
        uint64_t timestamp;     // 时间戳
    };
    
    TpuMetrics getMetrics(int deviceId = 0) {
        TpuMetrics metrics;
        
        // 调用 Sophon SDK API 获取 TPU 状态
        bm_dev_info_t info;
        bm_get_device_info(deviceId, &info);
        
        metrics.utilization = info.tpu_utilization;
        metrics.memoryUsed = info.memory_used;
        metrics.memoryTotal = info.memory_total;
        metrics.temperature = info.temperature;
        metrics.timestamp = getCurrentTimestamp();
        
        return metrics;
    }
};
```

---

### 2.6 报警规则引擎模块 (Alarm Module)

#### 2.6.1 架构

```
┌─────────────────────────────────────────┐
│         AlarmRuleEngine                 │
│  - evaluateRules(metrics)               │
│  - triggerAlarm(rule, event)            │
│  - sendNotification(alarm)              │
└────────────────┬────────────────────────┘
                 │
        ┌────────┴────────┐
        │                 │
        ▼                 ▼
┌──────────────┐  ┌──────────────────┐
│ AlarmService │  │ ProtocolService  │
│ (CRUD)       │  │ (MQTT/HTTP)      │
└──────────────┘  └──────────────────┘
```

#### 2.6.2 报警规则数据模型

```sql
CREATE TABLE alarm_rules (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name VARCHAR(100) NOT NULL,
    description TEXT,
    rule_type VARCHAR(50) NOT NULL,  -- resource, detection, intrusion
    condition_json TEXT NOT NULL,
    notification_methods TEXT,  -- JSON array: ["mqtt", "http", "web"]
    notification_config TEXT,   -- JSON config
    cooldown_seconds INTEGER DEFAULT 300,
    level VARCHAR(20) DEFAULT 'warning',  -- warning, critical, emergency
    enabled INTEGER DEFAULT 1,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);
```

#### 2.6.3 报警条件 JSON 结构

```json
{
  "type": "resource",
  "metric": "tpu_utilization",
  "operator": "greater_than",
  "threshold": 80,
  "duration": 60
}
```

---

### 2.7 协议集成模块 (Protocol Module)

#### 2.7.1 MQTT 客户端

```cpp
class MqttClient {
public:
    MqttClient(const std::string& host, int port);
    
    bool connect(const std::string& clientId, 
                 const std::string& username,
                 const std::string& password);
    
    void subscribe(const std::string& topic, 
                   std::function<void(const std::string&)> callback);
    
    void publish(const std::string& topic, 
                 const std::string& payload,
                 int qos = 0);
    
    void disconnect();
    
private:
    mosquitto* mMosquitto;
    std::string mHost;
    int mPort;
};
```

#### 2.7.2 GB28181 客户端

```cpp
class Gb28181Client {
public:
    struct Config {
        std::string sipServerIp;
        int sipServerPort;
        std::string deviceId;
        std::string password;
        std::string localIp;
        int localPort;
    };
    
    bool registerToServer(const Config& config);
    bool startPushStream(const std::string& channelId);
    bool stopPushStream(const std::string& channelId);
    void unregister();
    
private:
    Config mConfig;
    bool mRegistered;
};
```

---

### 2.8 系统升级模块 (System Module)

#### 2.8.1 升级流程

```
1. 用户上传升级包 (.tar.gz 或 .deb)
2. SystemService 验证包签名
3. 创建当前版本快照
4. 解压升级包
5. 备份旧版本文件
6. 复制新版本文件
7. 执行升级后脚本
8. 重启服务
9. 验证升级成功
10. 如果失败，回滚到快照
```

#### 2.8.2 数据模型

```sql
CREATE TABLE system_versions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    version VARCHAR(50) NOT NULL,
    build_date DATETIME,
    changelog TEXT,
    is_current INTEGER DEFAULT 0,
    backup_path TEXT,
    installed_at DATETIME DEFAULT CURRENT_TIMESTAMP
);
```

---

### 2.9 插件市场模块 (Plugin Module)

#### 2.9.1 架构

```
┌─────────────────────────────────────────┐
│         PluginMarket (Vue)              │
│  - 插件列表                             │
│  - 插件详情                             │
│  - 搜索和筛选                           │
│  - 安装/卸载操作                        │
└────────────────┬────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────┐
│         PluginService                   │
│  - listPlugins()                        │
│  - getPlugin(id)                        │
│  - installPlugin(file)                  │
│  - uninstallPlugin(id)                  │
│  - uploadPlugin(file)                   │
│  - validatePlugin(file)                 │
└────────────────┬────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────┐
│         Plugin Registry                 │
│  - 本地插件目录                         │
│  - 插件元数据索引                       │
└─────────────────────────────────────────┘
```

#### 2.9.2 插件元数据

```json
{
  "name": "yolov8-det",
  "version": "1.0.0",
  "description": "YOLOv8 目标检测插件",
  "author": "Sophgo",
  "type": "inference",
  "compatibility": ["BM1684", "BM1684X", "BM1688"],
  "dependencies": [],
  "config_schema": {},
  "download_url": "...",
  "checksum": "sha256:..."
}
```

---

## 3. API 接口设计

### 3.1 认证接口

| 方法 | 路径 | 描述 | 认证 |
|------|------|------|------|
| POST | /api/auth/login | 用户登录 | 否 |
| POST | /api/auth/logout | 用户登出 | 是 |
| POST | /api/auth/refresh | 刷新 Token | 是 |
| GET | /api/auth/me | 获取当前用户信息 | 是 |

### 3.2 配置接口

| 方法 | 路径 | 描述 | 认证 |
|------|------|------|------|
| GET | /api/configs | 获取配置列表 | 是 |
| GET | /api/configs/:id | 获取单个配置 | 是 |
| POST | /api/configs | 创建配置 | 是 |
| PUT | /api/configs/:id | 更新配置 | 是 |
| DELETE | /api/configs/:id | 删除配置 | 是 |
| POST | /api/configs/:id/apply | 应用配置 | 是 |
| GET | /api/configs/:id/history | 获取历史版本 | 是 |
| POST | /api/configs/:id/rollback | 回滚到指定版本 | 是 |

### 3.3 任务接口

| 方法 | 路径 | 描述 | 认证 |
|------|------|------|------|
| GET | /api/tasks | 获取任务列表 | 是 |
| GET | /api/tasks/:id | 获取任务详情 | 是 |
| POST | /api/tasks | 创建任务 | 是 |
| POST | /api/tasks/:id/start | 启动任务 | 是 |
| POST | /api/tasks/:id/stop | 停止任务 | 是 |
| POST | /api/tasks/:id/pause | 暂停任务 | 是 |
| POST | /api/tasks/:id/resume | 恢复任务 | 是 |
| DELETE | /api/tasks/:id | 删除任务 | 是 |

### 3.4 监控接口

| 方法 | 路径 | 描述 | 认证 |
|------|------|------|------|
| GET | /api/monitor/tpu | 获取 TPU 状态 | 是 |
| GET | /api/monitor/cpu | 获取 CPU 状态 | 是 |
| GET | /api/monitor/memory | 获取内存状态 | 是 |
| GET | /api/monitor/history | 获取历史数据 | 是 |
| GET | /api/monitor/graphs/:id | 获取 Graph 资源占用 | 是 |

### 3.5 报警接口

| 方法 | 路径 | 描述 | 认证 |
|------|------|------|------|
| GET | /api/alarms/rules | 获取报警规则列表 | 是 |
| POST | /api/alarms/rules | 创建报警规则 | 是 |
| PUT | /api/alarms/rules/:id | 更新报警规则 | 是 |
| DELETE | /api/alarms/rules/:id | 删除报警规则 | 是 |
| POST | /api/alarms/rules/:id/toggle | 启用/禁用规则 | 是 |
| GET | /api/alarms/events | 获取报警事件列表 | 是 |
| GET | /api/alarms/history | 获取报警历史 | 是 |

### 3.6 协议接口

| 方法 | 路径 | 描述 | 认证 |
|------|------|------|------|
| GET | /api/protocols/mqtt/config | 获取 MQTT 配置 | 是 |
| PUT | /api/protocols/mqtt/config | 更新 MQTT 配置 | 是 |
| GET | /api/protocols/gb28181/config | 获取 GB28181 配置 | 是 |
| PUT | /api/protocols/gb28181/config | 更新 GB28181 配置 | 是 |
| POST | /api/protocols/gb28181/register | 注册到 GB28181 平台 | 是 |
| POST | /api/protocols/gb28181/unregister | 从平台注销 | 是 |

### 3.7 系统接口

| 方法 | 路径 | 描述 | 认证 |
|------|------|------|------|
| GET | /api/system/info | 获取系统信息 | 是 |
| POST | /api/system/upgrade | 系统升级 | 是 |
| GET | /api/system/versions | 获取版本列表 | 是 |
| POST | /api/system/versions/:id/rollback | 版本回滚 | 是 |
| GET | /api/system/logs | 获取系统日志 | 是 |

### 3.8 插件接口

| 方法 | 路径 | 描述 | 认证 |
|------|------|------|------|
| GET | /api/plugins | 获取插件列表 | 是 |
| GET | /api/plugins/:id | 获取插件详情 | 是 |
| POST | /api/plugins/:id/install | 安装插件 | 是 |
| POST | /api/plugins/:id/uninstall | 卸载插件 | 是 |
| POST | /api/plugins/upload | 上传插件 | 是 |

### 3.9 用户接口

| 方法 | 路径 | 描述 | 认证 |
|------|------|------|------|
| GET | /api/users | 获取用户列表 | 是 (Admin) |
| POST | /api/users | 创建用户 | 是 (Admin) |
| PUT | /api/users/:id | 更新用户 | 是 (Admin) |
| DELETE | /api/users/:id | 删除用户 | 是 (Admin) |
| GET | /api/roles | 获取角色列表 | 是 (Admin) |
| POST | /api/roles | 创建角色 | 是 (Admin) |

---

## 4. 数据库设计

### 4.1 ER 图

```
┌─────────────┐       ┌─────────────┐
│    users    │       │    roles    │
├─────────────┤       ├─────────────┤
│ id          │◄──────│ id          │
│ username    │       │ name        │
│ password    │       │ description │
│ email       │       └─────────────┘
│ status      │              ▲
│ created_at  │              │
│ updated_at  │       ┌─────────────┐
└─────────────┘       │   perms     │
       │              ├─────────────┤
       │         ┌────│ id          │
       │         │    │ name        │
       ▼         ▼    │ resource    │
┌─────────────┐       │ action      │
│ user_roles  │       └─────────────┘
├─────────────┤
│ user_id     │
│ role_id     │
└─────────────┘
```

### 4.2 完整表结构

```sql
-- 用户表
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    email VARCHAR(100),
    status INTEGER DEFAULT 1,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 角色表
CREATE TABLE roles (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name VARCHAR(50) UNIQUE NOT NULL,
    description TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 权限表
CREATE TABLE permissions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name VARCHAR(100) UNIQUE NOT NULL,
    resource VARCHAR(100) NOT NULL,
    action VARCHAR(50) NOT NULL,
    description TEXT
);

-- 用户角色关联表
CREATE TABLE user_roles (
    user_id INTEGER NOT NULL,
    role_id INTEGER NOT NULL,
    PRIMARY KEY (user_id, role_id),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (role_id) REFERENCES roles(id) ON DELETE CASCADE
);

-- 角色权限关联表
CREATE TABLE role_permissions (
    role_id INTEGER NOT NULL,
    permission_id INTEGER NOT NULL,
    PRIMARY KEY (role_id, permission_id),
    FOREIGN KEY (role_id) REFERENCES roles(id) ON DELETE CASCADE,
    FOREIGN KEY (permission_id) REFERENCES permissions(id) ON DELETE CASCADE
);

-- 配置表
CREATE TABLE configs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name VARCHAR(100) NOT NULL,
    description TEXT,
    content TEXT NOT NULL,
    status INTEGER DEFAULT 0,
    version INTEGER DEFAULT 1,
    created_by INTEGER,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (created_by) REFERENCES users(id)
);

-- 配置历史表
CREATE TABLE config_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    config_id INTEGER NOT NULL,
    content TEXT NOT NULL,
    version INTEGER NOT NULL,
    change_type VARCHAR(20),
    changed_by INTEGER,
    changed_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (config_id) REFERENCES configs(id),
    FOREIGN KEY (changed_by) REFERENCES users(id)
);

-- 任务表
CREATE TABLE tasks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name VARCHAR(100) NOT NULL,
    dag_json TEXT,
    config_id INTEGER,
    status VARCHAR(20) DEFAULT 'stopped',
    graph_id INTEGER,
    created_by INTEGER,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (config_id) REFERENCES configs(id),
    FOREIGN KEY (created_by) REFERENCES users(id)
);

-- 报警规则表
CREATE TABLE alarm_rules (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name VARCHAR(100) NOT NULL,
    description TEXT,
    rule_type VARCHAR(50) NOT NULL,
    condition_json TEXT NOT NULL,
    notification_methods TEXT,
    notification_config TEXT,
    cooldown_seconds INTEGER DEFAULT 300,
    level VARCHAR(20) DEFAULT 'warning',
    enabled INTEGER DEFAULT 1,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 报警事件表
CREATE TABLE alarm_events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    rule_id INTEGER NOT NULL,
    message TEXT NOT NULL,
    level VARCHAR(20),
    triggered_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    acknowledged INTEGER DEFAULT 0,
    acknowledged_by INTEGER,
    acknowledged_at DATETIME,
    FOREIGN KEY (rule_id) REFERENCES alarm_rules(id)
);

-- 系统版本表
CREATE TABLE system_versions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    version VARCHAR(50) NOT NULL,
    build_date DATETIME,
    changelog TEXT,
    is_current INTEGER DEFAULT 0,
    backup_path TEXT,
    installed_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 操作日志表
CREATE TABLE operation_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER,
    action VARCHAR(100) NOT NULL,
    resource VARCHAR(100),
    resource_id INTEGER,
    details TEXT,
    ip_address VARCHAR(45),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id)
);

-- 系统日志表
CREATE TABLE system_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    level VARCHAR(20) NOT NULL,
    module VARCHAR(50),
    message TEXT NOT NULL,
    stack_trace TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 插件表
CREATE TABLE plugins (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name VARCHAR(100) NOT NULL,
    version VARCHAR(50) NOT NULL,
    description TEXT,
    author VARCHAR(100),
    type VARCHAR(50),
    path VARCHAR(255) NOT NULL,
    installed INTEGER DEFAULT 1,
    installed_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);
```

---

## 5. 前端设计

### 5.1 页面路由

```typescript
const routes = [
  { path: '/login', component: Login },
  { 
    path: '/', 
    component: Layout,
    children: [
      { path: '', redirect: '/dashboard' },
      { path: 'dashboard', component: Dashboard },
      { path: 'configs', component: ConfigManage },
      { path: 'tasks', component: TaskManage },
      { path: 'dag-designer', component: DagDesigner },
      { path: 'roi-designer', component: RoiDesigner },
      { path: 'monitor', component: ResourceMonitor },
      { path: 'alarms', component: AlarmRule },
      { path: 'protocols', component: ProtocolConfig },
      { path: 'system', component: SystemUpgrade },
      { path: 'plugins', component: PluginMarket },
      { path: 'users', component: UserManage },
      { path: 'logs', component: LogView }
    ]
  }
];
```

### 5.2 状态管理

```typescript
// stores/user.ts
interface UserState {
  token: string | null;
  userInfo: UserInfo | null;
  permissions: string[];
}

// stores/config.ts
interface ConfigState {
  configs: Config[];
  activeConfig: Config | null;
  loading: boolean;
}

// stores/task.ts
interface TaskState {
  tasks: Task[];
  runningTasks: number;
  maxTasks: number;
}
```

### 5.3 组件设计

**DagCanvas 组件**:
```vue
<template>
  <div class="dag-canvas">
    <VueFlow
      v-model:nodes="nodes"
      v-model:edges="edges"
      @connect="onConnect"
      @drag-stop="onDragStop"
    />
  </div>
</template>

<script setup lang="ts">
import { VueFlow } from '@vue-flow/core';
import { DagNode, DagEdge } from '@/types';

const nodes = ref<DagNode[]>([]);
const edges = ref<DagEdge[]>([]);

const onConnect = (params: Connection) => {
  // 验证并添加连线
};

const onDragStop = (event: NodeDragEvent) => {
  // 更新节点位置
};
</script>
```

---

## 6. 安全设计

### 6.1 认证安全

- JWT Token 有效期：2 小时
- Refresh Token 有效期：7 天
- 密码使用 bcrypt 加密，cost=12
- 登录失败 5 次锁定账户 30 分钟

### 6.2 通信安全

- 所有 API 使用 HTTPS
- CORS 限制来源域名
- 敏感操作需二次验证

### 6.3 数据安全

- SQL 参数化查询防止注入
- 输入数据验证和过滤
- 文件上传类型和大小限制

---

## 7. 部署设计

### 7.1 部署架构

```
┌─────────────────────────────────────┐
│         Nginx (可选)                │
│         反向代理/负载均衡           │
└────────────────┬────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────┐
│         Sophon-Stream Web           │
│         (Frontend + Backend)        │
├─────────────────────────────────────┤
│         SQLite Database             │
├─────────────────────────────────────┤
│         Sophon-Stream Engine        │
│         (C++ Framework)             │
└────────────────┬────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────┐
│         Sophgo TPU (BM1684X)        │
└─────────────────────────────────────┘
```

### 7.2 启动脚本

```bash
#!/bin/bash
# start.sh

# 启动后端
cd /opt/sophon-stream-web/backend
./sophon-stream-web-server &
BACKEND_PID=$!

# 启动前端 (生产环境使用 Nginx 托管静态文件)
cd /opt/sophon-stream-web/frontend
npm run preview &
FRONTEND_PID=$!

# 等待退出
trap "kill $BACKEND_PID $FRONTEND_PID" EXIT
wait
```

---

## 8. 测试策略

### 8.1 单元测试

- 后端：使用 Google Test
- 前端：使用 Vitest + Vue Test Utils
- 覆盖率目标：>80%

### 8.2 集成测试

- API 接口测试
- 数据库操作测试
- Sophon-Stream 集成测试

### 8.3 端到端测试

- 使用 Playwright
- 覆盖核心业务流程

---

## 附录 A：修订历史

| 版本 | 日期 | 修订内容 | 作者 |
|------|------|----------|------|
| 1.0.0 | 2026-05-16 | 初始版本 | AI Assistant |
