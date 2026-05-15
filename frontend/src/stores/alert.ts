import { defineStore } from 'pinia'
import {
  fetchAlerts as fetchAlertsApi,
  acknowledgeAlert as acknowledgeAlertApi,
  batchAcknowledgeAlerts as batchAcknowledgeAlertsApi,
  fetchAlertStats as fetchAlertStatsApi,
  fetchAlertRules as fetchAlertRulesApi,
  createAlertRule as createAlertRuleApi,
  updateAlertRule as updateAlertRuleApi,
  deleteAlertRule as deleteAlertRuleApi,
  type Alert,
  type AlertRule,
  type AlertRuleFormData,
  type AlertStats,
} from '@/api/alert'

interface AlertState {
  alerts: Alert[]
  rules: AlertRule[]
  stats: AlertStats | null
  total: number
  loading: boolean
  pagination: {
    page: number
    pageSize: number
  }
}

export const useAlertStore = defineStore('alert', {
  state: (): AlertState => ({
    alerts: [],
    rules: [],
    stats: null,
    total: 0,
    loading: false,
    pagination: { page: 1, pageSize: 20 },
  }),

  actions: {
    async fetchAlerts(params?: Record<string, any>) {
      this.loading = true
      try {
        const result = await fetchAlertsApi({
          page: this.pagination.page,
          pageSize: this.pagination.pageSize,
          ...params,
        })
        this.alerts = result.items
        this.total = result.total
      } finally {
        this.loading = false
      }
    },

    async fetchAlertStats() {
      this.stats = await fetchAlertStatsApi()
    },

    async acknowledgeAlert(id: number) {
      await acknowledgeAlertApi(id)
      const alert = this.alerts.find((a) => a.id === id)
      if (alert) alert.acknowledged = true
      if (this.stats) this.stats.unacknowledged = Math.max(0, this.stats.unacknowledged - 1)
    },

    async batchAcknowledge(ids: number[]) {
      await batchAcknowledgeAlertsApi(ids)
      ids.forEach((id) => {
        const alert = this.alerts.find((a) => a.id === id)
        if (alert) alert.acknowledged = true
      })
      if (this.stats) {
        this.stats.unacknowledged = Math.max(0, this.stats.unacknowledged - ids.length)
      }
    },

    async fetchAlertRules(params?: Record<string, any>) {
      this.loading = true
      try {
        const result = await fetchAlertRulesApi(params)
        this.rules = result.items
      } finally {
        this.loading = false
      }
    },

    async createAlertRule(data: AlertRuleFormData) {
      const rule = await createAlertRuleApi(data)
      this.rules.unshift(rule)
      return rule
    },

    async updateAlertRule(id: number, data: Partial<AlertRuleFormData>) {
      const rule = await updateAlertRuleApi(id, data)
      const index = this.rules.findIndex((r) => r.id === id)
      if (index !== -1) this.rules[index] = rule
      return rule
    },

    async deleteAlertRule(id: number) {
      await deleteAlertRuleApi(id)
      this.rules = this.rules.filter((r) => r.id !== id)
    },

    setPagination(page: number, pageSize: number) {
      this.pagination.page = page
      this.pagination.pageSize = pageSize
    },
  },
})
