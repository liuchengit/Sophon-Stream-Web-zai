import { get, post, put, del } from './request'

export interface Device {
  id: number
  name: string
  type: 'IPC' | 'NVR' | 'RTSP' | 'FILE' | 'GB28181'
  streamUrl: string
  protocol: 'RTSP' | 'RTMP' | 'GB28181' | 'FILE'
  resolution: string
  fps: number
  codec: string
  location: string
  description: string
  deviceId?: string
  status: 'online' | 'offline' | 'error'
  createdAt: string
  updatedAt: string
}

export interface DeviceFormData {
  name: string
  type: string
  streamUrl: string
  protocol: string
  resolution: string
  fps: number
  codec: string
  location: string
  description: string
  deviceId?: string
}

export function fetchDevices(params?: Record<string, any>) {
  return get<{ items: Device[]; total: number }>('/devices', params)
}

export function fetchDevice(id: number) {
  return get<Device>('/devices', { id })
}

export function createDevice(data: DeviceFormData) {
  return post<Device>('/devices', data)
}

export function updateDevice(id: number, data: Partial<DeviceFormData>) {
  return put<Device>('/devices', data, { id })
}

export function deleteDevice(id: number) {
  return del('/devices', { id })
}

export function getDeviceStatus(id: number) {
  return get<{ status: string; fps: number; bitrate: number }>('/devices/status', { id })
}

export function startStream(id: number) {
  return post('/devices/start', null, { id })
}

export function stopStream(id: number) {
  return post('/devices/stop', null, { id })
}
