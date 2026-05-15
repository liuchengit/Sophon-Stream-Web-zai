import express from 'express'
import cors from 'cors'
import { WebSocketServer } from 'ws'
import http from 'http'
import path from 'path'
import { fileURLToPath } from 'url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const app = express()
const PORT = process.env.PORT || 3001

app.use(cors())
app.use(express.json())

// ─── Serve frontend static files ───
app.use(express.static(path.join(__dirname, 'frontend/dist')))

// ─── Helpers ───
function ok(data = null, message = 'success') {
  return { code: 0, message, data }
}

function mockToken(userId, role) {
  return Buffer.from(JSON.stringify({ id: userId, role, exp: Date.now() + 86400000 })).toString('base64')
}

function paginate(arr, page, pageSize) {
  const total = arr.length
  const start = (page - 1) * pageSize
  return { items: arr.slice(start, start + pageSize), total }
}

// ─── Mock Data ───
const now = new Date().toISOString()

let users = [
  { id: 1, username: 'admin', role: 'admin' },
  { id: 2, username: 'operator', role: 'operator' },
  { id: 3, username: 'viewer', role: 'viewer' },
]

let devices = [
  { id: 1, name: '前门摄像头', type: 'IPC', streamUrl: 'rtsp://192.168.1.101:554/stream1', protocol: 'RTSP', resolution: '1920x1080', fps: 25, codec: 'H.264', location: '1号楼前门', description: '主入口高清摄像头', status: 'online', createdAt: now, updatedAt: now },
  { id: 2, name: '停车场摄像头', type: 'IPC', streamUrl: 'rtsp://192.168.1.102:554/stream1', protocol: 'RTSP', resolution: '2560x1440', fps: 30, codec: 'H.265', location: '地下停车场B区', description: '停车场监控', status: 'online', createdAt: now, updatedAt: now },
  { id: 3, name: '仓库NVR', type: 'NVR', streamUrl: 'rtsp://192.168.1.200:554/ch01', protocol: 'RTSP', resolution: '1920x1080', fps: 25, codec: 'H.264', location: '2号仓库', description: '仓库NVR录像机', deviceId: 'NVR-001', status: 'online', createdAt: now, updatedAt: now },
  { id: 4, name: 'GB28181设备', type: 'GB28181', streamUrl: 'gb28181://34020000001320000001', protocol: 'GB28181', resolution: '1920x1080', fps: 25, codec: 'H.264', location: '3号楼大堂', description: '国标设备', deviceId: '34020000001320000001', status: 'offline', createdAt: now, updatedAt: now },
  { id: 5, name: '车间摄像头-01', type: 'IPC', streamUrl: 'rtsp://192.168.1.103:554/stream1', protocol: 'RTSP', resolution: '3840x2160', fps: 30, codec: 'H.265', location: '生产车间A区', description: '4K超清摄像头', status: 'online', createdAt: now, updatedAt: now },
  { id: 6, name: '测试视频文件', type: 'FILE', streamUrl: 'file:///data/videos/test.mp4', protocol: 'FILE', resolution: '1920x1080', fps: 25, codec: 'H.264', location: '本地', description: '测试用视频文件', status: 'error', createdAt: now, updatedAt: now },
]

