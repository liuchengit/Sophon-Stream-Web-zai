<template>
  <div class="alert-page">
    <PageHeader title="报警管理" description="查看和管理系统报警事件">
      <template #actions>
        <el-button @click="$router.push('/alerts/rules')">报警规则</el-button>
      </template>
    </PageHeader>

    <!-- Alert Stats -->
    <el-row :gutter="16" class="stats-row">
      <el-col :xs="6" v-for="stat in alertStats" :key="stat.label">
        <div class="stat-card" :style="{ borderTopColor: stat.color }">
          <div class="stat-value" :style="{ color: stat.color }">{{ stat.value }}</div>
          <div class="stat-label">{{ stat.label }}</div>
        </div>
      </el-col>
    </el-row>

    <SearchBar v-model="searchKeyword" placeholder="搜索报警消息" @search="handleSearch" @reset="handleReset">
      <template #filters>
        <el-select v-model="filterLevel" placeholder="级别" clearable style="width: 110px" @change="handleSearch">
          <el-option label="信息" value="info" />
          <el-option label="警告" value="warning" />
          <el-option label="严重" value="critical" />
        </el-select>
        <el-select v-model="filterAck" placeholder="确认状态" clearable style="width: 120px" @change="handleSearch">
          <el-option label="未确认" value="false" />
          <el-option label="已确认" value="true" />
        </el-select>
        <el-date-picker
          v-model="dateRange"
          type="daterange"
          range-separator="至"
          start-placeholder="开始日期"
          end-placeholder="结束日期"
          size="default"
          style="width: 260px"
          @change="handleSearch"
        />
      </template>
    </SearchBar>

    <el-card shadow="never" class="table-card">
      <div class="batch-bar" v-if="selectedAlerts.length > 0">
        <el-button type="primary" size="small" @click="handleBatchAck">批量确认 ({{ selectedAlerts.length }})</el-button>
      </div>
      <el-table :data="alertStore.alerts" v-loading="alertStore.loading" stripe @selection-change="handleSelectionChange">
        <el-table-column type="selection" width="40" />
        <el-table-column prop="createdAt" label="时间" width="160">
          <template #default="{ row }">{{ formatTime(row.createdAt) }}</template>
        </el-table-column>
        <el-table-column prop="taskName" label="任务" width="120" show-overflow-tooltip />
        <el-table-column prop="type" label="类型" width="100" />
        <el-table-column prop="level" label="级别" width="80">
          <template #default="{ row }">
            <StatusBadge :type="row.level" size="small" />
          </template>
        </el-table-column>
        <el-table-column prop="message" label="消息" min-width="200" show-overflow-tooltip />
        <el-table-column prop="evidence" label="证据" width="100" show-overflow-tooltip>
          <template #default="{ row }">{{ row.evidence || '-' }}</template>
        </el-table-column>
        <el-table-column prop="acknowledged" label="状态" width="80">
          <template #default="{ row }">
            <el-tag :type="row.acknowledged ? 'info' : 'warning'" size="small">
              {{ row.acknowledged ? '已确认' : '未确认' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="操作" width="100" fixed="right">
          <template #default="{ row }">
            <el-button v-if="!row.acknowledged" text type="primary" size="small" @click="handleAck(row)">确认</el-button>
          </template>
        </el-table-column>
      </el-table>

      <div class="pagination-wrap">
        <el-pagination
          v-model:current-page="currentPage"
          v-model:page-size="pageSize"
          :total="alertStore.total"
          :page-sizes="[10, 20, 50, 100]"
          layout="total, sizes, prev, pager, next, jumper"
          @size-change="loadAlerts"
          @current-change="loadAlerts"
        />
      </div>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { useAlertStore } from '@/stores/alert'
import PageHeader from '@/components/common/PageHeader.vue'
import SearchBar from '@/components/common/SearchBar.vue'
import StatusBadge from '@/components/common/StatusBadge.vue'
import type { Alert } from '@/api/alert'
import dayjs from 'dayjs'

const alertStore = useAlertStore()
const searchKeyword = ref('')
const filterLevel = ref('')
const filterAck = ref('')
const dateRange = ref<[Date, Date] | null>(null)
const currentPage = ref(1)
const pageSize = ref(20)
const selectedAlerts = ref<Alert[]>([])

const alertStats = computed(() => {
  const s = alertStore.stats
  return [
    { label: '总报警', value: s?.total || 0, color: '#409EFF' },
    { label: '信息', value: s?.info || 0, color: '#909399' },
    { label: '警告', value: s?.warning || 0, color: '#E6A23C' },
    { label: '严重', value: s?.critical || 0, color: '#F56C6C' },
  ]
})

function formatTime(time: string) {
  return dayjs(time).format('YYYY-MM-DD HH:mm:ss')
}

function handleSelectionChange(rows: Alert[]) {
  selectedAlerts.value = rows
}

function handleSearch() {
  currentPage.value = 1
  loadAlerts()
}

function handleReset() {
  filterLevel.value = ''
  filterAck.value = ''
  searchKeyword.value = ''
  dateRange.value = null
  currentPage.value = 1
  loadAlerts()
}

async function loadAlerts() {
  const params: Record<string, any> = { page: currentPage.value, pageSize: pageSize.value }
  if (searchKeyword.value) params.keyword = searchKeyword.value
  if (filterLevel.value) params.level = filterLevel.value
  if (filterAck.value) params.acknowledged = filterAck.value
  if (dateRange.value) {
    params.startDate = dayjs(dateRange.value[0]).format('YYYY-MM-DD')
    params.endDate = dayjs(dateRange.value[1]).format('YYYY-MM-DD')
  }
  alertStore.setPagination(currentPage.value, pageSize.value)
  await alertStore.fetchAlerts(params)
}

async function handleAck(alert: Alert) {
  try {
    await alertStore.acknowledgeAlert(alert.id)
    ElMessage.success('已确认')
  } catch { /* handled */ }
}

async function handleBatchAck() {
  const ids = selectedAlerts.value.filter((a) => !a.acknowledged).map((a) => a.id)
  if (ids.length === 0) {
    ElMessage.warning('所选报警均已确认')
    return
  }
  try {
    await alertStore.batchAcknowledge(ids)
    ElMessage.success(`已确认 ${ids.length} 条报警`)
  } catch { /* handled */ }
}

onMounted(() => {
  alertStore.fetchAlertStats()
  loadAlerts()
})
</script>

<style scoped lang="scss">
.stats-row {
  margin-bottom: 16px;
}

.stat-card {
  background: #132f4c;
  border-radius: 8px;
  padding: 16px;
  text-align: center;
  border-top: 3px solid;
}

.stat-value {
  font-size: 24px;
  font-weight: 700;
}

.stat-label {
  font-size: 12px;
  color: #78909c;
  margin-top: 4px;
}

.table-card {
  background: #132f4c;
  border: 1px solid #1e3a5f;
}

.batch-bar {
  margin-bottom: 12px;
}

.pagination-wrap {
  display: flex;
  justify-content: flex-end;
  margin-top: 16px;
}
</style>
