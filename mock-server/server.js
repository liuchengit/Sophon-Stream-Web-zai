import express from 'express'
import cors from 'cors'
import { WebSocketServer } from 'ws'
import http from 'http'

const app = express()
const PORT = 8080

app.use(cors())
app.use(express.json())

// ─── Simple JWT mock ───
function mockToken(userId, role) {
  return Buffer.from(JSON.stringify({ id: userId, role, exp: Date.now() + 86400000 })).toString('base64')
}

function verifyAuth(req, res, next) {
  const auth = req.headers.authorization
  if (!auth || !auth.startsWith('Bearer ')) {
    return res.json({ code: 401, message: '未授权', data: null })
  }
  next()
}

// ─── Success helper ───
function ok(data = null, message = 'success') {
  return { code: 0, message, data }
}

// ─── Mock Data Store ───
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

let systemConfig = {
  logLevel: 'INFO',
  maxConnections: 100,
  autoRestart: true,
  monitorInterval: 2000,
  dataRetentionDays: 30,
  backupPath: '/data/backups',
  defaultVideoCodec: 'H.264',
  maxConcurrentTasks: 16,
}

// ─── Auth Routes ───
app.post('/api/v1/auth/login', (req, res) => {
  const { username, password } = req.body
  if (!username || !password) {
    return res.json({ code: 400, message: '用户名和密码不能为空', data: null })
  }
  const user = users.find(u => u.username === username)
  if (!user) {
    return res.json({ code: 401, message: '用户名或密码错误', data: null })
  }
  res.json(ok({
    token: mockToken(user.id, user.role),
    refreshToken: mockToken(user.id, user.role) + '_refresh',
    user: { id: user.id, username: user.username, role: user.role },
  }))
})

app.post('/api/v1/auth/register', (req, res) => {
  const { username, password, role } = req.body
  if (users.find(u => u.username === username)) {
    return res.json({ code: 400, message: '用户名已存在', data: null })
  }
  const newUser = { id: users.length + 1, username, role: role || 'viewer' }
  users.push(newUser)
  res.json(ok({
    token: mockToken(newUser.id, newUser.role),
    refreshToken: mockToken(newUser.id, newUser.role) + '_refresh',
    user: { id: newUser.id, username: newUser.username, role: newUser.role },
  }))
})

app.post('/api/v1/auth/refresh', (req, res) => {
  const user = users[0]
  res.json(ok({
    token: mockToken(user.id, user.role),
    refreshToken: mockToken(user.id, user.role) + '_refresh',
    user: { id: user.id, username: user.username, role: user.role },
  }))
})

app.put('/api/v1/auth/password', verifyAuth, (req, res) => {
  res.json(ok(null, '密码修改成功'))
})

app.get('/api/v1/auth/users', verifyAuth, (req, res) => {
  res.json(ok(users))
})

app.get('/api/v1/auth/users/:id', verifyAuth, (req, res) => {
  const user = users.find(u => u.id === +req.params.id)
  res.json(ok(user || null))
})

app.put('/api/v1/auth/users/:id', verifyAuth, (req, res) => {
  const user = users.find(u => u.id === +req.params.id)
  if (user) Object.assign(user, req.body)
  res.json(ok(user))
})

// ─── Device Routes ───
app.get('/api/v1/devices', verifyAuth, (req, res) => {
  const { page = 1, pageSize = 20, status, type, keyword } = req.query
  let filtered = [...devices]
  if (status) filtered = filtered.filter(d => d.status === status)
  if (type) filtered = filtered.filter(d => d.type === type)
  if (keyword) filtered = filtered.filter(d => d.name.includes(keyword) || d.location.includes(keyword))
  const total = filtered.length
  const start = (page - 1) * pageSize
  const items = filtered.slice(start, start + +pageSize)
  res.json(ok({ items, total }))
})

app.get('/api/v1/devices/:id', verifyAuth, (req, res) => {
  const device = devices.find(d => d.id === +req.params.id)
  res.json(ok(device || null))
})

app.post('/api/v1/devices', verifyAuth, (req, res) => {
  const newDevice = { id: devices.length + 1, ...req.body, status: 'offline', createdAt: new Date().toISOString(), updatedAt: new Date().toISOString() }
  devices.push(newDevice)
  res.json(ok(newDevice))
})

app.put('/api/v1/devices/:id', verifyAuth, (req, res) => {
  const device = devices.find(d => d.id === +req.params.id)
  if (device) Object.assign(device, req.body, { updatedAt: new Date().toISOString() })
  res.json(ok(device))
})

