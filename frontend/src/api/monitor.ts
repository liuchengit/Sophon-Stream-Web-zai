import { get } from './request'

export interface SystemMetrics {
  cpuUsage: number
  memoryUsage: number
  memoryTotal: number
  memoryUsed: number
  tpuUsage: number
  gpuUsage: number
  uptime: number
  timestamp: string
}

export interface TaskMonitorData {
  taskId: number
  taskName: string
  fps: number
  latency: number
  throughput: number
  detections: number
}

export interface DeviceMonitorData {
  deviceId: number
  deviceName: string
  status: string
  fps: number
  bitrate: number
}

export interface DashboardData {
  totalDevices: number
  onlineDevices: number
  runningTasks: number
  todayAlerts: number
  systemMetrics: SystemMetrics
  recentAlerts: AlertItem[]
  taskStatusDistribution: Record<string, number>
  alertTrend: { date: string; count: number }[]
}

export interface AlertItem {
  id: number
  time: string
  taskName: string
  type: string
  level: string
  message: string
  acknowledged: boolean
}

export function fetchSystemMetrics() {
  return get<SystemMetrics>('/monitor/system')
}

export function fetchTaskMonitor(id: number) {
  return get<TaskMonitorData>(`/monitor/tasks/${id}`)
}

export function fetchDeviceMonitor(id: number) {
  return get<DeviceMonitorData>(`/monitor/devices/${id}`)
}

export function fetchDashboard() {
  return get<DashboardData>('/dashboard')
}
