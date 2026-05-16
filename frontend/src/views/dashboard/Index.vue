<template>
  <div class="dashboard-page">
    <!-- Stat Cards -->
    <el-row :gutter="16" class="stat-row">
      <el-col :xs="12" :sm="6" v-for="stat in statCards" :key="stat.key">
        <div class="stat-card" :style="{ borderTopColor: stat.color }">
          <div class="stat-icon" :style="{ background: stat.iconBg }">
            <el-icon :size="24" :style="{ color: stat.color }">
              <component :is="stat.icon" />
            </el-icon>
          </div>
          <div class="stat-info">
            <div class="stat-value">{{ stat.value }}</div>
            <div class="stat-label">{{ stat.label }}</div>
          </div>
          <div v-if="stat.trend !== undefined" class="stat-trend" :class="stat.trend >= 0 ? 'up' : 'down'">
            <el-icon><Top v-if="stat.trend >= 0" /><Bottom v-else /></el-icon>
            {{ Math.abs(stat.trend) }}%
          </div>
        </div>
      </el-col>
    </el-row>

    <!-- Resource Gauges -->
    <el-row :gutter="16" class="chart-row">
      <el-col :xs="24" :sm="8">
        <el-card shadow="never" class="chart-card">
          <template #header>
            <span class="card-title">CPU 使用率</span>
          </template>
          <GaugeChart :value="dashboard.systemMetrics?.cpuUsage || 0" title="CPU" height="220px" />
        </el-card>
      </el-col>
      <el-col :xs="24" :sm="8">
        <el-card shadow="never" class="chart-card">
          <template #header>
            <span class="card-title">内存使用率</span>
          </template>
          <GaugeChart :value="dashboard.systemMetrics?.memoryUsage || 0" title="内存" height="220px" />
        </el-card>
      </el-col>
      <el-col :xs="24" :sm="8">
        <el-card shadow="never" class="chart-card">
          <template #header>
            <span class="card-title">TPU 使用率</span>
          </template>
          <GaugeChart :value="dashboard.systemMetrics?.tpuUsage || 0" title="TPU" height="220px" />
        </el-card>
      </el-col>
    </el-row>

    <!-- Charts Row -->
    <el-row :gutter="16" class="chart-row">
      <el-col :xs="24" :sm="14">
        <el-card shadow="never" class="chart-card">
          <template #header>
            <span class="card-title">报警趋势（近7天）</span>
          </template>
          <LineChart
            :data="alertTrendData"
            :height="'280px'"
            :area-style="true"
          />
        </el-card>
      </el-col>
      <el-col :xs="24" :sm="10">
        <el-card shadow="never" class="chart-card">
          <template #header>
            <span class="card-title">任务状态分布</span>
          </template>
          <PieChart :data="taskStatusData" :height="'280px'" />
        </el-card>
      </el-col>
    </el-row>

    <!-- Recent Alerts + Device Status -->
    <el-row :gutter="16" class="chart-row">
      <el-col :xs="24" :sm="14">
        <el-card shadow="never" class="chart-card">
          <template #header>
            <div class="card-header-flex">
              <span class="card-title">最近报警</span>
              <el-button text type="primary" @click="$router.push('/alerts')">查看全部</el-button>
            </div>
          </template>
          <el-table :data="recentAlerts" size="small" stripe>
            <el-table-column prop="time" label="时间" width="160">
              <template #default="{ row }">
                {{ formatTime(row.time) }}
              </template>
            </el-table-column>
            <el-table-column prop="taskName" label="任务" width="120" show-overflow-tooltip />
            <el-table-column prop="type" label="类型" width="100" />
            <el-table-column prop="level" label="级别" width="80">
              <template #default="{ row }">
                <StatusBadge :type="row.level" size="small" />
              </template>
            </el-table-column>
            <el-table-column prop="message" label="消息" show-overflow-tooltip />
          </el-table>
        </el-card>
      </el-col>
      <el-col :xs="24" :sm="10">
        <el-card shadow="never" class="chart-card">
          <template #header>
            <span class="card-title">设备状态概览</span>
          </template>
          <div class="device-overview">
            <div class="device-stat-item" v-for="item in deviceStatusOverview" :key="item.label">
              <div class="device-count" :style="{ color: item.color }">{{ item.count }}</div>
              <div class="device-label">{{ item.label }}</div>
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, onUnmounted } from 'vue'
import { useMonitorStore } from '@/stores/monitor'
import { useDeviceStore } from '@/stores/device'
import { useTaskStore } from '@/stores/task'
import dayjs from 'dayjs'
import GaugeChart from '@/components/chart/GaugeChart.vue'
import LineChart from '@/components/chart/LineChart.vue'
import PieChart from '@/components/chart/PieChart.vue'
import StatusBadge from '@/components/common/StatusBadge.vue'

const monitorStore = useMonitorStore()
const deviceStore = useDeviceStore()
const taskStore = useTaskStore()

let refreshTimer: ReturnType<typeof setInterval> | null = null

