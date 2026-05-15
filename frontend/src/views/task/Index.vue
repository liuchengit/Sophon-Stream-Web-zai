<template>
  <div class="task-page">
    <PageHeader title="任务管理" description="管理视频分析任务，包括创建、启停、配置等">
      <template #actions>
        <el-button type="primary" :icon="Plus" @click="$router.push('/tasks/create')">创建任务</el-button>
      </template>
    </PageHeader>

    <SearchBar v-model="searchKeyword" placeholder="搜索任务名称" @search="handleSearch" @reset="handleReset">
      <template #filters>
        <el-select v-model="filterStatus" placeholder="状态" clearable style="width: 120px" @change="handleSearch">
          <el-option label="运行中" value="running" />
          <el-option label="已停止" value="stopped" />
          <el-option label="已暂停" value="paused" />
          <el-option label="异常" value="error" />
        </el-select>
      </template>
    </SearchBar>

    <el-card shadow="never" class="table-card">
      <el-table :data="taskStore.tasks" v-loading="taskStore.loading" stripe @selection-change="handleSelectionChange">
        <el-table-column type="selection" width="40" />
        <el-table-column prop="name" label="名称" min-width="140" show-overflow-tooltip />
        <el-table-column prop="deviceName" label="关联设备" width="120" show-overflow-tooltip />
        <el-table-column label="算法" min-width="140" show-overflow-tooltip>
          <template #default="{ row }">
            {{ row.algorithmNames?.join(', ') || '-' }}
          </template>
        </el-table-column>
        <el-table-column prop="status" label="状态" width="100">
          <template #default="{ row }">
            <StatusBadge :type="row.status" />
          </template>
        </el-table-column>
        <el-table-column prop="fps" label="FPS" width="80">
          <template #default="{ row }">
            <span :class="{ 'fps-good': row.fps > 15, 'fps-warn': row.fps > 0 && row.fps <= 15, 'fps-bad': row.fps === 0 }">
              {{ row.fps || '-' }}
            </span>
          </template>
        </el-table-column>
        <el-table-column prop="createdAt" label="创建时间" width="160">
          <template #default="{ row }">
            {{ formatTime(row.createdAt) }}
          </template>
        </el-table-column>
        <el-table-column label="操作" width="300" fixed="right">
          <template #default="{ row }">
            <el-button text type="primary" size="small" @click="$router.push(`/tasks/${row.id}`)">查看</el-button>
            <el-button
              v-if="row.status === 'stopped' || row.status === 'error'"
              text type="success" size="small"
              @click="handleStart(row)"
            >启动</el-button>
            <el-button
              v-if="row.status === 'running'"
              text type="warning" size="small"
              @click="handlePause(row)"
            >暂停</el-button>
            <el-button
              v-if="row.status === 'paused'"
              text type="success" size="small"
              @click="handleResume(row)"
            >恢复</el-button>
            <el-button
              v-if="row.status === 'running' || row.status === 'paused'"
              text type="danger" size="small"
              @click="handleStop(row)"
            >停止</el-button>
            <el-button text type="primary" size="small" @click="$router.push(`/tasks/${row.id}/edit`)">编辑</el-button>
            <el-button text type="primary" size="small" @click="handleDelete(row)">删除</el-button>
          </template>
        </el-table-column>
      </el-table>

      <div class="table-footer">
        <div class="batch-actions">
          <el-dropdown v-if="selectedTasks.length > 0" @command="handleBatchCommand">
            <el-button size="small">
              批量操作 ({{ selectedTasks.length }}) <el-icon><ArrowDown /></el-icon>
            </el-button>
            <template #dropdown>
              <el-dropdown-menu>
                <el-dropdown-item command="start">批量启动</el-dropdown-item>
                <el-dropdown-item command="stop">批量停止</el-dropdown-item>
                <el-dropdown-item command="delete" divided>批量删除</el-dropdown-item>
              </el-dropdown-menu>
            </template>
          </el-dropdown>
        </div>
        <el-pagination
          v-model:current-page="currentPage"
          v-model:page-size="pageSize"
          :total="taskStore.total"
          :page-sizes="[10, 20, 50, 100]"
          layout="total, sizes, prev, pager, next, jumper"
          @size-change="handlePageChange"
          @current-change="handlePageChange"
        />
      </div>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { Plus, ArrowDown } from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useTaskStore } from '@/stores/task'
