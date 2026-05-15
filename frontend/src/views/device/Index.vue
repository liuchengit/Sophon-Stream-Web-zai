<template>
  <div class="device-page">
    <PageHeader title="设备管理" description="管理视频源设备，包括IPC、NVR、RTSP流等">
      <template #actions>
        <el-button type="primary" :icon="Plus" @click="showDeviceForm(null)">添加设备</el-button>
      </template>
    </PageHeader>

    <SearchBar v-model="searchKeyword" placeholder="搜索设备名称/地址" @search="handleSearch" @reset="handleReset">
      <template #filters>
        <el-select v-model="filterType" placeholder="设备类型" clearable style="width: 140px" @change="handleSearch">
          <el-option label="IPC" value="IPC" />
          <el-option label="NVR" value="NVR" />
          <el-option label="RTSP" value="RTSP" />
          <el-option label="FILE" value="FILE" />
          <el-option label="GB28181" value="GB28181" />
        </el-select>
        <el-select v-model="filterStatus" placeholder="状态" clearable style="width: 120px" @change="handleSearch">
          <el-option label="在线" value="online" />
          <el-option label="离线" value="offline" />
          <el-option label="异常" value="error" />
        </el-select>
      </template>
    </SearchBar>

    <el-card shadow="never" class="table-card">
      <el-table :data="deviceStore.devices" v-loading="deviceStore.loading" stripe>
        <el-table-column prop="name" label="名称" min-width="140" show-overflow-tooltip />
        <el-table-column prop="type" label="类型" width="100">
          <template #default="{ row }">
            <el-tag size="small" :type="typeTagMap[row.type]">{{ row.type }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="streamUrl" label="流地址" min-width="200" show-overflow-tooltip />
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
        <el-table-column prop="location" label="位置" width="120" show-overflow-tooltip />
        <el-table-column label="操作" width="260" fixed="right">
          <template #default="{ row }">
            <el-button text type="primary" size="small" @click="$router.push(`/devices/${row.id}`)">查看</el-button>
            <el-button text type="primary" size="small" @click="showDeviceForm(row)">编辑</el-button>
            <el-button
              v-if="row.status === 'offline'"
              text type="success" size="small"
              @click="handleStartStream(row)"
            >启动</el-button>
            <el-button
              v-else-if="row.status === 'online'"
              text type="warning" size="small"
              @click="handleStopStream(row)"
            >停止</el-button>
            <el-button text type="danger" size="small" @click="handleDeleteDevice(row)">删除</el-button>
          </template>
        </el-table-column>
      </el-table>

      <div class="pagination-wrap">
        <el-pagination
          v-model:current-page="currentPage"
          v-model:page-size="pageSize"
          :total="deviceStore.total"
          :page-sizes="[10, 20, 50, 100]"
          layout="total, sizes, prev, pager, next, jumper"
          @size-change="handlePageChange"
          @current-change="handlePageChange"
        />
      </div>
    </el-card>

    <DeviceForm
      v-model="formVisible"
      :device="editingDevice"
      @saved="handleFormSaved"
    />
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { Plus } from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useDeviceStore } from '@/stores/device'
import PageHeader from '@/components/common/PageHeader.vue'
import SearchBar from '@/components/common/SearchBar.vue'
import StatusBadge from '@/components/common/StatusBadge.vue'
import DeviceForm from './DeviceForm.vue'
import type { Device } from '@/api/device'

const deviceStore = useDeviceStore()
const searchKeyword = ref('')
const filterType = ref('')
const filterStatus = ref('')
const currentPage = ref(1)
const pageSize = ref(20)
const formVisible = ref(false)
const editingDevice = ref<Device | null>(null)

const typeTagMap: Record<string, string> = {
  IPC: '',
  NVR: 'success',
  RTSP: 'warning',
  FILE: 'info',
  GB28181: 'danger',
}

function showDeviceForm(device: Device | null) {
  editingDevice.value = device
  formVisible.value = true
}

function handleFormSaved() {
  formVisible.value = false
  loadDevices()
}

async function handleSearch() {
  currentPage.value = 1
  loadDevices()
}

function handleReset() {
  filterType.value = ''
  filterStatus.value = ''
  searchKeyword.value = ''
  currentPage.value = 1
  loadDevices()
}

function handlePageChange() {
  loadDevices()
}

async function loadDevices() {
  const params: Record<string, any> = {
    page: currentPage.value,
    pageSize: pageSize.value,
  }
  if (searchKeyword.value) params.keyword = searchKeyword.value
  if (filterType.value) params.type = filterType.value
  if (filterStatus.value) params.status = filterStatus.value
  deviceStore.setPagination(currentPage.value, pageSize.value)
  await deviceStore.fetchDevices(params)
}

async function handleStartStream(device: Device) {
  try {
    await deviceStore.startStream(device.id)
    ElMessage.success(`设备 ${device.name} 启动成功`)
  } catch {
    // handled by interceptor
  }
}

async function handleStopStream(device: Device) {
  try {
    await ElMessageBox.confirm(`确定要停止设备 ${device.name} 吗？`, '停止设备', { type: 'warning' })
    await deviceStore.stopStream(device.id)
    ElMessage.success(`设备 ${device.name} 已停止`)
  } catch {
    // cancelled or error
  }
}

async function handleDeleteDevice(device: Device) {
  try {
    await ElMessageBox.confirm(`确定要删除设备 ${device.name} 吗？此操作不可恢复。`, '删除确认', { type: 'danger' })
    await deviceStore.deleteDevice(device.id)
    ElMessage.success('设备已删除')
  } catch {
    // cancelled or error
  }
}

onMounted(() => {
  loadDevices()
})
</script>

<style scoped lang="scss">
.table-card {
  background: #132f4c;
  border: 1px solid #1e3a5f;
}

.pagination-wrap {
  display: flex;
  justify-content: flex-end;
  margin-top: 16px;
}

.fps-good { color: #67C23A; font-weight: 600; }
.fps-warn { color: #E6A23C; font-weight: 600; }
.fps-bad { color: #909399; }
</style>
