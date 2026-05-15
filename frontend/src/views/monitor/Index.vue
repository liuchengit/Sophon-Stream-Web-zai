<template>
  <div class="monitor-page">
    <PageHeader title="系统监控" description="实时监控系统资源使用情况和任务运行状态" />

    <!-- System Resource Gauges -->
    <el-row :gutter="16" class="gauge-row">
      <el-col :xs="24" :sm="8">
        <el-card shadow="never" class="monitor-card">
          <template #header>
            <span class="card-title">CPU 使用率</span>
          </template>
          <GaugeChart :value="metrics?.cpuUsage || 0" title="CPU" height="220px" />
          <div class="gauge-detail">
            <LineChart :x-axis="cpuHistoryLabels" :series="cpuHistorySeries" height="150px" :area-style="true" />
          </div>
        </el-card>
      </el-col>
      <el-col :xs="24" :sm="8">
        <el-card shadow="never" class="monitor-card">
          <template #header>
            <span class="card-title">内存使用率</span>
          </template>
          <GaugeChart :value="metrics?.memoryUsage || 0" title="内存" height="220px" />
          <div class="gauge-detail">
            <span class="detail-text">{{ formatBytes(metrics?.memoryUsed) }} / {{ formatBytes(metrics?.memoryTotal) }}</span>
            <LineChart :x-axis="memHistoryLabels" :series="memHistorySeries" height="150px" :area-style="true" />
          </div>
        </el-card>
      </el-col>
      <el-col :xs="24" :sm="8">
        <el-card shadow="never" class="monitor-card">
          <template #header>
            <span class="card-title">TPU 使用率</span>
          </template>
          <GaugeChart :value="metrics?.tpuUsage || 0" title="TPU" height="220px" />
          <div class="gauge-detail">
            <LineChart :x-axis="tpuHistoryLabels" :series="tpuHistorySeries" height="150px" :area-style="true" />
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- Task & Device Metrics -->
    <el-row :gutter="16" class="mt-16">
      <el-col :xs="24" :sm="12">
        <el-card shadow="never" class="monitor-card">
          <template #header>
            <div class="card-header-flex">
              <span class="card-title">任务指标</span>
              <el-select v-model="selectedTaskId" placeholder="选择任务" size="small" clearable style="width: 180px" @change="loadTaskMetrics">
                <el-option v-for="t in taskStore.tasks" :key="t.id" :label="t.name" :value="t.id" />
              </el-select>
            </div>
          </template>
          <div v-if="taskMetrics" class="task-metrics-grid">
            <div class="task-metric-item">
              <div class="metric-value">{{ taskMetrics.fps?.toFixed(1) || '-' }}</div>
              <div class="metric-label">FPS</div>
            </div>
            <div class="task-metric-item">
              <div class="metric-value">{{ taskMetrics.latency?.toFixed(0) || '-' }} ms</div>
              <div class="metric-label">延迟</div>
            </div>
            <div class="task-metric-item">
              <div class="metric-value">{{ taskMetrics.throughput?.toFixed(0) || '-' }}</div>
              <div class="metric-label">吞吐量</div>
            </div>
            <div class="task-metric-item">
              <div class="metric-value">{{ taskMetrics.detections || '-' }}</div>
              <div class="metric-label">检测数</div>
            </div>
          </div>
          <el-empty v-else description="请选择任务查看指标" :image-size="60" />
        </el-card>
      </el-col>
      <el-col :xs="24" :sm="12">
        <el-card shadow="never" class="monitor-card">
          <template #header>
            <div class="card-header-flex">
              <span class="card-title">设备指标</span>
              <el-select v-model="selectedDeviceId" placeholder="选择设备" size="small" clearable style="width: 180px">
                <el-option v-for="d in deviceStore.devices" :key="d.id" :label="d.name" :value="d.id" />
              </el-select>
            </div>
          </template>
          <div v-if="selectedDeviceId" class="device-metrics-grid">
            <div class="task-metric-item">
              <div class="metric-value">{{ selectedDevice?.status === 'online' ? '在线' : '离线' }}</div>
              <div class="metric-label">状态</div>
            </div>
            <div class="task-metric-item">
              <div class="metric-value">{{ selectedDevice?.fps || '-' }}</div>
              <div class="metric-label">FPS</div>
            </div>
            <div class="task-metric-item">
              <div class="metric-value">{{ selectedDevice?.resolution || '-' }}</div>
              <div class="metric-label">分辨率</div>
            </div>
            <div class="task-metric-item">
              <div class="metric-value">{{ selectedDevice?.codec || '-' }}</div>
              <div class="metric-label">编码</div>
            </div>
          </div>
          <el-empty v-else description="请选择设备查看指标" :image-size="60" />
        </el-card>
      </el-col>
    </el-row>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { useMonitorStore } from '@/stores/monitor'
