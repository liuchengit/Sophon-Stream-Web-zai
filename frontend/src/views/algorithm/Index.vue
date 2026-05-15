<template>
  <div class="algorithm-page">
    <PageHeader title="算法管理" description="管理AI算法模型，包括检测、跟踪、分类等">
      <template #actions>
        <el-button type="primary" :icon="Plus" @click="showForm(null)">添加算法</el-button>
      </template>
    </PageHeader>

    <SearchBar v-model="searchKeyword" placeholder="搜索算法名称" @search="handleSearch" @reset="handleReset">
      <template #filters>
        <el-select v-model="filterType" placeholder="算法类型" clearable style="width: 140px" @change="handleSearch">
          <el-option label="检测" value="detection" />
          <el-option label="跟踪" value="tracking" />
          <el-option label="分类" value="classification" />
          <el-option label="分割" value="segmentation" />
          <el-option label="姿态" value="pose" />
          <el-option label="OCR" value="ocr" />
          <el-option label="人脸" value="face" />
        </el-select>
      </template>
    </SearchBar>

    <el-card shadow="never" class="table-card">
      <el-table :data="algorithmStore.algorithms" v-loading="algorithmStore.loading" stripe>
        <el-table-column prop="name" label="名称" min-width="140" show-overflow-tooltip />
        <el-table-column prop="type" label="类型" width="100">
          <template #default="{ row }">
            <el-tag size="small" :type="algoTypeTag[row.type]">{{ algoTypeLabel[row.type] || row.type }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="modelPath" label="模型路径" min-width="180" show-overflow-tooltip />
        <el-table-column prop="version" label="版本" width="80" />
        <el-table-column prop="inputSize" label="输入尺寸" width="100" />
        <el-table-column prop="status" label="状态" width="100">
          <template #default="{ row }">
            <StatusBadge :type="row.status === 'active' ? 'active' : 'inactive'" size="small" />
          </template>
        </el-table-column>
        <el-table-column label="操作" width="180" fixed="right">
          <template #default="{ row }">
            <el-button text type="primary" size="small" @click="showForm(row)">编辑</el-button>
            <el-button text type="success" size="small" @click="handleValidate(row)">验证</el-button>
            <el-button text type="danger" size="small" @click="handleDelete(row)">删除</el-button>
          </template>
        </el-table-column>
      </el-table>

      <div class="pagination-wrap">
        <el-pagination
          v-model:current-page="currentPage"
          v-model:page-size="pageSize"
          :total="algorithmStore.total"
          :page-sizes="[10, 20, 50]"
          layout="total, sizes, prev, pager, next"
          @size-change="loadAlgorithms"
          @current-change="loadAlgorithms"
        />
      </div>
    </el-card>

    <!-- Algorithm Form Dialog -->
    <el-dialog v-model="formVisible" :title="editingAlgorithm ? '编辑算法' : '添加算法'" width="600px" :close-on-click-modal="false">
      <el-form ref="formRef" :model="algoForm" :rules="formRules" label-width="80px">
        <el-form-item label="名称" prop="name">
          <el-input v-model="algoForm.name" placeholder="请输入算法名称" />
        </el-form-item>
        <el-row :gutter="16">
          <el-col :span="12">
            <el-form-item label="类型" prop="type">
              <el-select v-model="algoForm.type" placeholder="算法类型" style="width: 100%">
                <el-option label="检测" value="detection" />
                <el-option label="跟踪" value="tracking" />
                <el-option label="分类" value="classification" />
                <el-option label="分割" value="segmentation" />
                <el-option label="姿态" value="pose" />
                <el-option label="OCR" value="ocr" />
                <el-option label="人脸" value="face" />
              </el-select>
            </el-form-item>
          </el-col>
          <el-col :span="12">
            <el-form-item label="版本" prop="version">
              <el-input v-model="algoForm.version" placeholder="如 v1.0" />
            </el-form-item>
          </el-col>
        </el-row>
        <el-form-item label="模型路径" prop="modelPath">
          <el-input v-model="algoForm.modelPath" placeholder="/path/to/model.bmodel" />
        </el-form-item>
        <el-form-item label="输入尺寸" prop="inputSize">
          <el-input v-model="algoForm.inputSize" placeholder="如 640x640" />
        </el-form-item>
        <el-form-item label="类别">
          <el-input v-model="classesText" placeholder="用逗号分隔，如 person,car,truck" />
        </el-form-item>
        <el-form-item label="描述">
          <el-input v-model="algoForm.description" type="textarea" :rows="3" placeholder="算法描述" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="formVisible = false">取消</el-button>
        <el-button type="primary" :loading="saving" @click="handleSave">保存</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed, watch, onMounted } from 'vue'
