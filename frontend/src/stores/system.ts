import { defineStore } from 'pinia'
import {
  fetchSystemInfo as fetchSystemInfoApi,
  fetchSystemConfig as fetchSystemConfigApi,
  updateSystemConfig as updateSystemConfigApi,
  fetchAuditLogs as fetchAuditLogsApi,
  fetchFirmwareVersions as fetchFirmwareVersionsApi,
  type SystemInfo,
  type SystemConfig,
  type AuditLog,
  type FirmwareVersion,
} from '@/api/system'

interface SystemState {
  systemInfo: SystemInfo | null
  systemConfig: SystemConfig | null
  auditLogs: AuditLog[]
  firmwareVersions: FirmwareVersion[]
  auditTotal: number
  loading: boolean
}

export const useSystemStore = defineStore('system', {
  state: (): SystemState => ({
    systemInfo: null,
    systemConfig: null,
    auditLogs: [],
    firmwareVersions: [],
    auditTotal: 0,
    loading: false,
  }),

  actions: {
    async fetchSystemInfo() {
      this.systemInfo = await fetchSystemInfoApi()
    },

    async fetchSystemConfig() {
      this.systemConfig = await fetchSystemConfigApi()
    },

    async updateSystemConfig(data: SystemConfig) {
      await updateSystemConfigApi(data)
      this.systemConfig = { ...this.systemConfig, ...data }
    },

    async fetchAuditLogs(params?: Record<string, any>) {
      this.loading = true
      try {
        const result = await fetchAuditLogsApi(params)
        this.auditLogs = result.items
        this.auditTotal = result.total
      } finally {
        this.loading = false
      }
    },

    async fetchFirmwareVersions() {
      this.loading = true
      try {
        this.firmwareVersions = await fetchFirmwareVersionsApi()
      } finally {
        this.loading = false
      }
    },
  },
})