import PageHeader from '@/components/common/PageHeader.vue'
import SearchBar from '@/components/common/SearchBar.vue'
import StatusBadge from '@/components/common/StatusBadge.vue'
import type { Task } from '@/api/task'
import dayjs from 'dayjs'

const taskStore = useTaskStore()
const searchKeyword = ref('')
const filterStatus = ref('')
const currentPage = ref(1)
const pageSize = ref(20)
const selectedTasks = ref<Task[]>([])

function formatTime(time: string) {
  return dayjs(time).format('YYYY-MM-DD HH:mm')
}

function handleSelectionChange(rows: Task[]) {
  selectedTasks.value = rows
}

function handleSearch() {
  currentPage.value = 1
  loadTasks()
}

function handleReset() {
  filterStatus.value = ''
  searchKeyword.value = ''
  currentPage.value = 1
  loadTasks()
}

function handlePageChange() {
  loadTasks()
}

async function loadTasks() {
  const params: Record<string, any> = {
    page: currentPage.value,
    pageSize: pageSize.value,
  }
  if (searchKeyword.value) params.keyword = searchKeyword.value
  if (filterStatus.value) params.status = filterStatus.value
  taskStore.setPagination(currentPage.value, pageSize.value)
  await taskStore.fetchTasks(params)
}

async function handleStart(task: Task) {
  try {
    await taskStore.startTask(task.id)
    ElMessage.success(`任务 ${task.name} 启动成功`)
  } catch { /* handled */ }
}

async function handlePause(task: Task) {
  try {
    await taskStore.pauseTask(task.id)
    ElMessage.success(`任务 ${task.name} 已暂停`)
  } catch { /* handled */ }
}

async function handleResume(task: Task) {
  try {
    await taskStore.resumeTask(task.id)
    ElMessage.success(`任务 ${task.name} 已恢复`)
  } catch { /* handled */ }
}

async function handleStop(task: Task) {
  try {
    await ElMessageBox.confirm(`确定要停止任务 ${task.name} 吗？`, '停止任务', { type: 'warning' })
    await taskStore.stopTask(task.id)
    ElMessage.success(`任务 ${task.name} 已停止`)
  } catch { /* cancelled */ }
}

async function handleDelete(task: Task) {
  try {
    await ElMessageBox.confirm(`确定要删除任务 ${task.name} 吗？此操作不可恢复。`, '删除确认', { type: 'danger' })
    await taskStore.deleteTask(task.id)
    ElMessage.success('任务已删除')
  } catch { /* cancelled */ }
}

async function handleBatchCommand(command: string) {
  const ids = selectedTasks.value.map((t) => t.id)
  try {
    if (command === 'delete') {
      await ElMessageBox.confirm(`确定要删除选中的 ${ids.length} 个任务吗？`, '批量删除', { type: 'danger' })
      for (const id of ids) {
        await taskStore.deleteTask(id)
      }
      ElMessage.success('批量删除成功')
    } else if (command === 'start') {
      for (const id of ids) await taskStore.startTask(id)
      ElMessage.success('批量启动成功')
    } else if (command === 'stop') {
      for (const id of ids) await taskStore.stopTask(id)
      ElMessage.success('批量停止成功')
    }
    loadTasks()
  } catch { /* cancelled or error */ }
}

onMounted(() => {
  loadTasks()
})
</script>

<style scoped lang="scss">
.table-card {
  background: #132f4c;
  border: 1px solid #1e3a5f;
}

.table-footer {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-top: 16px;
}

.batch-actions {
  display: flex;
  gap: 8px;
}

.fps-good { color: #67C23A; font-weight: 600; }
.fps-warn { color: #E6A23C; font-weight: 600; }
.fps-bad { color: #909399; }
</style>