let algorithms = [
  { id: 1, name: 'YOLOv5s 目标检测', type: 'detection', modelPath: '/models/yolov5s_bm1684x.bmodel', version: '1.2.0', inputSize: '640x640', classes: ['person', 'car', 'truck', 'bus', 'bicycle', 'motorbike'], status: 'active', description: '基于YOLOv5s的目标检测模型，优化部署于BM1684X', config: { confThreshold: 0.5, nmsThreshold: 0.4 }, createdAt: now, updatedAt: now },
  { id: 2, name: 'DeepSORT 多目标跟踪', type: 'tracking', modelPath: '/models/deepsort_bm1684x.bmodel', version: '1.0.0', inputSize: '256x128', classes: ['person', 'vehicle'], status: 'active', description: '多目标跟踪算法', config: { maxAge: 30, minHits: 3 }, createdAt: now, updatedAt: now },
  { id: 3, name: 'ResNet50 分类', type: 'classification', modelPath: '/models/resnet50_bm1684x.bmodel', version: '1.0.0', inputSize: '224x224', classes: ['normal', 'abnormal'], status: 'active', description: '图像分类模型', config: { topK: 5 }, createdAt: now, updatedAt: now },
  { id: 4, name: 'HRNet 姿态估计', type: 'pose', modelPath: '/models/hrnet_bm1684x.bmodel', version: '1.1.0', inputSize: '384x288', classes: ['person'], status: 'active', description: '人体姿态估计', config: { keypointThreshold: 0.3 }, createdAt: now, updatedAt: now },
  { id: 5, name: 'PaddleOCR 文字识别', type: 'ocr', modelPath: '/models/paddleocr_bm1684x.bmodel', version: '2.6.0', inputSize: '960x60', classes: [], status: 'inactive', description: 'OCR文字识别模型', config: { detLimitSideLen: 960 }, createdAt: now, updatedAt: now },
  { id: 6, name: 'U2Net 图像分割', type: 'segmentation', modelPath: '/models/u2net_bm1684x.bmodel', version: '1.0.0', inputSize: '320x320', classes: ['foreground', 'background'], status: 'active', description: '前景分割模型', config: { threshold: 0.5 }, createdAt: now, updatedAt: now },
  { id: 7, name: 'ArcFace 人脸识别', type: 'face', modelPath: '/models/arcface_bm1684x.bmodel', version: '1.0.0', inputSize: '112x112', classes: [], status: 'active', description: '人脸特征提取与识别', config: { similarityThreshold: 0.6 }, createdAt: now, updatedAt: now },
]

let tasks = [
  { id: 1, name: '前门人员检测', type: 'detection', description: '前门区域人员检测与跟踪', deviceId: 1, deviceName: '前门摄像头', algorithmIds: [1, 2], algorithmNames: ['YOLOv5s 目标检测', 'DeepSORT 多目标跟踪'], status: 'running', fps: 24.5, config: { confThreshold: 0.5, nmsThreshold: 0.4 }, roi: { type: 'polygon', points: [[100, 100], [500, 100], [500, 400], [100, 400]] }, graphId: 1, createdAt: now, updatedAt: now },
  { id: 2, name: '停车场车辆检测', type: 'detection', description: '停车场车辆检测与计数', deviceId: 2, deviceName: '停车场摄像头', algorithmIds: [1, 2], algorithmNames: ['YOLOv5s 目标检测', 'DeepSORT 多目标跟踪'], status: 'running', fps: 28.3, config: { confThreshold: 0.6 }, roi: { type: 'rect', x: 50, y: 50, width: 800, height: 600 }, graphId: 2, createdAt: now, updatedAt: now },
  { id: 3, name: '仓库安全监控', type: 'detection', description: '仓库区域安全监控', deviceId: 3, deviceName: '仓库NVR', algorithmIds: [1, 3], algorithmNames: ['YOLOv5s 目标检测', 'ResNet50 分类'], status: 'paused', fps: 0, config: { confThreshold: 0.5 }, roi: {}, graphId: 3, createdAt: now, updatedAt: now },
  { id: 4, name: '车间姿态分析', type: 'pose', description: '车间工人姿态分析', deviceId: 5, deviceName: '车间摄像头-01', algorithmIds: [4, 2], algorithmNames: ['HRNet 姿态估计', 'DeepSORT 多目标跟踪'], status: 'running', fps: 18.7, config: { keypointThreshold: 0.3 }, roi: { type: 'rect', x: 0, y: 0, width: 3840, height: 2160 }, graphId: 4, createdAt: now, updatedAt: now },
  { id: 5, name: '异常行为检测', type: 'classification', description: '异常行为检测与报警', deviceId: 1, deviceName: '前门摄像头', algorithmIds: [1, 3, 2], algorithmNames: ['YOLOv5s 目标检测', 'ResNet50 分类', 'DeepSORT 多目标跟踪'], status: 'stopped', fps: 0, config: {}, roi: {}, createdAt: now, updatedAt: now },
  { id: 6, name: '人脸识别门禁', type: 'face', description: '人脸识别门禁系统', deviceId: 2, deviceName: '停车场摄像头', algorithmIds: [7, 2], algorithmNames: ['ArcFace 人脸识别', 'DeepSORT 多目标跟踪'], status: 'error', fps: 0, config: { similarityThreshold: 0.6 }, roi: {}, graphId: 5, createdAt: now, updatedAt: now },
]

