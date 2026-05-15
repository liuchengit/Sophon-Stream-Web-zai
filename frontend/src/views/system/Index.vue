<template>
  <div class="system-page">
    <PageHeader title="系统设置" description="查看系统信息和管理系统配置" />

    <el-row :gutter="16">
      <!-- System Info -->
      <el-col :xs="24" :sm="12">
        <el-card shadow="never" class="info-card">
          <template #header>
            <span class="card-title">系统信息</span>
          </template>
          <el-descriptions :column="1" border size="small" v-loading="systemStore.loading">
            <el-descriptions-item label="系统版本">{{ sysInfo?.version || '-' }}</el-descriptions-item>
            <el-descriptions-item label="运行时间">{{ formatUptime(sysInfo?.uptime) }}</el-descriptions-item>
            <el-descriptions-item label="CPU型号">{{ sysInfo?.cpuModel || '-' }}</el-descriptions-item>
            <el-descriptions-item label="CPU核心">{{ sysInfo?.cpuCores || '-' }} 核</el-descriptions-item>
            <el-descriptions-item label="内存">{{ formatBytes(sysInfo?.memoryTotal) }}</el-descriptions-item>
            <el-descriptions-item label="TPU型号">{{ sysInfo?.tpuModel || '-' }}</el-descriptions-item>
            <el-descriptions-item label="TPU数量">{{ sysInfo?.tpuCount || '-' }}</el-descriptions-item>
            <el-descriptions-item label="操作系统">{{ sysInfo?.os || '-' }}</el-descriptions-item>
            <el-descriptions-item label="主机名">{{ sysInfo?.hostname || '-' }}</el-descriptions-item>
          </el-descriptions>
        </el-card>
      </el-col>

      <!-- System Config -->
      <el-col :xs="24" :sm="12">
        <el-card shadow="never" class="info-card">
          <template #header>
            <div class="card-header-flex">
              <span class="card-title">系统配置</span>
              <el-button type="primary" size="small" :loading="configSaving" @click="handleSaveConfig">保存配置</el-button>
            </div>
          </template>
          <div v-loading="systemStore.loading" class="config-editor">
            <div v-for="(value, key) in editableConfig" :key="key" class="config-item">
              <label class="config-key">{{ key }}</label>
              <el-input
                v-if="typeof value === 'string' || typeof value === 'number'"
                v-model="editableConfig[key]"
                size="small"
              />
              <el-switch
                v-else-if="typeof value === 'boolean'"
                v-model="editableConfig[key]"
              />
            </div>
          </div>
        </el-card>

        <!-- Quick Actions -->
        <el-card shadow="never" class="info-card mt-16">
          <template #header>
            <span class="card-title">快捷操作</span>
          </template>
          <div class="quick-actions">
            <el-button @click="handleBackup" :loading="backupLoading">
              <el-icon><Download /></el-icon>备份数据
            </el-button>
            <el-button @click="handleRestore">
              <el-icon><Upload /></el-icon>恢复数据
            </el-button>
            <el-button type="danger" @click="handleReboot">
              <el-icon><RefreshRight /></el-icon>重启系统
            </el-button>
          </div>
        </el-card>
      </el-col>
    </el-row>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useSystemStore } from '@/stores/system'
import { backupSystem, restoreSystem, rebootSystem } from '@/api/system'
import PageHeader from '@/components/common/PageHeader.vue'

const systemStore = useSystemStore()
const configSaving = ref(false)
const backupLoading = ref(false)

const sysInfo = computed(() => systemStore.systemInfo)

const editableConfig = reactive<Record<string, any>>({})

function formatUptime(seconds?: number) {
  if (!seconds) return '-'
  const d = Math.floor(seconds / 86400)
  const h = Math.floor((seconds % 86400) / 3600)
  const m = Math.floor((seconds % 3600) / 60)
  return `${d}天 ${h}小时 ${m}分钟`
}

function formatBytes(bytes?: number) {
  if (!bytes) return '-'
  if (bytes < 1024 * 1024 * 1024) return `${(bytes / 1024 / 1024).toFixed(0)} MB`
  return `${(bytes / 1024 / 1024 / 1024).toFixed(1)} GB`
}

async function handleSaveConfig() {
  configSaving.value = true
  try {
    await systemStore.updateSystemConfig({ ...editableConfig })
    ElMessage.success('配置已保存')
  } finally {
    configSaving.value = false
  }
}

async function handleBackup() {
  backupLoading.value = true
  try {
    const result = await backupSystem()
    ElMessage.success(`备份成功: ${result.path}`)
  } catch {
    // handled
  } finally {
    backupLoading.value = false
  }
}

async function handleRestore() {
  try {
    const { value } = await ElMessageBox.prompt('请输入备份文件路径', '恢复数据', {
      confirmButtonText: '恢复',
      cancelButtonText: '取消',
      inputPattern: /.+/,
      inputErrorMessage: '请输入有效路径',
    })
    await restoreSystem({ path: value })
    ElMessage.success('数据恢复成功')
  } catch { /* cancelled */ }
}

async function handleReboot() {
  try {
    await ElMessageBox.confirm('确定要重启系统吗？所有运行中的任务将被停止。', '重启确认', { type: 'danger' })
    await rebootSystem()
    ElMessage.success('系统重启指令已发送')
  } catch { /* cancelled */ }
}

onMounted(async () => {
  await Promise.all([
    systemStore.fetchSystemInfo(),
    systemStore.fetchSystemConfig(),
  ])
  if (systemStore.systemConfig) {
    Object.assign(editableConfig, systemStore.systemConfig)
  }
})
</script>

<style scoped lang="scss">
.info-card {
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

.config-editor {
  max-height: 400px;
  overflow-y: auto;
}

.config-item {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 8px 0;
  border-bottom: 1px solid #1e3a5f;

  &:last-child {
    border-bottom: none;
  }
}

.config-key {
  min-width: 140px;
  font-size: 13px;
  color: #b0bec5;
  font-family: 'Consolas', monospace;
}

.quick-actions {
  display: flex;
  gap: 12px;
  flex-wrap: wrap;
}
</style>
