<template>
  <el-dialog
    v-model="dialogVisible"
    :title="title"
    :width="width"
    :close-on-click-modal="false"
    @close="handleClose"
  >
    <div class="confirm-content">
      <el-icon v-if="type === 'warning'" class="confirm-icon warning"><WarningFilled /></el-icon>
      <el-icon v-else-if="type === 'danger'" class="confirm-icon danger"><CircleCloseFilled /></el-icon>
      <el-icon v-else class="confirm-icon info"><InfoFilled /></el-icon>
      <div class="confirm-message">
        <p class="message-text">{{ message }}</p>
        <p v-if="detail" class="message-detail">{{ detail }}</p>
      </div>
    </div>
    <template #footer>
      <el-button @click="handleClose">取消</el-button>
      <el-button :type="confirmButtonType" :loading="loading" @click="handleConfirm">
        {{ confirmText }}
      </el-button>
    </template>
  </el-dialog>
</template>

<script setup lang="ts">
import { ref, watch } from 'vue'

const props = withDefaults(defineProps<{
  modelValue: boolean
  title?: string
  message: string
  detail?: string
  type?: 'warning' | 'danger' | 'info'
  confirmText?: string
  loading?: boolean
  width?: string
}>(), {
  title: '确认操作',
  type: 'warning',
  confirmText: '确认',
  loading: false,
  width: '420px',
})

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  confirm: []
  cancel: []
}>()

const dialogVisible = ref(props.modelValue)

watch(
  () => props.modelValue,
  (val) => {
    dialogVisible.value = val
  }
)

const confirmButtonType = computed(() => {
  if (props.type === 'danger') return 'danger'
  return 'primary'
})

function handleClose() {
  dialogVisible.value = false
  emit('update:modelValue', false)
  emit('cancel')
}

function handleConfirm() {
  emit('confirm')
}

import { computed } from 'vue'
</script>

<style scoped lang="scss">
.confirm-content {
  display: flex;
  align-items: flex-start;
  gap: 16px;
}

.confirm-icon {
  font-size: 24px;
  flex-shrink: 0;
  margin-top: 2px;

  &.warning {
    color: #E6A23C;
  }

  &.danger {
    color: #F56C6C;
  }

  &.info {
    color: #409EFF;
  }
}

.confirm-message {
  flex: 1;
}

.message-text {
  font-size: 14px;
  color: #e3e8ef;
  margin: 0;
}

.message-detail {
  font-size: 12px;
  color: #78909c;
  margin-top: 8px;
}
</style>