const dashboard = computed(() => monitorStore.dashboardData || {
  totalDevices: 0,
  onlineDevices: 0,
  runningTasks: 0,
  todayAlerts: 0,
  systemMetrics: { cpuUsage: 0, memoryUsage: 0, tpuUsage: 0, memoryTotal: 0, memoryUsed: 0, gpuUsage: 0, uptime: 0, timestamp: '' },
  recentAlerts: [],
  taskStatusDistribution: {},
  alertTrend: [],
})

const statCards = computed(() => [
  {
    key: 'devices',
    label: '设备总数',
    value: dashboard.value.totalDevices,
    icon: 'VideoCamera',
    color: '#409EFF',
    iconBg: 'rgba(64, 158, 255, 0.15)',
    trend: 5,
  },
  {
    key: 'online',
    label: '在线设备',
    value: dashboard.value.onlineDevices,
    icon: 'Connection',
    color: '#67C23A',
    iconBg: 'rgba(103, 194, 58, 0.15)',
    trend: 2,
  },
  {
    key: 'tasks',
    label: '运行任务',
    value: dashboard.value.runningTasks,
    icon: 'List',
    color: '#E6A23C',
    iconBg: 'rgba(230, 162, 60, 0.15)',
    trend: -3,
  },
  {
    key: 'alerts',
    label: '今日报警',
    value: dashboard.value.todayAlerts,
    icon: 'Bell',
    color: '#F56C6C',
    iconBg: 'rgba(245, 108, 108, 0.15)',
    trend: 12,
  },
])

const alertTrendData = computed(() =>
  (dashboard.value.alertTrend || []).map((item) => ({
    name: item.date,
    value: item.count,
  }))
)

const taskStatusData = computed(() => {
  const dist = dashboard.value.taskStatusDistribution || {}
  return [
    { name: '运行中', value: dist.running || 0 },
    { name: '已停止', value: dist.stopped || 0 },
    { name: '已暂停', value: dist.paused || 0 },
    { name: '异常', value: dist.error || 0 },
  ].filter((d) => d.value > 0)
})

const recentAlerts = computed(() => (dashboard.value.recentAlerts || []).slice(0, 10))

const deviceStatusOverview = computed(() => {
  const devices = deviceStore.devices || []
  const online = devices.filter((d) => d.status === 'online').length
  const offline = devices.filter((d) => d.status === 'offline').length
  const error = devices.filter((d) => d.status === 'error').length
  return [
    { label: '在线', count: online, color: '#67C23A' },
    { label: '离线', count: offline, color: '#909399' },
    { label: '异常', count: error, color: '#F56C6C' },
  ]
})

function formatTime(time: string) {
  return dayjs(time).format('MM-DD HH:mm:ss')
}

async function loadData() {
  await Promise.all([
    monitorStore.fetchDashboard(),
    deviceStore.fetchDevices({ page: 1, pageSize: 100 }),
    taskStore.fetchTasks({ page: 1, pageSize: 100 }),
  ])
}

onMounted(() => {
  loadData()
  refreshTimer = setInterval(loadData, 30000)
})

onUnmounted(() => {
  if (refreshTimer) clearInterval(refreshTimer)
})
</script>

<style scoped lang="scss">
.dashboard-page {
  animation: fadeIn 0.3s ease;
}

.stat-row {
  margin-bottom: 16px;
}

.stat-card {
  background: #132f4c;
  border-radius: 8px;
  padding: 20px;
  display: flex;
  align-items: center;
  gap: 16px;
  border-top: 3px solid;
  transition: all 0.3s ease;
  cursor: default;

  &:hover {
    background: #1a3a5c;
    transform: translateY(-2px);
    box-shadow: 0 8px 24px rgba(0, 0, 0, 0.3);
  }
}

.stat-icon {
  width: 48px;
  height: 48px;
  border-radius: 12px;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.stat-info {
  flex: 1;
}

.stat-value {
  font-size: 24px;
  font-weight: 700;
  color: #e3e8ef;
  line-height: 1.2;
}

.stat-label {
  font-size: 13px;
  color: #78909c;
  margin-top: 4px;
}

.stat-trend {
  font-size: 12px;
  display: flex;
  align-items: center;
  gap: 2px;

  &.up {
    color: #67C23A;
  }

  &.down {
    color: #F56C6C;
  }
}

.chart-row {
  margin-bottom: 16px;
}

.chart-card {
  background: #132f4c;
  border: 1px solid #1e3a5f;
  border-radius: 8px;
  margin-bottom: 0;

  :deep(.el-card__header) {
    padding: 12px 20px;
    border-bottom: 1px solid #1e3a5f;
  }

  :deep(.el-card__body) {
    padding: 16px;
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

.device-overview {
  display: flex;
  flex-direction: column;
  gap: 16px;
  padding: 12px 0;
}

.device-stat-item {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 12px 16px;
  background: rgba(10, 25, 41, 0.5);
  border-radius: 8px;
  border: 1px solid #1e3a5f;
}

.device-count {
  font-size: 28px;
  font-weight: 700;
}

.device-label {
  font-size: 14px;
  color: #b0bec5;
}
</style>
