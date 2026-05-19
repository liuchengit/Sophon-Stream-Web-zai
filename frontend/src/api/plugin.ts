import { get, post, put, del } from './request'

export interface Plugin {
  id: number
  name: string
  version: string
  type: string
  description: string
  path: string
  status: 'active' | 'inactive'
  config: Record<string, any>
  createdAt: string
  updatedAt: string
}

export interface PluginFormData {
  name: string
  version: string
  type: string
  description: string
  path: string
  config: Record<string, any>
}

export function fetchPlugins(params?: Record<string, any>) {
  return get<{ items: Plugin[]; total: number }>('/plugins', params)
}

export function fetchPlugin(id: number) {
  return get<Plugin>('/plugins', { id })
}

export function installPlugin(data: { path: string } | FormData) {
  return post<Plugin>('/plugins/install', data as any)
}

export function updatePlugin(id: number, data: Partial<PluginFormData>) {
  return put<Plugin>('/plugins', { ...data, id })
}

export function deletePlugin(id: number) {
  return del('/plugins', { id })
}

export function activatePlugin(id: number) {
  return post('/plugins/activate', { id })
}

export function deactivatePlugin(id: number) {
  return post('/plugins/deactivate', { id })
}
