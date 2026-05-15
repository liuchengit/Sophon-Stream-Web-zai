import { get, post, put } from './request'

export interface SystemInfo {
  version: string
  uptime: number
  cpuModel: string
  cpuCores: number
  memoryTotal: number
  tpuModel: string
  tpuCount: number
  os: string
  hostname: string
}

export interface SystemConfig {
  [key: string]: string | number | boolean
}

export interface AuditLog {
  id: number
  userId: number
  username: string
  action: string
  resource: string
  detail: string
  ip: string
  createdAt: string
}

export interface FirmwareVersion {
  id: number
  version: string
  description: string
  filePath: string
  checksum: string
  installedAt?: string
  status: 'available' | 'installed' | 'installing' | 'failed'
  createdAt: string
}

export function fetchSystemInfo() {
  return get<SystemInfo>('/system/info')
}

export function fetchSystemConfig() {
  return get<SystemConfig>('/system/config')
}

export function updateSystemConfig(data: SystemConfig) {
  return put('/system/config', data)
}

export function fetchAuditLogs(params?: Record<string, any>) {
  return get<{ items: AuditLog[]; total: number }>('/system/audit-logs', params)
}

export function backupSystem() {
  return post<{ path: string }>('/system/backup')
}

export function restoreSystem(data: { path: string }) {
  return post('/system/restore', data)
}

export function fetchFirmwareVersions() {
  return get<FirmwareVersion[]>('/system/firmware')
}

export function installFirmware(id: number) {
  return post(`/system/firmware/${id}/install`)
}

export function rebootSystem() {
  return post('/system/reboot')
}