let alerts = [
  { id: 1, taskId: 1, taskName: '前门人员检测', type: 'intrusion', level: 'warning', message: '检测到非授权区域人员入侵', evidence: '/data/evidence/alert_001.jpg', acknowledged: false, createdAt: new Date(Date.now() - 3600000).toISOString() },
  { id: 2, taskId: 2, taskName: '停车场车辆检测', type: 'congestion', level: 'info', message: '停车场车位数低于20%', evidence: '/data/evidence/alert_002.jpg', acknowledged: false, createdAt: new Date(Date.now() - 7200000).toISOString() },
  { id: 3, taskId: 4, taskName: '车间姿态分析', type: 'fall', level: 'critical', message: '检测到人员跌倒', evidence: '/data/evidence/alert_003.jpg', acknowledged: false, createdAt: new Date(Date.now() - 1800000).toISOString() },
  { id: 4, taskId: 6, taskName: '人脸识别门禁', type: 'error', level: 'critical', message: '模型推理超时，任务异常', evidence: '', acknowledged: true, acknowledgedBy: 'admin', createdAt: new Date(Date.now() - 5400000).toISOString() },
  { id: 5, taskId: 1, taskName: '前门人员检测', type: 'crowd', level: 'warning', message: '人员密度超过阈值', evidence: '/data/evidence/alert_005.jpg', acknowledged: false, createdAt: new Date(Date.now() - 900000).toISOString() },
  { id: 6, taskId: 2, taskName: '停车场车辆检测', type: 'speeding', level: 'info', message: '检测到车辆超速行驶', evidence: '/data/evidence/alert_006.jpg', acknowledged: false, createdAt: new Date(Date.now() - 600000).toISOString() },
  { id: 7, taskId: 4, taskName: '车间姿态分析', type: 'posture', level: 'warning', message: '检测到异常工作姿态', evidence: '/data/evidence/alert_007.jpg', acknowledged: true, acknowledgedBy: 'operator', createdAt: new Date(Date.now() - 10800000).toISOString() },
]

let alertRules = [
  { id: 1, name: '人员入侵报警', type: 'intrusion', condition: { class: 'person', area: 'restricted' }, threshold: 1, debounce: 30, notificationConfig: { email: ['admin@example.com'], webhook: 'https://hooks.example.com/alert' }, enabled: true, createdAt: now, updatedAt: now },
  { id: 2, name: '人员密度报警', type: 'crowd', condition: { class: 'person', minCount: 10 }, threshold: 10, debounce: 60, notificationConfig: { email: ['admin@example.com'] }, enabled: true, createdAt: now, updatedAt: now },
  { id: 3, name: '跌倒检测报警', type: 'fall', condition: { action: 'fall' }, threshold: 1, debounce: 10, notificationConfig: { email: ['admin@example.com', 'safety@example.com'], webhook: 'https://hooks.example.com/fall' }, enabled: true, createdAt: now, updatedAt: now },
  { id: 4, name: '车辆超速报警', type: 'speeding', condition: { class: 'vehicle', maxSpeed: 30 }, threshold: 30, debounce: 15, notificationConfig: { email: ['security@example.com'] }, enabled: false, createdAt: now, updatedAt: now },
]

let plugins = [
  { id: 1, name: 'YOLOv5 检测插件', version: '1.2.0', type: 'element', description: 'YOLOv5目标检测推理插件', path: '/opt/sophon-stream/plugins/libyolov5_element.so', status: 'active', config: { batchSize: 4, useTPU: true }, createdAt: now, updatedAt: now },
  { id: 2, name: 'DeepSORT 跟踪插件', version: '1.0.0', type: 'element', description: 'DeepSORT多目标跟踪插件', path: '/opt/sophon-stream/plugins/libdeepsort_element.so', status: 'active', config: { maxAge: 30, minHits: 3 }, createdAt: now, updatedAt: now },
  { id: 3, name: 'OSD 绘制插件', version: '1.1.0', type: 'element', description: 'OSD绘制叠加插件', path: '/opt/sophon-stream/plugins/libosd_element.so', status: 'active', config: { fontSize: 24 }, createdAt: now, updatedAt: now },
  { id: 4, name: 'RTSP 推流插件', version: '1.0.0', type: 'sink', description: 'RTSP推流输出插件', path: '/opt/sophon-stream/plugins/librtsp_sink.so', status: 'inactive', config: { bitrate: 4000 }, createdAt: now, updatedAt: now },
]

