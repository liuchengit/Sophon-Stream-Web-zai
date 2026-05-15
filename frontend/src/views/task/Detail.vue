<template>
  <div class="task-detail">
    <PageHeader :title="`任务详情 - ${task?.name || ''}`">
      <template #actions>
        <el-button @click="$router.back()">返回</el-button>
        <el-button type="primary" @click="$router.push(`/tasks/${taskId}/edit`)">编辑</el-button>
      </template>
    </PageHeader>

    <el-row :gutter="16">
      <!-- Task Info -->
      <el-col :xs="24" :sm="12">
        <el-card shadow="never" class="detail-card">
          <template #header>
            <div class="card-header-flex">
              <span class="card-title">任务信息</span>
              <StatusBadge :type="task?.status || 'stopped'" />
            </div>
          </template>
          <el-descriptions :column="2" border size="small">
            <el-descriptions-item label="名称">{{ task?.name }}</el-descriptions-item>
            <el-descriptions-item label="类型">{{ task?.type || '-' }}</el-descriptions-item>
            <el-descriptions-item label="关联设备">{{ task?.deviceName || '-' }}</el-descriptions-item>
            <el-descriptions-item label="FPS">{{ task?.fps || '-' }}</el-descriptions-item>
            <el-descriptions-item label="创建时间" :span="2">{{ formatTime(task?.createdAt) }}</el-descriptions-item>
            <el-descriptions-item label="描述" :span="2">{{ task?.description || '-' }}</el-descriptions-item>
          </el-descriptions>
          <div class="task-actions">
            <el-button v-if="task?.status === 'stopped' || task?.status === 'error'" type="success" @click="handleStart">启动</el-button>
            <el-button v-if="task?.status === 'running'" type="warning" @click="handlePause">暂停</el-button>
            <el-button v-if="task?.status === 'paused'" type="success" @click="handleResume">恢复</el-button>
            <el-button v-if="task?.status === 'running' || task?.status === 'paused'" type="danger" @click="handleStop">停止</el-button>
          </div>
        </el-card>

        <!-- Real-time Metrics -->
        <el-card shadow="never" class="detail-card mt-16">
          <template #header>
            <span class="card-title">实时指标</span>
          </template>
          <div class="metrics-grid">
            <div class="metric-item">
              <div class="metric-value">{{ metrics?.fps?.toFixed(1) || '-' }}</div>
              <div class="metric-label">FPS</div>
            </div>
            <div class="metric-item">
              <div class="metric-value">{{ metrics?.latency?.toFixed(0) || '-' }} ms</div>
              <div class="metric-label">延迟</div>
            </div>
            <div class="metric-item">
              <div class="metric-value">{{ metrics?.throughput?.toFixed(0) || '-' }}</div>
              <div class="metric-label">吞吐量</div>
            </div>
            <div class="metric-item">
              <div class="metric-value">{{ metrics?.detections || '-' }}</div>
              <div class="metric-label">检测数</div>
            </div>
          </div>
        </el-card>
      </el-col>

      <!-- Pipeline Visualization -->
      <el-col :xs="24" :sm="12">
        <el-card shadow="never" class="detail-card">
          <template #header>
            <span class="card-title">Pipeline 流程</span>
          </template>
          <div class="pipeline-viz">
            <div v-for="(node, idx) in pipelineNodes" :key="idx" class="pipeline-node">
              <div class="node-box" :class="node.type">
                <el-icon :size="20"><component :is="node.icon" /></el-icon>
                <span>{{ node.label }}</span>
              </div>
              <div v-if="idx < pipelineNodes.length - 1" class="pipeline-arrow">
                <el-icon><Right /></el-icon>
              </div>
            </div>
          </div>
        </el-card>

        <!-- Config Viewer -->
        <el-card shadow="never" class="detail-card mt-16">
          <template #header>
            <span class="card-title">配置信息</span>
          </template>
          <JsonEditor :model-value="task?.config || {}" title="任务配置" :rows="8" @update:model-value="handleConfigUpdate" />
        </el-card>
      </el-col>
    </el-row>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, onUnmounted, ref } from 'vue'
