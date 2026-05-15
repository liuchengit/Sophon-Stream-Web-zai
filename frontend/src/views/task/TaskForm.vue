<template>
  <div class="task-form-page">
    <PageHeader :title="isEdit ? '编辑任务' : '创建任务'">
      <template #actions>
        <el-button @click="$router.back()">取消</el-button>
      </template>
    </PageHeader>

    <el-card shadow="never" class="form-card">
      <el-steps :active="currentStep" finish-status="success" align-center class="form-steps">
        <el-step title="基本信息" />
        <el-step title="选择设备" />
        <el-step title="选择算法" />
        <el-step title="配置参数" />
        <el-step title="ROI设置" />
      </el-steps>

      <div class="step-content">
        <!-- Step 1: Basic Info -->
        <div v-show="currentStep === 0">
          <el-form ref="step1Ref" :model="form" :rules="step1Rules" label-width="100px" style="max-width: 600px; margin: 0 auto; padding-top: 24px;">
            <el-form-item label="任务名称" prop="name">
              <el-input v-model="form.name" placeholder="请输入任务名称" />
            </el-form-item>
            <el-form-item label="任务类型" prop="type">
              <el-select v-model="form.type" placeholder="请选择任务类型" style="width: 100%">
                <el-option label="实时分析" value="realtime" />
                <el-option label="离线分析" value="offline" />
              </el-select>
            </el-form-item>
            <el-form-item label="描述" prop="description">
              <el-input v-model="form.description" type="textarea" :rows="4" placeholder="请输入任务描述" />
            </el-form-item>
          </el-form>
        </div>

        <!-- Step 2: Select Device -->
        <div v-show="currentStep === 1">
          <el-form ref="step2Ref" :model="form" :rules="step2Rules" label-width="100px" style="max-width: 600px; margin: 0 auto; padding-top: 24px;">
            <el-form-item label="选择设备" prop="deviceId">
              <el-select v-model="form.deviceId" placeholder="请选择设备" style="width: 100%" filterable>
                <el-option
                  v-for="device in deviceStore.devices"
                  :key="device.id"
                  :label="`${device.name} (${device.type})`"
                  :value="device.id"
                >
                  <div style="display: flex; justify-content: space-between; align-items: center; width: 100%">
                    <span>{{ device.name }} ({{ device.type }})</span>
                    <StatusBadge :type="device.status" size="small" />
                  </div>
                </el-option>
              </el-select>
            </el-form-item>
            <el-form-item v-if="selectedDevice" label="设备信息">
              <div class="device-preview">
                <p>类型: {{ selectedDevice.type }} | 协议: {{ selectedDevice.protocol }}</p>
                <p>流地址: {{ selectedDevice.streamUrl }}</p>
                <p>分辨率: {{ selectedDevice.resolution || '-' }} | FPS: {{ selectedDevice.fps }}</p>
              </div>
            </el-form-item>
          </el-form>
        </div>

        <!-- Step 3: Select Algorithms -->
        <div v-show="currentStep === 2">
          <div class="algorithm-select" style="max-width: 700px; margin: 0 auto; padding-top: 24px;">
            <el-checkbox-group v-model="form.algorithmIds">
              <el-row :gutter="16">
                <el-col :xs="24" :sm="12" v-for="algo in algorithmStore.algorithms" :key="algo.id">
                  <div class="algorithm-card" :class="{ selected: form.algorithmIds.includes(algo.id) }" @click="toggleAlgorithm(algo.id)">
                    <el-checkbox :label="algo.id" :value="algo.id" @click.stop>&nbsp;</el-checkbox>
                    <div class="algo-info">
                      <div class="algo-name">{{ algo.name }}</div>
                      <div class="algo-meta">{{ algo.type }} | {{ algo.inputSize }} | v{{ algo.version }}</div>
                    </div>
                    <StatusBadge :type="algo.status === 'active' ? 'active' : 'inactive'" size="small" />
                  </div>
                </el-col>
              </el-row>
            </el-checkbox-group>
            <el-empty v-if="algorithmStore.algorithms.length === 0" description="暂无可用算法，请先添加算法" />
          </div>
        </div>

        <!-- Step 4: Config -->
        <div v-show="currentStep === 3" style="max-width: 700px; margin: 0 auto; padding-top: 24px;">
          <el-form label-width="100px">
            <el-form-item label="Pipeline配置">
              <JsonEditor v-model="form.config" title="Graph配置" :rows="10" />
            </el-form-item>
            <el-form-item label="OSD叠加">
              <el-switch v-model="osdEnabled" active-text="开启" inactive-text="关闭" />
            </el-form-item>
            <el-form-item label="录像存储">
              <el-switch v-model="recordEnabled" active-text="开启" inactive-text="关闭" />
            </el-form-item>
            <el-form-item label="输出地址">
              <el-input v-model="outputUrl" placeholder="rtsp://输出流地址（可选）" />
            </el-form-item>
          </el-form>
        </div>

        <!-- Step 5: ROI -->
        <div v-show="currentStep === 4" style="padding-top: 24px;">
          <div class="roi-step">
            <el-alert type="info" :closable="false" show-icon>
              <template #title>ROI（感兴趣区域）配置可在任务创建后通过任务详情页面进行编辑</template>
            </el-alert>
            <JsonEditor v-model="form.roi" title="ROI配置" :rows="8" class="mt-16" />
          </div>
        </div>
      </div>

      <div class="step-actions">
        <el-button v-if="currentStep > 0" @click="currentStep--">上一步</el-button>
        <el-button v-if="currentStep < 4" type="primary" @click="handleNext">下一步</el-button>
        <el-button v-if="currentStep === 4" type="primary" :loading="saving" @click="handleSave">
          {{ isEdit ? '保存修改' : '创建任务' }}
        </el-button>
      </div>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed, onMounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import type { FormInstance, FormRules } from 'element-plus'