let auditLogs = [
  { id: 1, userId: 1, username: 'admin', action: 'login', resource: 'auth', detail: '管理员登录系统', ip: '192.168.1.10', createdAt: new Date(Date.now() - 3600000).toISOString() },
  { id: 2, userId: 1, username: 'admin', action: 'create', resource: 'task', detail: '创建任务：前门人员检测', ip: '192.168.1.10', createdAt: new Date(Date.now() - 7200000).toISOString() },
  { id: 3, userId: 2, username: 'operator', action: 'start', resource: 'task', detail: '启动任务：停车场车辆检测', ip: '192.168.1.11', createdAt: new Date(Date.now() - 5400000).toISOString() },
  { id: 4, userId: 1, username: 'admin', action: 'update', resource: 'device', detail: '更新设备：前门摄像头', ip: '192.168.1.10', createdAt: new Date(Date.now() - 10800000).toISOString() },
  { id: 5, userId: 3, username: 'viewer', action: 'view', resource: 'dashboard', detail: '查看仪表盘', ip: '192.168.1.12', createdAt: new Date(Date.now() - 900000).toISOString() },
  { id: 6, userId: 1, username: 'admin', action: 'acknowledge', resource: 'alert', detail: '确认报警：模型推理超时', ip: '192.168.1.10', createdAt: new Date(Date.now() - 1800000).toISOString() },
]

let firmwareVersions = [
  { id: 1, version: 'v2.1.0', description: '优化TPU推理性能，修复内存泄漏', filePath: '/firmware/bm1684x_v2.1.0.bin', checksum: 'a1b2c3d4e5f6', status: 'available', createdAt: new Date(Date.now() - 86400000).toISOString() },
  { id: 2, version: 'v2.0.5', description: '新增BM1688支持', filePath: '/firmware/bm1684x_v2.0.5.bin', checksum: 'f6e5d4c3b2a1', status: 'installed', installedAt: new Date(Date.now() - 172800000).toISOString(), createdAt: new Date(Date.now() - 259200000).toISOString() },
]

let systemConfig = { logLevel: 'INFO', maxConnections: 100, autoRestart: true, monitorInterval: 2000, dataRetentionDays: 30, backupPath: '/data/backups', defaultVideoCodec: 'H.264', maxConcurrentTasks: 16 }

// ─── Auth Routes ───
app.post('/api/v1/auth/login', (req, res) => {
  const { username, password } = req.body
  if (!username || !password) return res.json({ code: 400, message: '用户名和密码不能为空', data: null })
  const user = users.find(u => u.username === username)
  if (!user) return res.json({ code: 401, message: '用户名或密码错误', data: null })
  res.json(ok({ token: mockToken(user.id, user.role), refreshToken: mockToken(user.id, user.role) + '_refresh', user: { id: user.id, username: user.username, role: user.role } }))
})
app.post('/api/v1/auth/register', (req, res) => {
  const { username, role } = req.body
  if (users.find(u => u.username === username)) return res.json({ code: 400, message: '用户名已存在', data: null })
  const u = { id: users.length + 1, username, role: role || 'viewer' }
  users.push(u)
  res.json(ok({ token: mockToken(u.id, u.role), refreshToken: mockToken(u.id, u.role) + '_refresh', user: { id: u.id, username: u.username, role: u.role } }))
})
app.post('/api/v1/auth/refresh', (req, res) => { const u = users[0]; res.json(ok({ token: mockToken(u.id, u.role), refreshToken: mockToken(u.id, u.role) + '_refresh', user: { id: u.id, username: u.username, role: u.role } })) })
app.put('/api/v1/auth/password', (req, res) => res.json(ok(null, '密码修改成功')))
app.get('/api/v1/auth/users', (req, res) => res.json(ok(users)))
app.get('/api/v1/auth/users/:id', (req, res) => res.json(ok(users.find(u => u.id === +req.params.id) || null)))
app.put('/api/v1/auth/users/:id', (req, res) => { const u = users.find(x => x.id === +req.params.id); if (u) Object.assign(u, req.body); res.json(ok(u)) })