app.delete('/api/v1/devices/:id', verifyAuth, (req, res) => {
  devices = devices.filter(d => d.id !== +req.params.id)
  res.json(ok(null, '删除成功'))
})

app.get('/api/v1/devices/:id/status', verifyAuth, (req, res) => {
  const device = devices.find(d => d.id === +req.params.id)
  res.json(ok({ status: device?.status || 'offline', fps: device?.fps || 0, bitrate: Math.random() * 8000 + 2000 }))
})

app.post('/api/v1/devices/:id/start', verifyAuth, (req, res) => {
  const device = devices.find(d => d.id === +req.params.id)
  if (device) device.status = 'online'
  res.json(ok(null, '启动成功'))
})

app.post('/api/v1/devices/:id/stop', verifyAuth, (req, res) => {
  const device = devices.find(d => d.id === +req.params.id)
  if (device) device.status = 'offline'
  res.json(ok(null, '停止成功'))
})

// ─── Algorithm Routes ───
app.get('/api/v1/algorithms', verifyAuth, (req, res) => {
  const { page = 1, pageSize = 20, type, status, keyword } = req.query
  let filtered = [...algorithms]
  if (type) filtered = filtered.filter(a => a.type === type)
  if (status) filtered = filtered.filter(a => a.status === status)
  if (keyword) filtered = filtered.filter(a => a.name.includes(keyword))
  const total = filtered.length
  const start = (page - 1) * pageSize
  const items = filtered.slice(start, start + +pageSize)
  res.json(ok({ items, total }))
})

app.get('/api/v1/algorithms/types', verifyAuth, (req, res) => {
  res.json(ok(['detection', 'tracking', 'classification', 'segmentation', 'pose', 'ocr', 'face']))
})

app.get('/api/v1/algorithms/:id', verifyAuth, (req, res) => {
  const algo = algorithms.find(a => a.id === +req.params.id)
  res.json(ok(algo || null))
})

app.post('/api/v1/algorithms', verifyAuth, (req, res) => {
  const newAlgo = { id: algorithms.length + 1, ...req.body, status: 'inactive', createdAt: new Date().toISOString(), updatedAt: new Date().toISOString() }
  algorithms.push(newAlgo)
  res.json(ok(newAlgo))
})

app.put('/api/v1/algorithms/:id', verifyAuth, (req, res) => {
  const algo = algorithms.find(a => a.id === +req.params.id)
  if (algo) Object.assign(algo, req.body, { updatedAt: new Date().toISOString() })
  res.json(ok(algo))
})

app.delete('/api/v1/algorithms/:id', verifyAuth, (req, res) => {
  algorithms = algorithms.filter(a => a.id !== +req.params.id)
  res.json(ok(null, '删除成功'))
})

app.post('/api/v1/algorithms/:id/validate', verifyAuth, (req, res) => {
  res.json(ok({ valid: true, message: '模型验证通过' }))
})

// ─── Task Routes ───
app.get('/api/v1/tasks', verifyAuth, (req, res) => {
  const { page = 1, pageSize = 20, status, type, keyword } = req.query
  let filtered = [...tasks]
  if (status) filtered = filtered.filter(t => t.status === status)
  if (type) filtered = filtered.filter(t => t.type === type)
  if (keyword) filtered = filtered.filter(t => t.name.includes(keyword))
  const total = filtered.length
  const start = (page - 1) * pageSize
  const items = filtered.slice(start, start + +pageSize)
  res.json(ok({ items, total }))
})

app.get('/api/v1/tasks/:id', verifyAuth, (req, res) => {
  const task = tasks.find(t => t.id === +req.params.id)
  res.json(ok(task || null))
})

app.post('/api/v1/tasks', verifyAuth, (req, res) => {
  const newTask = { id: tasks.length + 1, ...req.body, status: 'stopped', fps: 0, createdAt: new Date().toISOString(), updatedAt: new Date().toISOString() }
  tasks.push(newTask)
  res.json(ok(newTask))
})

app.put('/api/v1/tasks/:id', verifyAuth, (req, res) => {
  const task = tasks.find(t => t.id === +req.params.id)
  if (task) Object.assign(task, req.body, { updatedAt: new Date().toISOString() })
  res.json(ok(task))
})

