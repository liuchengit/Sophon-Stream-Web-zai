<template>
  <div class="firmware-page">
    <PageHeader title="固件管理" description="管理系统固件版本，支持升级和回滚">
      <template #actions>
        <el-button type="primary" :icon="Upload" @click="uploadDialogVisible = true">上传固件</el-button>
      </template>
    </PageHeader>

    <el-card shadow="never" class="table-card">
      <el-table :data="systemStore.firmwareVersions" v-loading="systemStore.loading" stripe>
        <el-table-column prop="version" label="版本号" width="120" />
        <el-table-column prop="description" label="描述" min-width="200" show-overflow-tooltip />
        <el-table-column prop="filePath" label="文件路径" min-width="180" show-overflow-tooltip />
        <el-table-column prop="checksum" label="校验和" width="140" show-overflow-tooltip />
        <el-table-column prop="status" label="状态" width="100">
          <template #default="{ row }">
            <el-tag :type="firmwareStatusTag[row.status] || 'info'" size="small">
              {{ firmwareStatusLabel[row.status] || row.status }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="installedAt" label="安装时间" width="160">
          <template #default="{ row }">{{ row.installedAt ? formatTime(row.installedAt) : '-' }}</template>
        </el-table-column>
        <el-table-column label="操作" width="180" fixed="right">
          <template #default="{ row }">
            <el-button
              v-if="row.status === 'available'"
              text type="success" size="small"
              @click="handleInstall(row)"
            >安装</el-button>
            <el-button
              v-if="row.status === 'installed'"
              text type="warning" size="small"
              @click="handleRollback(row)"
            >回滚</el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <!-- Upload Dialog -->
    <el-dialog v-model="uploadDialogVisible" title="上传固件" width="460px" :close-on-click-modal="false">
      <el-form :model="uploadForm" label-width="80px">
        <el-form-item label="固件路径">
          <el-input v-model="uploadForm.filePath" placeholder="/path/to/firmware.bin" />
        </el-form-item>
        <el-form-item label="版本号">
          <el-input v-model="uploadForm.version" placeholder="如 v2.0.0" />
        </el-form-item>
        <el-form-item label="描述">
          <el-input v-model="uploadForm.description" type="textarea" :rows="3" placeholder="固件更新说明" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="uploadDialogVisible = false">取消</el-button>
        <el-button type="primary">上传</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted } from 'vue'
import { Upload } from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useSystemStore } from '@/stores/system'
import { installFirmware } from '@/api/system'
import PageHeader from '@/components/common/PageHeader.vue'
import dayjs from 'dayjs'

const systemStore = useSystemStore()
const uploadDialogVisible = ref(false)

const uploadForm = reactive({
  filePath: '',
  version: '',
  description: '',
})

const firmwareStatusTag: Record<string, string> = {
  available: '', installed: 'success', installing: 'warning', failed: 'danger',
}

const firmwareStatusLabel: Record<string, string> = {
  available: '可用', installed: '已安装', installing: '安装中', failed: '安装失败',
}

function formatTime(time: string) {
  return dayjs(time).format('YYYY-MM-DD HH:mm:ss')
}

async function handleInstall(fw: any) {
  try {
    await ElMessageBox.confirm(`确定要安装固件 ${fw.version} 吗？安装期间系统将暂时不可用。`, '安装确认', { type: 'warning' })
    await installFirmware(fw.id)
    ElMessage.success('固件安装指令已发送')
    systemStore.fetchFirmwareVersions()
  } catch { /* cancelled */ }
}

async function handleRollback(fw: any) {
  try {
    await ElMessageBox.confirm(`确定要回滚到固件 ${fw.version} 吗？`, '回滚确认', { type: 'warning' })
    ElMessage.info('回滚功能开发中')
  } catch { /* cancelled */ }
}

onMounted(() => {
  systemStore.fetchFirmwareVersions()
})
</script>

<style scoped lang="scss">
.table-card {
  background: #132f4c;
  border: 1px solid #1e3a5f;
}
</style>