// ─── Device Routes ───
app.get('/api/v1/devices', (req, res) => {
  let f = [...devices]; const { page = 1, pageSize = 20, status, type, keyword } = req.query
  if (status) f = f.filter(d => d.status === status); if (type) f = f.filter(d => d.type === type); if (keyword) f = f.filter(d => d.name.includes(keyword) || d.location.includes(keyword))
  res.json(ok(paginate(f, +page, +pageSize)))
})
app.get('/api/v1/devices/:id', (req, res) => res.json(ok(devices.find(d => d.id === +req.params.id) || null)))
app.post('/api/v1/devices', (req, res) => { const d = { id: devices.length + 1, ...req.body, status: 'offline', createdAt: new Date().toISOString(), updatedAt: new Date().toISOString() }; devices.push(d); res.json(ok(d)) })
app.put('/api/v1/devices/:id', (req, res) => { const d = devices.find(x => x.id === +req.params.id); if (d) Object.assign(d, req.body, { updatedAt: new Date().toISOString() }); res.json(ok(d)) })
app.delete('/api/v1/devices/:id', (req, res) => { devices = devices.filter(d => d.id !== +req.params.id); res.json(ok(null, '删除成功')) })
app.get('/api/v1/devices/:id/status', (req, res) => { const d = devices.find(x => x.id === +req.params.id); res.json(ok({ status: d?.status || 'offline', fps: d?.fps || 0, bitrate: Math.floor(Math.random() * 8000 + 2000) })) })
app.post('/api/v1/devices/:id/start', (req, res) => { const d = devices.find(x => x.id === +req.params.id); if (d) d.status = 'online'; res.json(ok(null, '启动成功')) })
app.post('/api/v1/devices/:id/stop', (req, res) => { const d = devices.find(x => x.id === +req.params.id); if (d) d.status = 'offline'; res.json(ok(null, '停止成功')) })

// ─── Algorithm Routes ───
app.get('/api/v1/algorithms', (req, res) => {
  let f = [...algorithms]; const { page = 1, pageSize = 20, type, status, keyword } = req.query
  if (type) f = f.filter(a => a.type === type); if (status) f = f.filter(a => a.status === status); if (keyword) f = f.filter(a => a.name.includes(keyword))
  res.json(ok(paginate(f, +page, +pageSize)))
})
app.get('/api/v1/algorithms/types', (req, res) => res.json(ok(['detection', 'tracking', 'classification', 'segmentation', 'pose', 'ocr', 'face'])))
app.get('/api/v1/algorithms/:id', (req, res) => res.json(ok(algorithms.find(a => a.id === +req.params.id) || null)))
app.post('/api/v1/algorithms', (req, res) => { const a = { id: algorithms.length + 1, ...req.body, status: 'inactive', createdAt: new Date().toISOString(), updatedAt: new Date().toISOString() }; algorithms.push(a); res.json(ok(a)) })
app.put('/api/v1/algorithms/:id', (req, res) => { const a = algorithms.find(x => x.id === +req.params.id); if (a) Object.assign(a, req.body, { updatedAt: new Date().toISOString() }); res.json(ok(a)) })
app.delete('/api/v1/algorithms/:id', (req, res) => { algorithms = algorithms.filter(a => a.id !== +req.params.id); res.json(ok(null, '删除成功')) })
app.post('/api/v1/algorithms/:id/validate', (req, res) => res.json(ok({ valid: true, message: '模型验证通过' })))