app.delete('/api/v1/tasks/:id', verifyAuth, (req, res) => {
  tasks = tasks.filter(t => t.id !== +req.params.id)
  res.json(ok(null, '删除成功'))
})

app.post('/api/v1/tasks/:id/start', verifyAuth, (req, res) => {
  const task = tasks.find(t => t.id === +req.params.id)
  if (task) { task.status = 'running'; task.fps = +(Math.random() * 30 + 10).toFixed(1) }
  res.json(ok(null, '任务已启动'))
})

app.post('/api/v1/tasks/:id/stop', verifyAuth, (req, res) => {
  const task = tasks.find(t => t.id === +req.params.id)
  if (task) { task.status = 'stopped'; task.fps = 0 }
  res.json(ok(null, '任务已停止'))
})

app.post('/api/v1/tasks/:id/pause', verifyAuth, (req, res) => {
  const task = tasks.find(t => t.id === +req.params.id)
  if (task) { task.status = 'paused'; task.fps = 0 }
  res.json(ok(null, '任务已暂停'))
})

app.post('/api/v1/tasks/:id/resume', verifyAuth, (req, res) => {
  const task = tasks.find(t => t.id === +req.params.id)
  if (task) { task.status = 'running'; task.fps = +(Math.random() * 30 + 10).toFixed(1) }
  res.json(ok(null, '任务已恢复'))
})

app.put('/api/v1/tasks/:id/config', verifyAuth, (req, res) => {
  const task = tasks.find(t => t.id === +req.params.id)
  if (task) task.config = { ...task.config, ...req.body }
  res.json(ok(null, '配置已更新'))
})

app.put('/api/v1/tasks/:id/roi', verifyAuth, (req, res) => {
  const task = tasks.find(t => t.id === +req.params.id)
  if (task) task.roi = req.body
  res.json(ok(null, 'ROI已更新'))
})

app.get('/api/v1/tasks/:id/metrics', verifyAuth, (req, res) => {
  res.json(ok({
    fps: +(Math.random() * 30 + 10).toFixed(1),
    latency: +(Math.random() * 50 + 5).toFixed(1),
    throughput: +(Math.random() * 100 + 50).toFixed(0),
    detections: Math.floor(Math.random() * 20 + 1),
    timestamp: new Date().toISOString(),
  }))
})

// ─── Monitor Routes ───
app.get('/api/v1/monitor/system', verifyAuth, (req, res) => {
  res.json(ok({
    cpuUsage: +(Math.random() * 40 + 30).toFixed(1),
    memoryUsage: +(Math.random() * 30 + 40).toFixed(1),
    memoryTotal: 16384,
    memoryUsed: Math.floor(Math.random() * 4096 + 6144),
    tpuUsage: +(Math.random() * 50 + 20).toFixed(1),
    gpuUsage: +(Math.random() * 30 + 10).toFixed(1),
    uptime: Math.floor(Date.now() / 1000 - 86400 * 3),
    timestamp: new Date().toISOString(),
  }))
})

app.get('/api/v1/monitor/tasks/:id', verifyAuth, (req, res) => {
  res.json(ok({
    taskId: +req.params.id,
    taskName: '示例任务',
    fps: +(Math.random() * 30 + 10).toFixed(1),
    latency: +(Math.random() * 50 + 5).toFixed(1),
    throughput: +(Math.random() * 100 + 50).toFixed(0),
    detections: Math.floor(Math.random() * 20 + 1),
  }))
})

app.get('/api/v1/monitor/devices/:id', verifyAuth, (req, res) => {
  const device = devices.find(d => d.id === +req.params.id)
  res.json(ok({
    deviceId: +req.params.id,
    deviceName: device?.name || '未知设备',
    status: device?.status || 'offline',
    fps: device?.fps || 0,
    bitrate: Math.floor(Math.random() * 8000 + 2000),
  }))
})

