import { get, post, put, del } from './request'

export interface Alert {
  id: number
  taskId: number
  taskName?: string
  type: string
  level: 'info' | 'warning' | 'critical'
  message: string
  evidence: string
  acknowledged: boolean
  acknowledgedBy?: string
  createdAt: string
}

export interface AlertRule {
  id: number
  name: string
  type: string
  condition: Record<string, any>
  threshold: number
  debounce: number
  notificationConfig: Record<string, any>
  enabled: boolean
  createdAt: string
  updatedAt: string
}

export interface AlertRuleFormData {
  name: string
  type: string
  condition: Record<string, any>
  threshold: number
  debounce: number
  notificationConfig: Record<string, any>
  enabled: boolean
}

export interface AlertStats {
  total: number
  info: number
  warning: number
  critical: number
  unacknowledged: number
}

export function fetchAlerts(params?: Record<string, any>) {
  return get<{ items: Alert[]; total: number }>('/alerts', params)
}

export function acknowledgeAlert(id: number) {
  return put(`/alerts`, { id, acknowledged: true })
}

export function batchAcknowledgeAlerts(ids: number[]) {
  return post('/alerts/batch-acknowledge', { ids })
}

export function fetchAlertStats() {
  return post<AlertStats>('/alerts/stats')
}

export function fetchAlertRules(params?: Record<string, any>) {
  return get<{ items: AlertRule[]; total: number }>('/alert-rules', params)
}

export function createAlertRule(data: AlertRuleFormData) {
  return post<AlertRule>('/alert-rules', data)
}

export function updateAlertRule(id: number, data: Partial<AlertRuleFormData>) {
  return put<AlertRule>(`/alert-rules/${id}`, data)
}

export function deleteAlertRule(id: number) {
  return del(`/alert-rules/${id}`)
}
