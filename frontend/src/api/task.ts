import { get, post, put, del } from './request'

export interface Task {
  id: number
  name: string
  type: string
  description: string
  deviceId: number
  deviceName?: string
  algorithmIds: number[]
  algorithmNames?: string[]
  status: 'running' | 'stopped' | 'paused' | 'error'
  fps: number
  config: Record<string, any>
  roi: Record<string, any>
  graphId?: number
  createdAt: string
  updatedAt: string
}

export interface TaskFormData {
  name: string
  type: string
  description: string
  deviceId: number
  algorithmIds: number[]
  config: Record<string, any>
  roi: Record<string, any>
}

export interface TaskMetrics {
  fps: number
  latency: number
  throughput: number
  detections: number
  timestamp: string
}

export function fetchTasks(params?: Record<string, any>) {
  return get<{ items: Task[]; total: number }>('/tasks', params)
}

export function fetchTask(id: number) {
  return get<Task>(`/tasks/${id}`)
}

export function createTask(data: TaskFormData) {
  return post<Task>('/tasks', data)
}

export function updateTask(id: number, data: Partial<TaskFormData>) {
  return put<Task>(`/tasks/${id}`, data)
}

export function deleteTask(id: number) {
  return del(`/tasks/${id}`)
}

export function startTask(id: number) {
  return post(`/tasks/${id}/start`)
}

export function stopTask(id: number) {
  return post(`/tasks/${id}/stop`)
}

export function pauseTask(id: number) {
  return post(`/tasks/${id}/pause`)
}

export function resumeTask(id: number) {
  return post(`/tasks/${id}/resume`)
}

export function updateTaskConfig(id: number, config: Record<string, any>) {
  return put(`/tasks/${id}/config`, config)
}

export function updateTaskROI(id: number, roi: Record<string, any>) {
  return put(`/tasks/${id}/roi`, roi)
}

export function fetchTaskMetrics(id: number) {
  return get<TaskMetrics>(`/tasks/${id}/metrics`)
}
