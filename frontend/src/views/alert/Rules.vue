<template>
  <div class="alert-rules-page">
    <PageHeader title="报警规则" description="配置报警触发条件和通知方式">
      <template #actions>
        <el-button type="primary" :icon="Plus" @click="showForm(null)">添加规则</el-button>
      </template>
    </PageHeader>

    <el-card shadow="never" class="table-card">
      <el-table :data="alertStore.rules" v-loading="alertStore.loading" stripe>
        <el-table-column prop="name" label="规则名称" min-width="140" show-overflow-tooltip />
        <el-table-column prop="type" label="报警类型" width="120" />
        <el-table-column label="条件" min-width="180" show-overflow-tooltip>
          <template #default="{ row }">
            {{ formatCondition(row.condition) }}
          </template>
        </el-table-column>
        <el-table-column prop="threshold" label="阈值" width="80" />
        <el-table-column prop="debounce" label="防抖(ms)" width="90" />
        <el-table-column prop="enabled" label="状态" width="80">
          <template #default="{ row }">
            <el-tag :type="row.enabled ? 'success' : 'info'" size="small">
              {{ row.enabled ? '启用' : '禁用' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="操作" width="180" fixed="right">
          <template #default="{ row }">
            <el-button text type="primary" size="small" @click="showForm(row)">编辑</el-button>
            <el-button text :type="row.enabled ? 'warning' : 'success'" size="small" @click="handleToggle(row)">
              {{ row.enabled ? '禁用' : '启用' }}
            </el-button>
            <el-button text type="danger" size="small" @click="handleDelete(row)">删除</el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <!-- Rule Form Dialog -->
    <el-dialog v-model="formVisible" :title="editingRule ? '编辑规则' : '添加规则'" width="560px" :close-on-click-modal="false">
      <el-form ref="formRef" :model="ruleForm" :rules="formRules" label-width="90px">
        <el-form-item label="规则名称" prop="name">
          <el-input v-model="ruleForm.name" placeholder="请输入规则名称" />
        </el-form-item>
        <el-form-item label="报警类型" prop="type">
          <el-select v-model="ruleForm.type" placeholder="选择类型" style="width: 100%">
            <el-option label="检测报警" value="detection" />
            <el-option label="越界报警" value="intrusion" />
            <el-option label="聚集报警" value="gathering" />
            <el-option label="离岗报警" value="leave" />
            <el-option label="自定义" value="custom" />
          </el-select>
        </el-form-item>
        <el-row :gutter="16">
          <el-col :span="12">
            <el-form-item label="条件类型">
              <el-select v-model="ruleForm.conditionType" style="width: 100%">
                <el-option label="类别匹配" value="class" />
                <el-option label="置信度" value="confidence" />
                <el-option label="数量" value="count" />
                <el-option label="表达式" value="expression" />
              </el-select>
            </el-form-item>
          </el-col>
          <el-col :span="12">
            <el-form-item label="条件值">
              <el-input v-model="ruleForm.conditionValue" placeholder="如 person, >0.8" />
            </el-form-item>
          </el-col>
        </el-row>
        <el-row :gutter="16">
          <el-col :span="12">
            <el-form-item label="阈值" prop="threshold">
              <el-input-number v-model="ruleForm.threshold" :min="0" style="width: 100%" />
            </el-form-item>
          </el-col>
          <el-col :span="12">
            <el-form-item label="防抖(ms)">
              <el-input-number v-model="ruleForm.debounce" :min="0" :step="100" style="width: 100%" />
            </el-form-item>
          </el-col>
        </el-row>
        <el-form-item label="通知方式">
          <el-checkbox-group v-model="notifyMethods">
            <el-checkbox label="webhook">Webhook</el-checkbox>
            <el-checkbox label="email">邮件</el-checkbox>
            <el-checkbox label="sms">短信</el-checkbox>
          </el-checkbox-group>
        </el-form-item>
        <el-form-item v-if="notifyMethods.includes('webhook')" label="Webhook">
          <el-input v-model="webhookUrl" placeholder="http://webhook.url/callback" />
        </el-form-item>
        <el-form-item label="启用">
          <el-switch v-model="ruleForm.enabled" />
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
import { ref, reactive, onMounted } from 'vue'
import { Plus } from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import type { FormInstance, FormRules } from 'element-plus'
import { useAlertStore } from '@/stores/alert'
import PageHeader from '@/components/common/PageHeader.vue'
import type { AlertRule } from '@/api/alert'

const alertStore = useAlertStore()
const formVisible = ref(false)
const editingRule = ref<AlertRule | null>(null)
const formRef = ref<FormInstance>()
const saving = ref(false)
const notifyMethods = ref<string[]>([])
const webhookUrl = ref('')

const ruleForm = reactive({
  name: '',
  type: 'detection',
  conditionType: 'class',
  conditionValue: '',
  threshold: 1,
  debounce: 1000,
  enabled: true,
})

const formRules: FormRules = {
  name: [{ required: true, message: '请输入规则名称', trigger: 'blur' }],
  type: [{ required: true, message: '请选择报警类型', trigger: 'change' }],
  threshold: [{ required: true, message: '请输入阈值', trigger: 'blur' }],
}

function formatCondition(condition: Record<string, any>) {
  if (!condition) return '-'
  return Object.entries(condition).map(([k, v]) => `${k}:${v}`).join(', ')
}

function showForm(rule: AlertRule | null) {
  editingRule.value = rule
  if (rule) {
    Object.assign(ruleForm, {
      name: rule.name,
      type: rule.type,
      conditionType: rule.condition?.type || 'class',
      conditionValue: rule.condition?.value || '',
      threshold: rule.threshold,
      debounce: rule.debounce,
      enabled: rule.enabled,
    })
    notifyMethods.value = rule.notificationConfig?.methods || []
    webhookUrl.value = rule.notificationConfig?.webhookUrl || ''
  } else {
    Object.assign(ruleForm, {
      name: '', type: 'detection', conditionType: 'class', conditionValue: '',
      threshold: 1, debounce: 1000, enabled: true,
    })
    notifyMethods.value = []
    webhookUrl.value = ''
  }
  formVisible.value = true
}

async function handleSave() {
  const formEl = formRef.value
  if (!formEl) return
  await formEl.validate()
  saving.value = true
  const data = {
    name: ruleForm.name,
    type: ruleForm.type,
    condition: { type: ruleForm.conditionType, value: ruleForm.conditionValue },
    threshold: ruleForm.threshold,
    debounce: ruleForm.debounce,
    enabled: ruleForm.enabled,
    notificationConfig: {
      methods: notifyMethods.value,
      webhookUrl: webhookUrl.value,
    },
  }
  try {
    if (editingRule.value) {
      await alertStore.updateAlertRule(editingRule.value.id, data)
      ElMessage.success('规则更新成功')
    } else {
      await alertStore.createAlertRule(data)
      ElMessage.success('规则添加成功')
    }
    formVisible.value = false
    alertStore.fetchAlertRules()
  } finally {
    saving.value = false
  }
}

async function handleToggle(rule: AlertRule) {
  try {
    await alertStore.updateAlertRule(rule.id, { enabled: !rule.enabled })
    ElMessage.success(rule.enabled ? '规则已禁用' : '规则已启用')
  } catch { /* handled */ }
}

async function handleDelete(rule: AlertRule) {
  try {
    await ElMessageBox.confirm(`确定要删除规则 ${rule.name} 吗？`, '删除确认', { type: 'danger' })
    await alertStore.deleteAlertRule(rule.id)
    ElMessage.success('规则已删除')
  } catch { /* cancelled */ }
}

onMounted(() => {
  alertStore.fetchAlertRules()
})
</script>

<style scoped lang="scss">
.table-card {
  background: #132f4c;
  border: 1px solid #1e3a5f;
}
</style>
