import { defineStore } from 'pinia'
import {
  fetchSystemMetrics as fetchSystemMetricsApi,
  fetchDashboard as fetchDashboardApi,
  type SystemMetrics,
  type DashboardData,
} from '@/api/monitor'

interface MonitorState {
  systemMetrics: SystemMetrics | null
  dashboardData: DashboardData | null
  loading: boolean
  wsConnected: boolean
}

export const useMonitorStore = defineStore('monitor', {
  state: (): MonitorState => ({
    systemMetrics: null,
    dashboardData: null,
    loading: false,
    wsConnected: false,
  }),

  actions: {
    async fetchSystemMetrics() {
      this.systemMetrics = await fetchSystemMetricsApi()
    },

    async fetchDashboard() {
      this.loading = true
      try {
        this.dashboardData = await fetchDashboardApi()
      } finally {
        this.loading = false
      }
    },

    updateMetricsFromWS(data: Partial<SystemMetrics>) {
      if (this.systemMetrics) {
        Object.assign(this.systemMetrics, data)
      } else {
        this.systemMetrics = data as SystemMetrics
      }
    },

    setWsConnected(connected: boolean) {
      this.wsConnected = connected
    },
  },
})
