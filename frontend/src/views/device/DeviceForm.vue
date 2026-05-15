<template>
  <el-dialog
    v-model="visible"
    :title="isEdit ? '编辑设备' : '添加设备'"
    width="600px"
    :close-on-click-modal="false"
    @close="handleClose"
  >
    <el-form ref="formRef" :model="form" :rules="rules" label-width="80px">
      <el-form-item label="名称" prop="name">
        <el-input v-model="form.name" placeholder="请输入设备名称" />
      </el-form-item>
      <el-row :gutter="16">
        <el-col :span="12">
          <el-form-item label="类型" prop="type">
            <el-select v-model="form.type" placeholder="请选择类型" style="width: 100%">
              <el-option label="IPC" value="IPC" />
              <el-option label="NVR" value="NVR" />
              <el-option label="RTSP" value="RTSP" />
              <el-option label="FILE" value="FILE" />
              <el-option label="GB28181" value="GB28181" />
            </el-select>
          </el-form-item>
        </el-col>
        <el-col :span="12">
          <el-form-item label="协议" prop="protocol">
            <el-select v-model="form.protocol" placeholder="请选择协议" style="width: 100%">
              <el-option label="RTSP" value="RTSP" />
              <el-option label="RTMP" value="RTMP" />
              <el-option label="GB28181" value="GB28181" />
              <el-option label="FILE" value="FILE" />
            </el-select>
          </el-form-item>
        </el-col>
      </el-row>
      <el-form-item label="流地址" prop="streamUrl">
        <el-input v-model="form.streamUrl" placeholder="rtsp:// 或文件路径" />
      </el-form-item>
      <el-form-item v-if="form.type === 'GB28181'" label="设备ID" prop="deviceId">
        <el-input v-model="form.deviceId" placeholder="GB28181设备ID" />
      </el-form-item>
      <el-row :gutter="16">
        <el-col :span="12">
          <el-form-item label="分辨率" prop="resolution">
            <el-input v-model="form.resolution" placeholder="如 1920x1080" />
          </el-form-item>
        </el-col>
        <el-col :span="12">
          <el-form-item label="FPS" prop="fps">
            <el-input-number v-model="form.fps" :min="1" :max="60" style="width: 100%" />
          </el-form-item>
        </el-col>
      </el-row>
      <el-row :gutter="16">
        <el-col :span="12">
          <el-form-item label="编码" prop="codec">
            <el-select v-model="form.codec" placeholder="编码格式" style="width: 100%">
              <el-option label="H.264" value="H264" />
              <el-option label="H.265" value="H265" />
              <el-option label="MJPEG" value="MJPEG" />
            </el-select>
          </el-form-item>
        </el-col>
        <el-col :span="12">
          <el-form-item label="位置" prop="location">
            <el-input v-model="form.location" placeholder="设备安装位置" />
          </el-form-item>
        </el-col>
      </el-row>
      <el-form-item label="描述" prop="description">
        <el-input v-model="form.description" type="textarea" :rows="3" placeholder="设备描述" />
      </el-form-item>
    </el-form>

    <template #footer>
      <el-button @click="handleClose">取消</el-button>
      <el-button type="primary" :loading="saving" @click="handleSave">保存</el-button>
    </template>
  </el-dialog>
</template>

<script setup lang="ts">
import { ref, reactive, computed, watch } from 'vue'
import { ElMessage } from 'element-plus'
import type { FormInstance, FormRules } from 'element-plus'
import { useDeviceStore } from '@/stores/device'
import type { Device, DeviceFormData } from '@/api/device'

const props = defineProps<{
  modelValue: boolean
  device: Device | null
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  saved: []
}>()

const deviceStore = useDeviceStore()
const formRef = ref<FormInstance>()
const saving = ref(false)

const visible = computed({
  get: () => props.modelValue,
  set: (val) => emit('update:modelValue', val),
})

const isEdit = computed(() => !!props.device)

const form = reactive<DeviceFormData>({
  name: '',
  type: 'IPC',
  streamUrl: '',
  protocol: 'RTSP',
  resolution: '',
  fps: 25,
  codec: 'H264',
  location: '',
  description: '',
  deviceId: '',
})

const rules: FormRules = {
  name: [{ required: true, message: '请输入设备名称', trigger: 'blur' }],
  type: [{ required: true, message: '请选择设备类型', trigger: 'change' }],
  streamUrl: [{ required: true, message: '请输入流地址', trigger: 'blur' }],
  protocol: [{ required: true, message: '请选择协议', trigger: 'change' }],
  fps: [{ required: true, message: '请输入FPS', trigger: 'blur' }],
}

watch(
  () => props.device,
  (device) => {
    if (device) {
      Object.assign(form, {
        name: device.name,
        type: device.type,
        streamUrl: device.streamUrl,
        protocol: device.protocol,
        resolution: device.resolution,
        fps: device.fps,
        codec: device.codec,
        location: device.location,
        description: device.description,
        deviceId: device.deviceId || '',
      })
    } else {
      Object.assign(form, {
        name: '',
        type: 'IPC',
        streamUrl: '',
        protocol: 'RTSP',
        resolution: '',
        fps: 25,
        codec: 'H264',
        location: '',
        description: '',
        deviceId: '',
      })
    }
  },
  { immediate: true }
)

function handleClose() {
  visible.value = false
  formRef.value?.resetFields()
}

async function handleSave() {
  const formEl = formRef.value
  if (!formEl) return
  await formEl.validate()
  saving.value = true
  try {
    if (isEdit.value && props.device) {
      await deviceStore.updateDevice(props.device.id, { ...form })
      ElMessage.success('设备更新成功')
    } else {
      await deviceStore.createDevice({ ...form })
      ElMessage.success('设备添加成功')
    }
    emit('saved')
    handleClose()
  } finally {
    saving.value = false
  }
}
</script>