// ─── Dashboard Route ───
app.get('/api/v1/dashboard', verifyAuth, (req, res) => {
  const runningTasks = tasks.filter(t => t.status === 'running').length
  const onlineDevices = devices.filter(d => d.status === 'online').length
  const todayAlerts = alerts.filter(a => !a.acknowledged).length
  const taskStatusDistribution = {
    running: tasks.filter(t => t.status === 'running').length,
    stopped: tasks.filter(t => t.status === 'stopped').length,
    paused: tasks.filter(t => t.status === 'paused').length,
    error: tasks.filter(t => t.status === 'error').length,
  }
  const alertTrend = []
  for (let i = 6; i >= 0; i--) {
    const d = new Date()
    d.setDate(d.getDate() - i)
    alertTrend.push({
      date: d.toISOString().split('T')[0],
      count: Math.floor(Math.random() * 20 + 5),
    })
  }
  res.json(ok({
    totalDevices: devices.length,
    onlineDevices,
    runningTasks,
    todayAlerts,
    systemMetrics: {
      cpuUsage: +(Math.random() * 40 + 30).toFixed(1),
      memoryUsage: +(Math.random() * 30 + 40).toFixed(1),
      memoryTotal: 16384,
      memoryUsed: Math.floor(Math.random() * 4096 + 6144),
      tpuUsage: +(Math.random() * 50 + 20).toFixed(1),
      gpuUsage: +(Math.random() * 30 + 10).toFixed(1),
      uptime: Math.floor(Date.now() / 1000 - 86400 * 3),
      timestamp: new Date().toISOString(),
    },
    recentAlerts: alerts.slice(0, 5),
    taskStatusDistribution,
    alertTrend,
  }))
})

// ─── Alert Routes ───
app.get('/api/v1/alerts', verifyAuth, (req, res) => {
  const { page = 1, pageSize = 20, level, acknowledged } = req.query
  let filtered = [...alerts]
  if (level) filtered = filtered.filter(a => a.level === level)
  if (acknowledged !== undefined) filtered = filtered.filter(a => a.acknowledged === (acknowledged === 'true'))
  const total = filtered.length
  const start = (page - 1) * pageSize
  const items = filtered.slice(start, start + +pageSize)
  res.json(ok({ items, total }))
})

app.put('/api/v1/alerts/:id/acknowledge', verifyAuth, (req, res) => {
  const alert = alerts.find(a => a.id === +req.params.id)
  if (alert) { alert.acknowledged = true; alert.acknowledgedBy = 'admin' }
  res.json(ok(null, '已确认报警'))
})

app.post('/api/v1/alerts/batch-acknowledge', verifyAuth, (req, res) => {
  const { ids } = req.body
  ids.forEach(id => {
    const alert = alerts.find(a => a.id === id)
    if (alert) { alert.acknowledged = true; alert.acknowledgedBy = 'admin' }
  })
  res.json(ok(null, '批量确认成功'))
})

app.get('/api/v1/alerts/stats', verifyAuth, (req, res) => {
  res.json(ok({
    total: alerts.length,
    info: alerts.filter(a => a.level === 'info').length,
    warning: alerts.filter(a => a.level === 'warning').length,
    critical: alerts.filter(a => a.level === 'critical').length,
    unacknowledged: alerts.filter(a => !a.acknowledged).length,
  }))
})

app.get('/api/v1/alerts/rules', verifyAuth, (req, res) => {
  const { page = 1, pageSize = 20 } = req.query
  const total = alertRules.length
  const start = (page - 1) * pageSize
  const items = alertRules.slice(start, start + +pageSize)
  res.json(ok({ items, total }))
})

app.post('/api/v1/alerts/rules', verifyAuth, (req, res) => {
  const newRule = { id: alertRules.length + 1, ...req.body, createdAt: new Date().toISOString(), updatedAt: new Date().toISOString() }
  alertRules.push(newRule)
  res.json(ok(newRule))
})

app.put('/api/v1/alerts/rules/:id', verifyAuth, (req, res) => {
  const rule = alertRules.find(r => r.id === +req.params.id)
  if (rule) Object.assign(rule, req.body, { updatedAt: new Date().toISOString() })
  res.json(ok(rule))
})

app.delete('/api/v1/alerts/rules/:id', verifyAuth, (req, res) => {
  alertRules = alertRules.filter(r => r.id !== +req.params.id)
  res.json(ok(null, '删除成功'))
})

// ─── Plugin Routes ───
app.get('/api/v1/plugins', verifyAuth, (req, res) => {
  const { page = 1, pageSize = 20, status, type } = req.query
  let filtered = [...plugins]
  if (status) filtered = filtered.filter(p => p.status === status)
  if (type) filtered = filtered.filter(p => p.type === type)
  const total = filtered.length
  const start = (page - 1) * pageSize
  const items = filtered.slice(start, start + +pageSize)
  res.json(ok({ items, total }))
})

app.get('/api/v1/plugins/:id', verifyAuth, (req, res) => {
  const plugin = plugins.find(p => p.id === +req.params.id)
  res.json(ok(plugin || null))
})