// ─── Task Routes ───
app.get('/api/v1/tasks', (req, res) => {
  let f = [...tasks]; const { page = 1, pageSize = 20, status, type, keyword } = req.query
  if (status) f = f.filter(t => t.status === status); if (type) f = f.filter(t => t.type === type); if (keyword) f = f.filter(t => t.name.includes(keyword))
  res.json(ok(paginate(f, +page, +pageSize)))
})
app.get('/api/v1/tasks/:id', (req, res) => res.json(ok(tasks.find(t => t.id === +req.params.id) || null)))
app.post('/api/v1/tasks', (req, res) => { const t = { id: tasks.length + 1, ...req.body, status: 'stopped', fps: 0, createdAt: new Date().toISOString(), updatedAt: new Date().toISOString() }; tasks.push(t); res.json(ok(t)) })
app.put('/api/v1/tasks/:id', (req, res) => { const t = tasks.find(x => x.id === +req.params.id); if (t) Object.assign(t, req.body, { updatedAt: new Date().toISOString() }); res.json(ok(t)) })
app.delete('/api/v1/tasks/:id', (req, res) => { tasks = tasks.filter(t => t.id !== +req.params.id); res.json(ok(null, '删除成功')) })
app.post('/api/v1/tasks/:id/start', (req, res) => { const t = tasks.find(x => x.id === +req.params.id); if (t) { t.status = 'running'; t.fps = +(Math.random() * 30 + 10).toFixed(1) }; res.json(ok(null, '任务已启动')) })
app.post('/api/v1/tasks/:id/stop', (req, res) => { const t = tasks.find(x => x.id === +req.params.id); if (t) { t.status = 'stopped'; t.fps = 0 }; res.json(ok(null, '任务已停止')) })
app.post('/api/v1/tasks/:id/pause', (req, res) => { const t = tasks.find(x => x.id === +req.params.id); if (t) { t.status = 'paused'; t.fps = 0 }; res.json(ok(null, '任务已暂停')) })
app.post('/api/v1/tasks/:id/resume', (req, res) => { const t = tasks.find(x => x.id === +req.params.id); if (t) { t.status = 'running'; t.fps = +(Math.random() * 30 + 10).toFixed(1) }; res.json(ok(null, '任务已恢复')) })
app.put('/api/v1/tasks/:id/config', (req, res) => { const t = tasks.find(x => x.id === +req.params.id); if (t) t.config = { ...t.config, ...req.body }; res.json(ok(null, '配置已更新')) })
app.put('/api/v1/tasks/:id/roi', (req, res) => { const t = tasks.find(x => x.id === +req.params.id); if (t) t.roi = req.body; res.json(ok(null, 'ROI已更新')) })
app.get('/api/v1/tasks/:id/metrics', (req, res) => res.json(ok({ fps: +(Math.random() * 30 + 10).toFixed(1), latency: +(Math.random() * 50 + 5).toFixed(1), throughput: +(Math.random() * 100 + 50).toFixed(0), detections: Math.floor(Math.random() * 20 + 1), timestamp: new Date().toISOString() })))

// ─── Monitor Routes ───
app.get('/api/v1/monitor/system', (req, res) => res.json(ok({ cpuUsage: +(Math.random() * 40 + 30).toFixed(1), memoryUsage: +(Math.random() * 30 + 40).toFixed(1), memoryTotal: 16384, memoryUsed: Math.floor(Math.random() * 4096 + 6144), tpuUsage: +(Math.random() * 50 + 20).toFixed(1), gpuUsage: +(Math.random() * 30 + 10).toFixed(1), uptime: Math.floor(Date.now() / 1000 - 86400 * 3), timestamp: new Date().toISOString() })))
app.get('/api/v1/monitor/tasks/:id', (req, res) => res.json(ok({ taskId: +req.params.id, taskName: '示例任务', fps: +(Math.random() * 30 + 10).toFixed(1), latency: +(Math.random() * 50 + 5).toFixed(1), throughput: +(Math.random() * 100 + 50).toFixed(0), detections: Math.floor(Math.random() * 20 + 1) })))
app.get('/api/v1/monitor/devices/:id', (req, res) => { const d = devices.find(x => x.id === +req.params.id); res.json(ok({ deviceId: +req.params.id, deviceName: d?.name || '未知', status: d?.status || 'offline', fps: d?.fps || 0, bitrate: Math.floor(Math.random() * 8000 + 2000) })) })

