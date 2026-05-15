<template>
  <div class="json-editor">
    <div class="editor-header">
      <span class="editor-title">{{ title }}</span>
      <div class="editor-actions">
        <el-button size="small" text @click="formatJson">格式化</el-button>
        <el-button size="small" text @click="compressJson">压缩</el-button>
      </div>
    </div>
    <el-input
      v-model="jsonText"
      type="textarea"
      :rows="rows"
      :placeholder="placeholder"
      class="json-textarea"
      @blur="handleBlur"
    />
    <div v-if="error" class="json-error">
      <el-icon><WarningFilled /></el-icon>
      <span>{{ error }}</span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, watch } from 'vue'

const props = withDefaults(defineProps<{
  modelValue: Record<string, any> | string
  title?: string
  rows?: number
  placeholder?: string
}>(), {
  title: 'JSON配置',
  rows: 12,
  placeholder: '请输入JSON配置',
})

const emit = defineEmits<{
  'update:modelValue': [value: Record<string, any>]
  error: [msg: string]
}>()

const jsonText = ref('')
const error = ref('')

watch(
  () => props.modelValue,
  (val) => {
    if (typeof val === 'string') {
      jsonText.value = val
    } else {
      jsonText.value = JSON.stringify(val, null, 2)
    }
    error.value = ''
  },
  { immediate: true }
)

function handleBlur() {
  try {
    const parsed = JSON.parse(jsonText.value)
    error.value = ''
    emit('update:modelValue', parsed)
    emit('error', '')
  } catch (e: any) {
    error.value = `JSON格式错误: ${e.message}`
    emit('error', error.value)
  }
}

function formatJson() {
  try {
    const parsed = JSON.parse(jsonText.value)
    jsonText.value = JSON.stringify(parsed, null, 2)
    error.value = ''
  } catch (e: any) {
    error.value = `JSON格式错误: ${e.message}`
  }
}

function compressJson() {
  try {
    const parsed = JSON.parse(jsonText.value)
    jsonText.value = JSON.stringify(parsed)
    error.value = ''
  } catch (e: any) {
    error.value = `JSON格式错误: ${e.message}`
  }
}
</script>

<style scoped lang="scss">
.json-editor {
  border: 1px solid #1e3a5f;
  border-radius: 4px;
  overflow: hidden;
}

.editor-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 8px 12px;
  background: #0d2137;
  border-bottom: 1px solid #1e3a5f;
}

.editor-title {
  font-size: 13px;
  color: #b0bec5;
}

.editor-actions {
  display: flex;
  gap: 4px;
}

.json-textarea {
  :deep(.el-textarea__inner) {
    font-family: 'Consolas', 'Monaco', 'Courier New', monospace;
    font-size: 13px;
    line-height: 1.6;
    background: #0a1929;
    border: none;
    border-radius: 0;
    color: #e3e8ef;
  }
}

.json-error {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 6px 12px;
  background: rgba(245, 108, 108, 0.1);
  color: #F56C6C;
  font-size: 12px;
}
</style>