import { useRoute } from 'vue-router'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useTaskStore } from '@/stores/task'
import dayjs from 'dayjs'
import PageHeader from '@/components/common/PageHeader.vue'
import StatusBadge from '@/components/common/StatusBadge.vue'
import JsonEditor from '@/components/common/JsonEditor.vue'

const route = useRoute()
const taskStore = useTaskStore()

const taskId = computed(() => Number(route.params.id))
const task = computed(() => taskStore.currentTask)
const metrics = computed(() => taskStore.currentMetrics)

let metricsTimer: ReturnType<typeof setInterval> | null = null

const pipelineNodes = computed(() => {
  const nodes = [
    { type: 'source', label: '视频源', icon: 'VideoCamera' },
    { type: 'decode', label: '解码', icon: 'Film' },
    { type: 'detect', label: '检测', icon: 'View' },
    { type: 'osd', label: 'OSD', icon: 'PictureFilled' },
    { type: 'encode', label: '编码', icon: 'VideoPlay' },
    { type: 'sink', label: '输出', icon: 'Upload' },
  ]
  return nodes
})

function formatTime(time?: string) {
  if (!time) return '-'
  return dayjs(time).format('YYYY-MM-DD HH:mm:ss')
}

async function handleStart() {
  try {
    await taskStore.startTask(taskId.value)
    ElMessage.success('任务已启动')
  } catch { /* handled */ }
}

async function handlePause() {
  try {
    await taskStore.pauseTask(taskId.value)
    ElMessage.success('任务已暂停')
  } catch { /* handled */ }
}

async function handleResume() {
  try {
    await taskStore.resumeTask(taskId.value)
    ElMessage.success('任务已恢复')
  } catch { /* handled */ }
}

async function handleStop() {
  try {
    await ElMessageBox.confirm('确定要停止此任务吗？', '停止任务', { type: 'warning' })
    await taskStore.stopTask(taskId.value)
    ElMessage.success('任务已停止')
  } catch { /* cancelled */ }
}

async function handleConfigUpdate(config: Record<string, any>) {
  try {
    await taskStore.updateTaskConfig(taskId.value, config)
    ElMessage.success('配置已更新')
  } catch { /* handled */ }
}

onMounted(async () => {
  await taskStore.fetchTask(taskId.value)
  metricsTimer = setInterval(() => {
    taskStore.fetchTaskMetrics(taskId.value)
  }, 3000)
})

onUnmounted(() => {
  if (metricsTimer) clearInterval(metricsTimer)
})
</script>

<style scoped lang="scss">
.detail-card {
  background: #132f4c;
  border: 1px solid #1e3a5f;

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

.task-actions {
  margin-top: 16px;
  display: flex;
  gap: 8px;
}

.metrics-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 16px;
}

.metric-item {
  background: rgba(10, 25, 41, 0.5);
  border: 1px solid #1e3a5f;
  border-radius: 8px;
  padding: 16px;
  text-align: center;
}

.metric-value {
  font-size: 24px;
  font-weight: 700;
  color: #409EFF;
}

.metric-label {
  font-size: 12px;
  color: #78909c;
  margin-top: 4px;
}

.pipeline-viz {
  display: flex;
  align-items: center;
  flex-wrap: wrap;
  gap: 8px;
  padding: 16px;
}

.pipeline-node {
  display: flex;
  align-items: center;
  gap: 8px;
}

.node-box {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 4px;
  padding: 12px 16px;
  border-radius: 8px;
  font-size: 12px;
  color: #e3e8ef;
  min-width: 70px;

  &.source { background: rgba(64, 158, 255, 0.15); border: 1px solid rgba(64, 158, 255, 0.3); }
  &.decode { background: rgba(103, 194, 58, 0.15); border: 1px solid rgba(103, 194, 58, 0.3); }
  &.detect { background: rgba(230, 162, 60, 0.15); border: 1px solid rgba(230, 162, 60, 0.3); }
  &.osd { background: rgba(144, 147, 153, 0.15); border: 1px solid rgba(144, 147, 153, 0.3); }
  &.encode { background: rgba(103, 194, 58, 0.15); border: 1px solid rgba(103, 194, 58, 0.3); }
  &.sink { background: rgba(245, 108, 108, 0.15); border: 1px solid rgba(245, 108, 108, 0.3); }
}

.pipeline-arrow {
  color: #546e7a;
  font-size: 16px;
}
</style>
