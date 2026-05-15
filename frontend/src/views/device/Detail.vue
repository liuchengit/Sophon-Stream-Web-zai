<template>
  <div class="device-detail">
    <PageHeader :title="`设备详情 - ${device?.name || ''}`">
      <template #actions>
        <el-button @click="$router.back()">返回</el-button>
        <el-button type="primary" @click="$router.push('/devices')">设备列表</el-button>
      </template>
    </PageHeader>

    <el-row :gutter="16">
      <!-- Device Info -->
      <el-col :xs="24" :sm="12">
        <el-card shadow="never" class="detail-card">
          <template #header>
            <div class="card-header-flex">
              <span class="card-title">设备信息</span>
              <StatusBadge :type="device?.status || 'offline'" />
            </div>
          </template>
          <el-descriptions :column="2" border size="small">
            <el-descriptions-item label="名称">{{ device?.name }}</el-descriptions-item>
            <el-descriptions-item label="类型">
              <el-tag size="small">{{ device?.type }}</el-tag>
            </el-descriptions-item>
            <el-descriptions-item label="协议">{{ device?.protocol }}</el-descriptions-item>
            <el-descriptions-item label="分辨率">{{ device?.resolution || '-' }}</el-descriptions-item>
            <el-descriptions-item label="FPS">{{ device?.fps || '-' }}</el-descriptions-item>
            <el-descriptions-item label="编码">{{ device?.codec || '-' }}</el-descriptions-item>
            <el-descriptions-item label="位置">{{ device?.location || '-' }}</el-descriptions-item>
            <el-descriptions-item label="设备ID" v-if="device?.type === 'GB28181'">{{ device?.deviceId || '-' }}</el-descriptions-item>
            <el-descriptions-item label="创建时间" :span="2">{{ formatTime(device?.createdAt) }}</el-descriptions-item>
            <el-descriptions-item label="流地址" :span="2">{{ device?.streamUrl }}</el-descriptions-item>
            <el-descriptions-item label="描述" :span="2">{{ device?.description || '-' }}</el-descriptions-item>
          </el-descriptions>
          <div class="device-actions">
            <el-button
              v-if="device?.status === 'offline'"
              type="success"
              @click="handleStart"
            >启动</el-button>
            <el-button
              v-if="device?.status === 'online'"
              type="warning"
              @click="handleStop"
            >停止</el-button>
            <el-button @click="$router.push('/devices')">编辑</el-button>
          </div>
        </el-card>
      </el-col>

      <!-- Stream Preview -->
      <el-col :xs="24" :sm="12">
        <el-card shadow="never" class="detail-card">
          <template #header>
            <span class="card-title">流预览</span>
          </template>
          <div class="stream-preview">
            <div class="preview-placeholder">
              <el-icon :size="48"><VideoCamera /></el-icon>
              <p>视频流预览</p>
              <p class="preview-hint">连接设备后可查看实时画面</p>
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- Metrics Chart -->
    <el-row :gutter="16" class="mt-16">
      <el-col :span="24">
        <el-card shadow="never" class="detail-card">
          <template #header>
            <span class="card-title">设备指标</span>
          </template>
          <LineChart
            :x-axis="metricsTimeLabels"
            :series="metricsSeries"
            :height="'250px'"
            :area-style="true"
          />
        </el-card>
      </el-col>
    </el-row>

    <!-- Linked Tasks -->
    <el-row :gutter="16" class="mt-16">
      <el-col :span="24">
        <el-card shadow="never" class="detail-card">
          <template #header>
            <span class="card-title">关联任务</span>
          </template>
          <el-table :data="linkedTasks" size="small" stripe>
            <el-table-column prop="name" label="任务名称" />
            <el-table-column prop="algorithmNames" label="算法" show-overflow-tooltip />
            <el-table-column prop="status" label="状态" width="100">
              <template #default="{ row }">
                <StatusBadge :type="row.status" size="small" />
              </template>
            </el-table-column>
            <el-table-column prop="fps" label="FPS" width="80" />
            <el-table-column label="操作" width="80">
              <template #default="{ row }">
                <el-button text type="primary" size="small" @click="$router.push(`/tasks/${row.id}`)">查看</el-button>
              </template>
            </el-table-column>
          </el-table>
        </el-card>
      </el-col>
    </el-row>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useDeviceStore } from '@/stores/device'
import { useTaskStore } from '@/stores/task'
import dayjs from 'dayjs'
import PageHeader from '@/components/common/PageHeader.vue'
import StatusBadge from '@/components/common/StatusBadge.vue'
import LineChart from '@/components/chart/LineChart.vue'

const route = useRoute()
const deviceStore = useDeviceStore()
const taskStore = useTaskStore()

const deviceId = computed(() => Number(route.params.id))
const device = computed(() => deviceStore.currentDevice)
const linkedTasks = computed(() => taskStore.tasks.filter((t) => t.deviceId === deviceId.value))

// Mock metrics data for display
const metricsTimeLabels = computed(() => {
  const labels: string[] = []
  for (let i = 29; i >= 0; i--) {
    labels.push(dayjs().subtract(i, 'minute').format('HH:mm'))
  }
  return labels
})

const metricsSeries = computed(() => [
  {
    name: 'FPS',
    data: Array.from({ length: 30 }, () => Math.floor(Math.random() * 10 + 20)),
  },
])

function formatTime(time?: string) {
  if (!time) return '-'
  return dayjs(time).format('YYYY-MM-DD HH:mm:ss')
}

async function handleStart() {
  try {
    await deviceStore.startStream(deviceId.value)
    ElMessage.success('设备已启动')
  } catch { /* handled */ }
}

async function handleStop() {
  try {
    await ElMessageBox.confirm('确定要停止此设备吗？', '停止设备', { type: 'warning' })
    await deviceStore.stopStream(deviceId.value)
    ElMessage.success('设备已停止')
  } catch { /* cancelled */ }
}

onMounted(async () => {
  await Promise.all([
    deviceStore.fetchDevice(deviceId.value),
    taskStore.fetchTasks({ page: 1, pageSize: 100 }),
  ])
})
</script>

<style scoped lang="scss">
.detail-card {
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

.stream-preview {
  width: 100%;
  height: 280px;
  background: #0a1929;
  border-radius: 4px;
  overflow: hidden;
  display: flex;
  align-items: center;
  justify-content: center;
}

.preview-placeholder {
  text-align: center;
  color: #546e7a;

  p {
    margin-top: 8px;
    font-size: 14px;
  }

  .preview-hint {
    font-size: 12px;
    color: #3a506b;
  }
}

.device-actions {
  margin-top: 16px;
  display: flex;
  gap: 8px;
}
</style>
