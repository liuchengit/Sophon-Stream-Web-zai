import { defineStore } from 'pinia'
import {
  fetchTasks as fetchTasksApi,
  fetchTask as fetchTaskApi,
  createTask as createTaskApi,
  updateTask as updateTaskApi,
  deleteTask as deleteTaskApi,
  startTask as startTaskApi,
  stopTask as stopTaskApi,
  pauseTask as pauseTaskApi,
  resumeTask as resumeTaskApi,
  updateTaskConfig as updateTaskConfigApi,
  updateTaskROI as updateTaskROIApi,
  fetchTaskMetrics as fetchTaskMetricsApi,
  type Task,
  type TaskFormData,
  type TaskMetrics,
} from '@/api/task'

interface TaskState {
  tasks: Task[]
  currentTask: Task | null
  currentMetrics: TaskMetrics | null
  total: number
  loading: boolean
  pagination: {
    page: number
    pageSize: number
  }
}

export const useTaskStore = defineStore('task', {
  state: (): TaskState => ({
    tasks: [],
    currentTask: null,
    currentMetrics: null,
    total: 0,
    loading: false,
    pagination: { page: 1, pageSize: 20 },
  }),

  actions: {
    async fetchTasks(params?: Record<string, any>) {
      this.loading = true
      try {
        const result = await fetchTasksApi({
          page: this.pagination.page,
          pageSize: this.pagination.pageSize,
          ...params,
        })
        this.tasks = result.items
        this.total = result.total
      } finally {
        this.loading = false
      }
    },

    async fetchTask(id: number) {
      this.loading = true
      try {
        this.currentTask = await fetchTaskApi(id)
      } finally {
        this.loading = false
      }
    },

    async createTask(data: TaskFormData) {
      const task = await createTaskApi(data)
      this.tasks.unshift(task)
      this.total++
      return task
    },

    async updateTask(id: number, data: Partial<TaskFormData>) {
      const task = await updateTaskApi(id, data)
      const index = this.tasks.findIndex((t) => t.id === id)
      if (index !== -1) this.tasks[index] = task
      if (this.currentTask?.id === id) this.currentTask = task
      return task
    },

    async deleteTask(id: number) {
      await deleteTaskApi(id)
      this.tasks = this.tasks.filter((t) => t.id !== id)
      this.total--
      if (this.currentTask?.id === id) this.currentTask = null
    },

    async startTask(id: number) {
      await startTaskApi(id)
      this.updateTaskStatus(id, 'running')
    },

    async stopTask(id: number) {
      await stopTaskApi(id)
      this.updateTaskStatus(id, 'stopped')
    },

    async pauseTask(id: number) {
      await pauseTaskApi(id)
      this.updateTaskStatus(id, 'paused')
    },

    async resumeTask(id: number) {
      await resumeTaskApi(id)
      this.updateTaskStatus(id, 'running')
    },

    async updateTaskConfig(id: number, config: Record<string, any>) {
      await updateTaskConfigApi(id, config)
      if (this.currentTask?.id === id) {
        this.currentTask.config = config
      }
    },

    async updateTaskROI(id: number, roi: Record<string, any>) {
      await updateTaskROIApi(id, roi)
      if (this.currentTask?.id === id) {
        this.currentTask.roi = roi
      }
    },

    async fetchTaskMetrics(id: number) {
      this.currentMetrics = await fetchTaskMetricsApi(id)
    },

    updateTaskStatus(id: number, status: Task['status']) {
      const task = this.tasks.find((t) => t.id === id)
      if (task) task.status = status
      if (this.currentTask?.id === id) this.currentTask.status = status
    },

    setPagination(page: number, pageSize: number) {
      this.pagination.page = page
      this.pagination.pageSize = pageSize
    },
  },
})