import { Plus } from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import type { FormInstance, FormRules } from 'element-plus'
import { useAlgorithmStore } from '@/stores/algorithm'
import PageHeader from '@/components/common/PageHeader.vue'
import SearchBar from '@/components/common/SearchBar.vue'
import StatusBadge from '@/components/common/StatusBadge.vue'
import type { Algorithm } from '@/api/algorithm'

const algorithmStore = useAlgorithmStore()
const searchKeyword = ref('')
const filterType = ref('')
const currentPage = ref(1)
const pageSize = ref(20)
const formVisible = ref(false)
const editingAlgorithm = ref<Algorithm | null>(null)
const formRef = ref<FormInstance>()
const saving = ref(false)
const classesText = ref('')

const algoTypeTag: Record<string, string> = {
  detection: '', tracking: 'success', classification: 'warning',
  segmentation: 'danger', pose: 'info', ocr: 'success', face: '',
}

const algoTypeLabel: Record<string, string> = {
  detection: '检测', tracking: '跟踪', classification: '分类',
  segmentation: '分割', pose: '姿态', ocr: 'OCR', face: '人脸',
}

const algoForm = reactive({
  name: '',
  type: 'detection',
  modelPath: '',
  version: '',
  inputSize: '',
  classes: [] as string[],
  description: '',
  config: {},
})

const formRules: FormRules = {
  name: [{ required: true, message: '请输入算法名称', trigger: 'blur' }],
  type: [{ required: true, message: '请选择算法类型', trigger: 'change' }],
  modelPath: [{ required: true, message: '请输入模型路径', trigger: 'blur' }],
}

function showForm(algo: Algorithm | null) {
  editingAlgorithm.value = algo
  if (algo) {
    Object.assign(algoForm, {
      name: algo.name,
      type: algo.type,
      modelPath: algo.modelPath,
      version: algo.version,
      inputSize: algo.inputSize,
      classes: algo.classes || [],
      description: algo.description,
      config: algo.config || {},
    })
    classesText.value = (algo.classes || []).join(',')
  } else {
    Object.assign(algoForm, {
      name: '', type: 'detection', modelPath: '', version: '',
      inputSize: '', classes: [], description: '', config: {},
    })
    classesText.value = ''
  }
  formVisible.value = true
}

async function handleSave() {
  const formEl = formRef.value
  if (!formEl) return
  await formEl.validate()
  saving.value = true
  algoForm.classes = classesText.value.split(',').map((s) => s.trim()).filter(Boolean)
  try {
    if (editingAlgorithm.value) {
      await algorithmStore.updateAlgorithm(editingAlgorithm.value.id, { ...algoForm })
      ElMessage.success('算法更新成功')
    } else {
      await algorithmStore.createAlgorithm({ ...algoForm })
      ElMessage.success('算法添加成功')
    }
    formVisible.value = false
    loadAlgorithms()
  } finally {
    saving.value = false
  }
}

async function handleValidate(algo: Algorithm) {
  try {
    const result = await algorithmStore.validateModel(algo.id)
    if (result.valid) {
      ElMessage.success('模型验证通过')
    } else {
      ElMessage.error(result.message || '模型验证失败')
    }
  } catch { /* handled */ }
}

async function handleDelete(algo: Algorithm) {
  try {
    await ElMessageBox.confirm(`确定要删除算法 ${algo.name} 吗？`, '删除确认', { type: 'danger' })
    await algorithmStore.deleteAlgorithm(algo.id)
    ElMessage.success('算法已删除')
  } catch { /* cancelled */ }
}

function handleSearch() {
  currentPage.value = 1
  loadAlgorithms()
}

function handleReset() {
  filterType.value = ''
  searchKeyword.value = ''
  currentPage.value = 1
  loadAlgorithms()
}

async function loadAlgorithms() {
  const params: Record<string, any> = { page: currentPage.value, pageSize: pageSize.value }
  if (searchKeyword.value) params.keyword = searchKeyword.value
  if (filterType.value) params.type = filterType.value
  algorithmStore.setPagination(currentPage.value, pageSize.value)
  await algorithmStore.fetchAlgorithms(params)
}

onMounted(() => {
  loadAlgorithms()
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
