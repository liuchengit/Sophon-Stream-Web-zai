import { get, put, del } from './request'

export interface User {
  id: number
  username: string
  role: 'admin' | 'operator' | 'viewer'
}

export interface AuditLog {
  id: number
  userId: number
  username: string
  action: string
  resource: string
  ipAddress: string
  userAgent: string
  createdAt: string
}

export interface FirmwareVersion {
  id: number
  version: string
  description: string
  releaseDate: string
  fileSize: string
  downloadUrl: string
  isLatest: boolean
}

export interface SystemInfo {
  version: string
  buildDate: string
  platform: string
  uptime: number
}

export interface SystemConfig {
  siteName: string
  enableAuth: boolean
  sessionTimeout: number
  maxDevices: number
}

export function fetchUsers() {
  return get<User[]>('/auth/users')
}

export function fetchSystemInfo() {
  return Promise.resolve({
    version: 'v1.0.0',
    buildDate: '2024-01-01',
    platform: 'Linux x86_64',
    uptime: 86400,
  } as SystemInfo)
}

export function fetchSystemConfig() {
  return Promise.resolve({
    siteName: 'Sophon-Stream',
    enableAuth: true,
    sessionTimeout: 3600,
    maxDevices: 100,
  } as SystemConfig)
}

export function updateSystemConfig(data: Partial<SystemConfig>) {
  return Promise.resolve(data as SystemConfig)
}

export function fetchAuditLogs(params?: Record<string, any>) {
  return Promise.resolve({ items: [] as AuditLog[], total: 0 })
}

export function fetchFirmwareVersions() {
  return Promise.resolve([
    {
      id: 1,
      version: 'v1.0.0',
      description: '初始版本',
      releaseDate: '2024-01-01',
      fileSize: '128MB',
      downloadUrl: '',
      isLatest: true,
    },
    {
      id: 2,
      version: 'v0.9.5',
      description: '测试版本',
      releaseDate: '2023-12-01',
      fileSize: '120MB',
      downloadUrl: '',
      isLatest: false,
    }
  ] as FirmwareVersion[])
}

export function installFirmware(id: number) {
  return Promise.resolve({ success: true, message: '固件安装成功' })
}

export function updateUser(id: number, data: Partial<User>) {
  return put<User>('/auth/users', data, { id })
}

export function deleteUser(id: number) {
  return del('/auth/users', { id })
}

export function backupSystem() {
  return Promise.resolve({ success: true, backupPath: '/backup/system.tar.gz' })
}

export function restoreSystem(data: { path: string }) {
  return Promise.resolve({ success: true, message: '系统已恢复' })
}

export function rebootSystem() {
  return Promise.resolve({ success: true, message: '系统重启中...' })
}
