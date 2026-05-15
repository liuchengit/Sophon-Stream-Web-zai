import { defineStore } from 'pinia'
import {
  fetchDevices as fetchDevicesApi,
  fetchDevice as fetchDeviceApi,
  createDevice as createDeviceApi,
  updateDevice as updateDeviceApi,
  deleteDevice as deleteDeviceApi,
  startStream as startStreamApi,
  stopStream as stopStreamApi,
  type Device,
  type DeviceFormData,
} from '@/api/device'

interface DeviceState {
  devices: Device[]
  currentDevice: Device | null
  total: number
  loading: boolean
  pagination: {
    page: number
    pageSize: number
  }
}

export const useDeviceStore = defineStore('device', {
  state: (): DeviceState => ({
    devices: [],
    currentDevice: null,
    total: 0,
    loading: false,
    pagination: { page: 1, pageSize: 20 },
  }),

  actions: {
    async fetchDevices(params?: Record<string, any>) {
      this.loading = true
      try {
        const result = await fetchDevicesApi({
          page: this.pagination.page,
          pageSize: this.pagination.pageSize,
          ...params,
        })
        this.devices = result.items
        this.total = result.total
      } finally {
        this.loading = false
      }
    },

    async fetchDevice(id: number) {
      this.loading = true
      try {
        this.currentDevice = await fetchDeviceApi(id)
      } finally {
        this.loading = false
      }
    },

    async createDevice(data: DeviceFormData) {
      const device = await createDeviceApi(data)
      this.devices.unshift(device)
      this.total++
      return device
    },

    async updateDevice(id: number, data: Partial<DeviceFormData>) {
      const device = await updateDeviceApi(id, data)
      const index = this.devices.findIndex((d) => d.id === id)
      if (index !== -1) {
        this.devices[index] = device
      }
      if (this.currentDevice?.id === id) {
        this.currentDevice = device
      }
      return device
    },

    async deleteDevice(id: number) {
      await deleteDeviceApi(id)
      this.devices = this.devices.filter((d) => d.id !== id)
      this.total--
      if (this.currentDevice?.id === id) {
        this.currentDevice = null
      }
    },

    async startStream(id: number) {
      await startStreamApi(id)
      const device = this.devices.find((d) => d.id === id)
      if (device) device.status = 'online'
      if (this.currentDevice?.id === id) this.currentDevice.status = 'online'
    },

    async stopStream(id: number) {
      await stopStreamApi(id)
      const device = this.devices.find((d) => d.id === id)
      if (device) device.status = 'offline'
      if (this.currentDevice?.id === id) this.currentDevice.status = 'offline'
    },

    setPagination(page: number, pageSize: number) {
      this.pagination.page = page
      this.pagination.pageSize = pageSize
    },
  },
})
