import { defineStore } from 'pinia'
import {
  fetchPlugins as fetchPluginsApi,
  installPlugin as installPluginApi,
  updatePlugin as updatePluginApi,
  deletePlugin as deletePluginApi,
  activatePlugin as activatePluginApi,
  deactivatePlugin as deactivatePluginApi,
  type Plugin,
  type PluginFormData,
} from '@/api/plugin'

interface PluginState {
  plugins: Plugin[]
  total: number
  loading: boolean
}

export const usePluginStore = defineStore('plugin', {
  state: (): PluginState => ({
    plugins: [],
    total: 0,
    loading: false,
  }),

  actions: {
    async fetchPlugins(params?: Record<string, any>) {
      this.loading = true
      try {
        const result = await fetchPluginsApi(params)
        this.plugins = result.items
        this.total = result.total
      } finally {
        this.loading = false
      }
    },

    async installPlugin(data: { path: string }) {
      const plugin = await installPluginApi(data)
      this.plugins.unshift(plugin)
      this.total++
      return plugin
    },

    async updatePlugin(id: number, data: Partial<PluginFormData>) {
      const plugin = await updatePluginApi(id, data)
      const index = this.plugins.findIndex((p) => p.id === id)
      if (index !== -1) this.plugins[index] = plugin
      return plugin
    },

    async deletePlugin(id: number) {
      await deletePluginApi(id)
      this.plugins = this.plugins.filter((p) => p.id !== id)
      this.total--
    },

    async activatePlugin(id: number) {
      await activatePluginApi(id)
      const plugin = this.plugins.find((p) => p.id === id)
      if (plugin) plugin.status = 'active'
    },

    async deactivatePlugin(id: number) {
      await deactivatePluginApi(id)
      const plugin = this.plugins.find((p) => p.id === id)
      if (plugin) plugin.status = 'inactive'
    },
  },
})