// ─── Dashboard Route ───
app.get('/api/v1/dashboard', (req, res) => {
  const rt = tasks.filter(t => t.status === 'running').length, od = devices.filter(d => d.status === 'online').length, ta = alerts.filter(a => !a.acknowledged).length
  const tsd = { running: tasks.filter(t => t.status === 'running').length, stopped: tasks.filter(t => t.status === 'stopped').length, paused: tasks.filter(t => t.status === 'paused').length, error: tasks.filter(t => t.status === 'error').length }
  const at = []; for (let i = 6; i >= 0; i--) { const d = new Date(); d.setDate(d.getDate() - i); at.push({ date: d.toISOString().split('T')[0], count: Math.floor(Math.random() * 20 + 5) }) }
  res.json(ok({ totalDevices: devices.length, onlineDevices: od, runningTasks: rt, todayAlerts: ta, systemMetrics: { cpuUsage: +(Math.random() * 40 + 30).toFixed(1), memoryUsage: +(Math.random() * 30 + 40).toFixed(1), memoryTotal: 16384, memoryUsed: Math.floor(Math.random() * 4096 + 6144), tpuUsage: +(Math.random() * 50 + 20).toFixed(1), gpuUsage: +(Math.random() * 30 + 10).toFixed(1), uptime: Math.floor(Date.now() / 1000 - 86400 * 3), timestamp: new Date().toISOString() }, recentAlerts: alerts.slice(0, 5), taskStatusDistribution: tsd, alertTrend: at }))
})

// ─── Alert Routes ───
app.get('/api/v1/alerts', (req, res) => { let f = [...alerts]; const { page = 1, pageSize = 20, level, acknowledged } = req.query; if (level) f = f.filter(a => a.level === level); if (acknowledged !== undefined) f = f.filter(a => a.acknowledged === (acknowledged === 'true')); res.json(ok(paginate(f, +page, +pageSize))) })
app.put('/api/v1/alerts/:id/acknowledge', (req, res) => { const a = alerts.find(x => x.id === +req.params.id); if (a) { a.acknowledged = true; a.acknowledgedBy = 'admin' }; res.json(ok(null, '已确认报警')) })
app.post('/api/v1/alerts/batch-acknowledge', (req, res) => { const { ids } = req.body; ids.forEach(id => { const a = alerts.find(x => x.id === id); if (a) { a.acknowledged = true; a.acknowledgedBy = 'admin' } }); res.json(ok(null, '批量确认成功')) })
app.get('/api/v1/alerts/stats', (req, res) => res.json(ok({ total: alerts.length, info: alerts.filter(a => a.level === 'info').length, warning: alerts.filter(a => a.level === 'warning').length, critical: alerts.filter(a => a.level === 'critical').length, unacknowledged: alerts.filter(a => !a.acknowledged).length })))
app.get('/api/v1/alerts/rules', (req, res) => { const { page = 1, pageSize = 20 } = req.query; res.json(ok(paginate(alertRules, +page, +pageSize))) })
app.post('/api/v1/alerts/rules', (req, res) => { const r = { id: alertRules.length + 1, ...req.body, createdAt: new Date().toISOString(), updatedAt: new Date().toISOString() }; alertRules.push(r); res.json(ok(r)) })
app.put('/api/v1/alerts/rules/:id', (req, res) => { const r = alertRules.find(x => x.id === +req.params.id); if (r) Object.assign(r, req.body, { updatedAt: new Date().toISOString() }); res.json(ok(r)) })
app.delete('/api/v1/alerts/rules/:id', (req, res) => { alertRules = alertRules.filter(r => r.id !== +req.params.id); res.json(ok(null, '删除成功')) })

// ─── Plugin Routes ───
app.get('/api/v1/plugins', (req, res) => { let f = [...plugins]; const { page = 1, pageSize = 20, status, type } = req.query; if (status) f = f.filter(p => p.status === status); if (type) f = f.filter(p => p.type === type); res.json(ok(paginate(f, +page, +pageSize))) })
app.get('/api/v1/plugins/:id', (req, res) => res.json(ok(plugins.find(p => p.id === +req.params.id) || null)))
app.post('/api/v1/plugins/install', (req, res) => { const p = { id: plugins.length + 1, name: '新插件', version: '1.0.0', type: 'element', description: '新安装的插件', path: req.body.path || '/plugins/new.so', status: 'inactive', config: {}, createdAt: new Date().toISOString(), updatedAt: new Date().toISOString() }; plugins.push(p); res.json(ok(p)) })
app.put('/api/v1/plugins/:id', (req, res) => { const p = plugins.find(x => x.id === +req.params.id); if (p) Object.assign(p, req.body, { updatedAt: new Date().toISOString() }); res.json(ok(p)) })
app.delete('/api/v1/plugins/:id', (req, res) => { plugins = plugins.filter(p => p.id !== +req.params.id); res.json(ok(null, '删除成功')) })
app.post('/api/v1/plugins/:id/activate', (req, res) => { const p = plugins.find(x => x.id === +req.params.id); if (p) p.status = 'active'; res.json(ok(null, '插件已激活')) })
app.post('/api/v1/plugins/:id/deactivate', (req, res) => { const p = plugins.find(x => x.id === +req.params.id); if (p) p.status = 'inactive'; res.json(ok(null, '插件已停用')) })

