import { defineStore } from 'pinia'
import {
  fetchAlgorithms as fetchAlgorithmsApi,
  createAlgorithm as createAlgorithmApi,
  updateAlgorithm as updateAlgorithmApi,
  deleteAlgorithm as deleteAlgorithmApi,
  validateModel as validateModelApi,
  type Algorithm,
  type AlgorithmFormData,
} from '@/api/algorithm'

interface AlgorithmState {
  algorithms: Algorithm[]
  currentAlgorithm: Algorithm | null
  total: number
  loading: boolean
  pagination: {
    page: number
    pageSize: number
  }
}

export const useAlgorithmStore = defineStore('algorithm', {
  state: (): AlgorithmState => ({
    algorithms: [],
    currentAlgorithm: null,
    total: 0,
    loading: false,
    pagination: { page: 1, pageSize: 20 },
  }),

  actions: {
    async fetchAlgorithms(params?: Record<string, any>) {
      this.loading = true
      try {
        const result = await fetchAlgorithmsApi({
          page: this.pagination.page,
          pageSize: this.pagination.pageSize,
          ...params,
        })
        this.algorithms = result.items
        this.total = result.total
      } finally {
        this.loading = false
      }
    },

    async createAlgorithm(data: AlgorithmFormData) {
      const algo = await createAlgorithmApi(data)
      this.algorithms.unshift(algo)
      this.total++
      return algo
    },

    async updateAlgorithm(id: number, data: Partial<AlgorithmFormData>) {
      const algo = await updateAlgorithmApi(id, data)
      const index = this.algorithms.findIndex((a) => a.id === id)
      if (index !== -1) this.algorithms[index] = algo
      return algo
    },

    async deleteAlgorithm(id: number) {
      await deleteAlgorithmApi(id)
      this.algorithms = this.algorithms.filter((a) => a.id !== id)
      this.total--
    },

    async validateModel(id: number) {
      return await validateModelApi(id)
    },

    setPagination(page: number, pageSize: number) {
      this.pagination.page = page
      this.pagination.pageSize = pageSize
    },
  },
})
