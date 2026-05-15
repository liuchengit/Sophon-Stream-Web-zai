<template>
  <div class="audit-page">
    <PageHeader title="审计日志" description="查看系统操作审计记录" />

    <SearchBar v-model="searchKeyword" placeholder="搜索操作/用户" @search="handleSearch" @reset="handleReset">
      <template #filters>
        <el-select v-model="filterAction" placeholder="操作类型" clearable style="width: 140px" @change="handleSearch">
          <el-option label="创建" value="create" />
          <el-option label="更新" value="update" />
          <el-option label="删除" value="delete" />
          <el-option label="登录" value="login" />
          <el-option label="启动" value="start" />
          <el-option label="停止" value="stop" />
        </el-select>
        <el-date-picker
          v-model="dateRange"
          type="daterange"
          range-separator="至"
          start-placeholder="开始"
          end-placeholder="结束"
          style="width: 240px"
          @change="handleSearch"
        />
      </template>
    </SearchBar>

    <el-card shadow="never" class="table-card">
      <el-table :data="systemStore.auditLogs" v-loading="systemStore.loading" stripe>
        <el-table-column prop="createdAt" label="时间" width="160">
          <template #default="{ row }">{{ formatTime(row.createdAt) }}</template>
        </el-table-column>
        <el-table-column prop="username" label="用户" width="120" />
        <el-table-column prop="action" label="操作" width="100">
          <template #default="{ row }">
            <el-tag :type="actionTagMap[row.action] || 'info'" size="small">{{ row.action }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="resource" label="资源" width="120" show-overflow-tooltip />
        <el-table-column prop="detail" label="详情" min-width="200" show-overflow-tooltip />
        <el-table-column prop="ip" label="IP地址" width="130" />
      </el-table>

      <div class="pagination-wrap">
        <el-pagination
          v-model:current-page="currentPage"
          v-model:page-size="pageSize"
          :total="systemStore.auditTotal"
          :page-sizes="[20, 50, 100]"
          layout="total, sizes, prev, pager, next"
          @size-change="loadLogs"
          @current-change="loadLogs"
        />
      </div>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useSystemStore } from '@/stores/system'
import PageHeader from '@/components/common/PageHeader.vue'
import SearchBar from '@/components/common/SearchBar.vue'
import dayjs from 'dayjs'

const systemStore = useSystemStore()
const searchKeyword = ref('')
const filterAction = ref('')
const dateRange = ref<[Date, Date] | null>(null)
const currentPage = ref(1)
const pageSize = ref(20)

const actionTagMap: Record<string, string> = {
  create: 'success', update: 'warning', delete: 'danger',
  login: '', start: 'success', stop: 'info',
}

function formatTime(time: string) {
  return dayjs(time).format('YYYY-MM-DD HH:mm:ss')
}

function handleSearch() {
  currentPage.value = 1
  loadLogs()
}

function handleReset() {
  searchKeyword.value = ''
  filterAction.value = ''
  dateRange.value = null
  currentPage.value = 1
  loadLogs()
}

async function loadLogs() {
  const params: Record<string, any> = { page: currentPage.value, pageSize: pageSize.value }
  if (searchKeyword.value) params.keyword = searchKeyword.value
  if (filterAction.value) params.action = filterAction.value
  if (dateRange.value) {
    params.startDate = dayjs(dateRange.value[0]).format('YYYY-MM-DD')
    params.endDate = dayjs(dateRange.value[1]).format('YYYY-MM-DD')
  }
  await systemStore.fetchAuditLogs(params)
}

onMounted(() => {
  loadLogs()
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
</style>