// ─── System Routes ───
app.get('/api/v1/system/info', (req, res) => res.json(ok({ version: '1.0.0', uptime: Math.floor(Date.now() / 1000 - 86400 * 3), cpuModel: 'Sophon BM1684X', cpuCores: 8, memoryTotal: 16384, tpuModel: 'BM1684X', tpuCount: 1, os: 'Linux 5.4.0-aarch64', hostname: 'sophon-edge-01' })))
app.get('/api/v1/system/config', (req, res) => res.json(ok(systemConfig)))
app.put('/api/v1/system/config', (req, res) => { Object.assign(systemConfig, req.body); res.json(ok(systemConfig, '配置已更新')) })
app.get('/api/v1/system/audit-logs', (req, res) => { let f = [...auditLogs]; const { page = 1, pageSize = 20, action, userId } = req.query; if (action) f = f.filter(l => l.action === action); if (userId) f = f.filter(l => l.userId === +userId); res.json(ok(paginate(f, +page, +pageSize))) })
app.post('/api/v1/system/backup', (req, res) => res.json(ok({ path: `/data/backups/backup_${Date.now()}.tar.gz` })))
app.post('/api/v1/system/restore', (req, res) => res.json(ok(null, '系统已恢复')))
app.get('/api/v1/system/firmware', (req, res) => res.json(ok(firmwareVersions)))
app.post('/api/v1/system/firmware/:id/install', (req, res) => { const fw = firmwareVersions.find(f => f.id === +req.params.id); if (fw) { fw.status = 'installed'; fw.installedAt = new Date().toISOString() }; res.json(ok(null, '固件安装成功')) })
app.post('/api/v1/system/reboot', (req, res) => res.json(ok(null, '系统将重启')))

// ─── Health Check ───
app.get('/api/v1/health', (req, res) => res.json(ok({ status: 'ok', version: '1.0.0' })))

// ─── SPA Fallback ───
app.get('*', (req, res) => {
  if (!req.path.startsWith('/api/')) {
    res.sendFile(path.join(__dirname, 'frontend/dist/index.html'))
  } else {
    res.json({ code: 404, message: '接口不存在', data: null })
  }
})

// ─── Create HTTP server ───
const server = http.createServer(app)

// ─── WebSocket for monitoring ───
const wss = new WebSocketServer({ server, path: '/ws/monitor' })
wss.on('connection', (ws) => {
  console.log('[WS] Monitor client connected')
  const interval = setInterval(() => {
    if (ws.readyState === ws.OPEN) {
      ws.send(JSON.stringify({
        type: 'system_metrics',
        data: {
          cpuUsage: +(Math.random() * 40 + 30).toFixed(1),
          memoryUsage: +(Math.random() * 30 + 40).toFixed(1),
          memoryTotal: 16384,
          memoryUsed: Math.floor(Math.random() * 4096 + 6144),
          tpuUsage: +(Math.random() * 50 + 20).toFixed(1),
          gpuUsage: +(Math.random() * 30 + 10).toFixed(1),
          uptime: Math.floor(Date.now() / 1000 - 86400 * 3),
          timestamp: new Date().toISOString(),
        },
      }))
    }
  }, 2000)
  ws.on('close', () => { console.log('[WS] Client disconnected'); clearInterval(interval) })
  ws.on('error', () => clearInterval(interval))
})

server.listen(PORT, '0.0.0.0', () => {
  console.log(`[Sophon-Stream Web] Server running on http://0.0.0.0:${PORT}`)
  console.log(`[Sophon-Stream Web] API: http://0.0.0.0:${PORT}/api/v1/`)
  console.log(`[Sophon-Stream Web] WebSocket: ws://0.0.0.0:${PORT}/ws/monitor`)
  console.log(`[Sophon-Stream Web] Login: admin / any password`)
})
