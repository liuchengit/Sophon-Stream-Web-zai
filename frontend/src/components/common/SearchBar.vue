<template>
  <div class="search-bar">
    <el-input
      v-model="keyword"
      :placeholder="placeholder"
      clearable
      :prefix-icon="Search"
      class="search-input"
      @keyup.enter="handleSearch"
      @clear="handleSearch"
    />
    <slot name="filters" />
    <el-button type="primary" :icon="Search" @click="handleSearch">搜索</el-button>
    <el-button :icon="Refresh" @click="handleReset">重置</el-button>
  </div>
</template>

<script setup lang="ts">
import { ref, watch } from 'vue'
import { Search, Refresh } from '@element-plus/icons-vue'

const props = defineProps<{
  placeholder?: string
  modelValue?: string
}>()

const emit = defineEmits<{
  search: [keyword: string]
  reset: []
  'update:modelValue': [value: string]
}>()

const keyword = ref(props.modelValue || '')

watch(
  () => props.modelValue,
  (val) => {
    keyword.value = val || ''
  }
)

function handleSearch() {
  emit('update:modelValue', keyword.value)
  emit('search', keyword.value)
}

function handleReset() {
  keyword.value = ''
  emit('update:modelValue', '')
  emit('reset')
}
</script>

<style scoped lang="scss">
.search-bar {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-wrap: wrap;
  margin-bottom: 16px;
}

.search-input {
  width: 260px;
}
</style>