app.post('/api/v1/plugins/install', verifyAuth, (req, res) => {
  const newPlugin = { id: plugins.length + 1, name: '新插件', version: '1.0.0', type: 'element', description: '新安装的插件', path: req.body.path || '/plugins/new.so', status: 'inactive', config: {}, createdAt: new Date().toISOString(), updatedAt: new Date().toISOString() }
  plugins.push(newPlugin)
  res.json(ok(newPlugin))
})

app.put('/api/v1/plugins/:id', verifyAuth, (req, res) => {
  const plugin = plugins.find(p => p.id === +req.params.id)
  if (plugin) Object.assign(plugin, req.body, { updatedAt: new Date().toISOString() })
  res.json(ok(plugin))
})

app.delete('/api/v1/plugins/:id', verifyAuth, (req, res) => {
  plugins = plugins.filter(p => p.id !== +req.params.id)
  res.json(ok(null, '删除成功'))
})

app.post('/api/v1/plugins/:id/activate', verifyAuth, (req, res) => {
  const plugin = plugins.find(p => p.id === +req.params.id)
  if (plugin) plugin.status = 'active'
  res.json(ok(null, '插件已激活'))
})

app.post('/api/v1/plugins/:id/deactivate', verifyAuth, (req, res) => {
  const plugin = plugins.find(p => p.id === +req.params.id)
  if (plugin) plugin.status = 'inactive'
  res.json(ok(null, '插件已停用'))
})

// ─── System Routes ───
app.get('/api/v1/system/info', verifyAuth, (req, res) => {
  res.json(ok({
    version: '1.0.0',
    uptime: Math.floor(Date.now() / 1000 - 86400 * 3),
    cpuModel: 'Sophon BM1684X',
    cpuCores: 8,
    memoryTotal: 16384,
    tpuModel: 'BM1684X',
    tpuCount: 1,
    os: 'Linux 5.4.0-aarch64',
    hostname: 'sophon-edge-01',
  }))
})

app.get('/api/v1/system/config', verifyAuth, (req, res) => {
  res.json(ok(systemConfig))
})

app.put('/api/v1/system/config', verifyAuth, (req, res) => {
  Object.assign(systemConfig, req.body)
  res.json(ok(systemConfig, '配置已更新'))
})

app.get('/api/v1/system/audit-logs', verifyAuth, (req, res) => {
  const { page = 1, pageSize = 20, action, userId } = req.query
  let filtered = [...auditLogs]
  if (action) filtered = filtered.filter(l => l.action === action)
  if (userId) filtered = filtered.filter(l => l.userId === +userId)
  const total = filtered.length
  const start = (page - 1) * pageSize
  const items = filtered.slice(start, start + +pageSize)
  res.json(ok({ items, total }))
})

app.post('/api/v1/system/backup', verifyAuth, (req, res) => {
  res.json(ok({ path: `/data/backups/backup_${Date.now()}.tar.gz` }))
})

app.post('/api/v1/system/restore', verifyAuth, (req, res) => {
  res.json(ok(null, '系统已恢复'))
})

app.get('/api/v1/system/firmware', verifyAuth, (req, res) => {
  res.json(ok(firmwareVersions))
})

app.post('/api/v1/system/firmware/:id/install', verifyAuth, (req, res) => {
  const fw = firmwareVersions.find(f => f.id === +req.params.id)
  if (fw) { fw.status = 'installed'; fw.installedAt = new Date().toISOString() }
  res.json(ok(null, '固件安装成功'))
})

app.post('/api/v1/system/reboot', verifyAuth, (req, res) => {
  res.json(ok(null, '系统将重启'))
})

// ─── Health Check ───
app.get('/api/v1/health', (req, res) => {
  res.json(ok({ status: 'ok', version: '1.0.0' }))
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
  ws.on('close', () => {
    console.log('[WS] Monitor client disconnected')
    clearInterval(interval)
  })
  ws.on('error', () => clearInterval(interval))
})

server.listen(PORT, '0.0.0.0', () => {
  console.log(`[Mock API Server] Running on http://0.0.0.0:${PORT}`)
  console.log(`[Mock API Server] API base: http://0.0.0.0:${PORT}/api/v1/`)
  console.log(`[Mock API Server] WebSocket: ws://0.0.0.0:${PORT}/ws/monitor`)
  console.log(`[Mock API Server] Test login: admin / any password`)
})
