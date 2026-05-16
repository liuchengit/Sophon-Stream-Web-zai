# 用户指令记忆

本文件记录了用户的指令、偏好和教导，用于在未来的交互中提供参考。

## 格式

### 用户指令条目
用户指令条目应遵循以下格式：

[用户指令摘要]
- Date: [YYYY-MM-DD]
- Context: [提及的场景或时间]
- Instructions:
  - [用户教导或指示的内容，逐行描述]

### 项目知识条目
Agent 在任务执行过程中发现的条目应遵循以下格式：

[项目知识摘要]
- Date: [YYYY-MM-DD]
- Context: Agent 在执行 [具体任务描述] 时发现
- Category: [运维部署 | 构建方法 | 测试方法 | 排错调试 | 工作流协作 | 环境配置]
- Instructions:
  - [具体的知识点，逐行描述]

## 去重策略
- 添加新条目前，检查是否存在相似或相同的指令
- 若发现重复，跳过新条目或与已有条目合并
- 合并时，更新上下文或日期信息
- 这有助于避免冗余条目，保持记忆文件整洁

## 条目

[项目运行环境配置]
- Date: 2026-05-16
- Context: Agent 在部署 Sophon-Stream Web 管理系统时发现
- Category: 环境配置
- Instructions:
  - 系统环境：Debian GNU/Linux 12 (bookworm)
  - 前端端口：5173 (Vite dev server)
  - 后端端口：8080 (Drogon HTTP server)
  - 后端二进制文件需要 GLIBC_2.38 和 GLIBCXX_3.4.32，当前系统 GLIBC 版本较低
  - Sophon-Stream SDK 未安装在/opt/sophon-stream 目录
  - 数据目录：/workspace/data (SQLite 数据库)
  - 日志目录：/workspace/logs
  - 插件目录：/workspace/plugins
  - 默认账号：admin / admin123

[构建与运行方式]
- Date: 2026-05-16
- Context: Agent 在部署项目时查阅 README.md 和 scripts/dev.sh
- Category: 构建方法
- Instructions:
  - 开发模式：`./scripts/dev.sh` 同时启动前后端
  - 生产模式：编译后运行 `./dist/bin/sophon-stream-web --config config/config.json`
  - Docker 部署：`docker-compose up -d`
  - 前端构建：`cd frontend && npm run build`
  - 后端构建：使用 CMake，Debug 模式输出到 `build/debug`，Release 模式输出到 `build/release`
  - 交叉编译：使用 cmake/BM1684X-toolchain.cmake 或 cmake/BM1688-toolchain.cmake
