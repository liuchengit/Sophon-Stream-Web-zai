import { get, post, put, del } from './request'

export interface Task {
  id: number
  name: string
  status: 'running' | 'stopped' | 'paused' | 'error'
  deviceId: number
  algorithmId: number
  config: Record<string, any>
  description: string
  createdAt: string
  updatedAt: string
}

export interface TaskFormData {
  name: string
  deviceId: number
  algorithmId: number
  config?: Record<string, any>
  description?: string
}

export interface TaskMetrics {
  deviceId: number
  taskId: number
  fps: number
  latency: number
  processedCount: number
  alertCount: number
  uptime: number
  timestamp: string
}

export function fetchTasks(params?: Record<string, any>) {
  return get<{ items: Task[]; total: number }>('/tasks', params)
}

export function fetchTask(id: number) {
  return get<Task>('/tasks', { id })
}

export function createTask(data: TaskFormData) {
  return post<Task>('/tasks', data)
}

export function updateTask(id: number, data: Partial<TaskFormData>) {
  return put<Task>('/tasks', data, { id })
}

export function deleteTask(id: number) {
  return del('/tasks', { id })
}

export function startTask(id: number) {
  return post('/tasks/start', null, { id })
}

export function stopTask(id: number) {
  return post('/tasks/stop', null, { id })
}

export function pauseTask(id: number) {
  return post('/tasks/pause', null, { id })
}

export function resumeTask(id: number) {
  return post('/tasks/resume', null, { id })
}

export function getTaskMetrics(id: number) {
  return get<TaskMetrics>('/tasks/metrics', { id })
}

// Alias for compatibility
export const fetchTaskMetrics = getTaskMetrics

export function updateTaskConfig(id: number, config: Record<string, any>) {
  return put('/tasks/config', config, { id })
}

export function updateTaskROI(id: number, roi: Record<string, any>) {
  return put('/tasks/roi', roi, { id })
}