import { useDeviceStore } from '@/stores/device'
import { useTaskStore } from '@/stores/task'
import { useAlgorithmStore } from '@/stores/algorithm'
import PageHeader from '@/components/common/PageHeader.vue'
import StatusBadge from '@/components/common/StatusBadge.vue'
import JsonEditor from '@/components/common/JsonEditor.vue'

const route = useRoute()
const router = useRouter()
const deviceStore = useDeviceStore()
const taskStore = useTaskStore()
const algorithmStore = useAlgorithmStore()

const currentStep = ref(0)
const saving = ref(false)
const step1Ref = ref<FormInstance>()
const step2Ref = ref<FormInstance>()
const osdEnabled = ref(false)
const recordEnabled = ref(false)
const outputUrl = ref('')

const taskId = computed(() => route.params.id ? Number(route.params.id) : null)
const isEdit = computed(() => !!taskId.value && route.path.includes('/edit'))

const form = reactive({
  name: '',
  type: 'realtime',
  description: '',
  deviceId: null as number | null,
  algorithmIds: [] as number[],
  config: {} as Record<string, any>,
  roi: {} as Record<string, any>,
})

const step1Rules: FormRules = {
  name: [{ required: true, message: '请输入任务名称', trigger: 'blur' }],
  type: [{ required: true, message: '请选择任务类型', trigger: 'change' }],
}

const step2Rules: FormRules = {
  deviceId: [{ required: true, message: '请选择设备', trigger: 'change' }],
}

const selectedDevice = computed(() =>
  form.deviceId ? deviceStore.devices.find((d) => d.id === form.deviceId) : null
)

function toggleAlgorithm(id: number) {
  const idx = form.algorithmIds.indexOf(id)
  if (idx === -1) {
    form.algorithmIds.push(id)
  } else {
    form.algorithmIds.splice(idx, 1)
  }
}

async function handleNext() {
  if (currentStep.value === 0) {
    const valid = await step1Ref.value?.validate().catch(() => false)
    if (!valid) return
  } else if (currentStep.value === 1) {
    const valid = await step2Ref.value?.validate().catch(() => false)
    if (!valid) return
  }
  currentStep.value++
}

async function handleSave() {
  saving.value = true
  try {
    const data = {
      ...form,
      config: {
        ...form.config,
        osd: osdEnabled.value,
        record: recordEnabled.value,
        outputUrl: outputUrl.value,
      },
    }
    if (isEdit.value && taskId.value) {
      await taskStore.updateTask(taskId.value, data)
      ElMessage.success('任务更新成功')
    } else {
      const task = await taskStore.createTask(data as any)
      ElMessage.success('任务创建成功')
      router.push(`/tasks/${task.id}`)
    }
  } finally {
    saving.value = false
  }
}

onMounted(async () => {
  await Promise.all([
    deviceStore.fetchDevices({ page: 1, pageSize: 100 }),
    algorithmStore.fetchAlgorithms({ page: 1, pageSize: 100 }),
  ])
  if (isEdit.value && taskId.value) {
    await taskStore.fetchTask(taskId.value)
    const task = taskStore.currentTask
    if (task) {
      Object.assign(form, {
        name: task.name,
        type: task.type || 'realtime',
        description: task.description,
        deviceId: task.deviceId,
        algorithmIds: task.algorithmIds || [],
        config: task.config || {},
        roi: task.roi || {},
      })
      osdEnabled.value = task.config?.osd || false
      recordEnabled.value = task.config?.record || false
      outputUrl.value = task.config?.outputUrl || ''
    }
  }
})
</script>

<style scoped lang="scss">
.form-card {
  background: #132f4c;
  border: 1px solid #1e3a5f;
}

.form-steps {
  padding: 20px 40px;
  :deep(.el-step__title) {
    color: #78909c;
    font-size: 13px;
  }
  :deep(.el-step__head.is-finish .el-step__title) {
    color: #409EFF;
  }
  :deep(.el-step__head.is-process .el-step__title) {
    color: #e3e8ef;
  }
}

.step-content {
  padding: 0 20px;
  min-height: 300px;
}

.step-actions {
  display: flex;
  justify-content: center;
  gap: 12px;
  padding: 24px;
  border-top: 1px solid #1e3a5f;
}

.device-preview {
  background: rgba(10, 25, 41, 0.5);
  border: 1px solid #1e3a5f;
  border-radius: 4px;
  padding: 12px;
  font-size: 13px;
  color: #b0bec5;
  line-height: 1.8;
}

.algorithm-card {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 12px 16px;
  background: rgba(10, 25, 41, 0.5);
  border: 1px solid #1e3a5f;
  border-radius: 8px;
  margin-bottom: 12px;
  cursor: pointer;
  transition: all 0.2s;

  &:hover {
    border-color: #409EFF;
    background: rgba(64, 158, 255, 0.05);
  }

  &.selected {
    border-color: #409EFF;
    background: rgba(64, 158, 255, 0.1);
  }
}

.algo-info {
  flex: 1;
}

.algo-name {
  font-size: 14px;
  color: #e3e8ef;
  font-weight: 500;
}

.algo-meta {
  font-size: 12px;
  color: #78909c;
  margin-top: 2px;
}

.roi-step {
  max-width: 700px;
  margin: 0 auto;
}
</style>
