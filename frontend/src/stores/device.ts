import { defineStore } from 'pinia'
import { ref } from 'vue'
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

export const useDeviceStore = defineStore('device', () => {
  const devices = ref<Device[]>([])
  const currentDevice = ref<Device | null>(null)
  const total = ref(0)
  const loading = ref(false)
  const pagination = ref({ page: 1, pageSize: 20 })

  async function fetchDevices(params?: Record<string, any>) {
    loading.value = true
    try {
      const result = await fetchDevicesApi({
        page: pagination.value.page,
        pageSize: pagination.value.pageSize,
        ...params,
      })
      devices.value = result.items
      total.value = result.total
    } finally {
      loading.value = false
    }
  }

  async function fetchDevice(id: number) {
    loading.value = true
    try {
      currentDevice.value = await fetchDeviceApi(id)
    } finally {
      loading.value = false
    }
  }

  async function createDevice(data: DeviceFormData) {
    const device = await createDeviceApi(data)
    devices.value.unshift(device)
    total.value++
    return device
  }

  async function updateDevice(id: number, data: Partial<DeviceFormData>) {
    const device = await updateDeviceApi(id, data)
    const index = devices.value.findIndex((d) => d.id === id)
    if (index !== -1) {
      devices.value[index] = device
    }
    if (currentDevice.value?.id === id) {
      currentDevice.value = device
    }
    return device
  }

  async function deleteDevice(id: number) {
    await deleteDeviceApi(id)
    devices.value = devices.value.filter((d) => d.id !== id)
    total.value--
    if (currentDevice.value?.id === id) {
      currentDevice.value = null
    }
  }

  async function startStream(id: number) {
    await startStreamApi(id)
    const device = devices.value.find((d) => d.id === id)
    if (device) device.status = 'online'
    if (currentDevice.value?.id === id) currentDevice.value.status = 'online'
  }

  async function stopStream(id: number) {
    await stopStreamApi(id)
    const device = devices.value.find((d) => d.id === id)
    if (device) device.status = 'offline'
    if (currentDevice.value?.id === id) currentDevice.value.status = 'offline'
  }

  function setPagination(page: number, pageSize: number) {
    pagination.value.page = page
    pagination.value.pageSize = pageSize
  }

  return {
    devices,
    currentDevice,
    total,
    loading,
    pagination,
    fetchDevices,
    fetchDevice,
    createDevice,
    updateDevice,
    deleteDevice,
    startStream,
    stopStream,
    setPagination,
  }
})