import { useTaskStore } from '@/stores/task'
import { useDeviceStore } from '@/stores/device'
import { fetchTaskMetrics, type TaskMetrics } from '@/api/task'
import GaugeChart from '@/components/chart/GaugeChart.vue'
import LineChart from '@/components/chart/LineChart.vue'
import PageHeader from '@/components/common/PageHeader.vue'
import dayjs from 'dayjs'

const monitorStore = useMonitorStore()
const taskStore = useTaskStore()
const deviceStore = useDeviceStore()

const selectedTaskId = ref<number | null>(null)
const selectedDeviceId = ref<number | null>(null)
const taskMetrics = ref<TaskMetrics | null>(null)

// History data for charts (simulated rolling data)
const maxHistoryPoints = 30
const cpuHistory = ref<number[]>([])
const memHistory = ref<number[]>([])
const tpuHistory = ref<number[]>([])
const historyLabels = ref<string[]>([])

let refreshTimer: ReturnType<typeof setInterval> | null = null

const metrics = computed(() => monitorStore.systemMetrics)

const selectedDevice = computed(() =>
  selectedDeviceId.value ? deviceStore.devices.find((d) => d.id === selectedDeviceId.value) : null
)

function addHistoryPoint(arr: ref<number[]>, value: number) {
  arr.value.push(value)
  if (arr.value.length > maxHistoryPoints) arr.value.shift()
}

function formatBytes(bytes?: number) {
  if (!bytes) return '-'
  if (bytes < 1024) return `${bytes} B`
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`
  if (bytes < 1024 * 1024 * 1024) return `${(bytes / 1024 / 1024).toFixed(1)} MB`
  return `${(bytes / 1024 / 1024 / 1024).toFixed(1)} GB`
}

const cpuHistoryLabels = computed(() => historyLabels.value)
const memHistoryLabels = computed(() => historyLabels.value)
const tpuHistoryLabels = computed(() => historyLabels.value)

const cpuHistorySeries = computed(() => [{ name: 'CPU', data: cpuHistory.value }])
const memHistorySeries = computed(() => [{ name: '内存', data: memHistory.value }])
const tpuHistorySeries = computed(() => [{ name: 'TPU', data: tpuHistory.value }])

async function loadTaskMetrics() {
  if (selectedTaskId.value) {
    try {
      taskMetrics.value = await fetchTaskMetrics(selectedTaskId.value)
    } catch { /* handled */ }
  } else {
    taskMetrics.value = null
  }
}

async function refreshData() {
  await monitorStore.fetchSystemMetrics()
  const m = metrics.value
  if (m) {
    addHistoryPoint(cpuHistory, m.cpuUsage)
    addHistoryPoint(memHistory, m.memoryUsage)
    addHistoryPoint(tpuHistory, m.tpuUsage)
    historyLabels.value.push(dayjs(m.timestamp || Date.now()).format('HH:mm:ss'))
    if (historyLabels.value.length > maxHistoryPoints) historyLabels.value.shift()
  }
  if (selectedTaskId.value) {
    loadTaskMetrics()
  }
}

onMounted(async () => {
  await Promise.all([
    taskStore.fetchTasks({ page: 1, pageSize: 100 }),
    deviceStore.fetchDevices({ page: 1, pageSize: 100 }),
  ])
  await refreshData()
  refreshTimer = setInterval(refreshData, 5000)
})

onUnmounted(() => {
  if (refreshTimer) clearInterval(refreshTimer)
})
</script>

<style scoped lang="scss">
.monitor-card {
  background: #132f4c;
  border: 1px solid #1e3a5f;
  margin-bottom: 16px;

  :deep(.el-card__header) {
    padding: 12px 20px;
    border-bottom: 1px solid #1e3a5f;
  }
}

.card-title {
  font-size: 14px;
  font-weight: 600;
  color: #e3e8ef;
}

.card-header-flex {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.gauge-detail {
  margin-top: 8px;
  border-top: 1px solid #1e3a5f;
  padding-top: 12px;
}

.detail-text {
  font-size: 12px;
  color: #78909c;
  display: block;
  margin-bottom: 8px;
}

.task-metrics-grid, .device-metrics-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 12px;
}

.task-metric-item {
  background: rgba(10, 25, 41, 0.5);
  border: 1px solid #1e3a5f;
  border-radius: 8px;
  padding: 16px;
  text-align: center;
}

.metric-value {
  font-size: 20px;
  font-weight: 700;
  color: #409EFF;
}

.metric-label {
  font-size: 12px;
  color: #78909c;
  margin-top: 4px;
}
</style>
