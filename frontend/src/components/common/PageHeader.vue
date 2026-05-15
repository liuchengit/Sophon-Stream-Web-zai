<template>
  <div class="page-header">
    <div class="header-content">
      <div class="header-left">
        <h2 class="page-title">{{ title }}</h2>
        <p v-if="description" class="page-description">{{ description }}</p>
      </div>
      <div class="header-actions">
        <slot name="actions" />
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
defineProps<{
  title: string
  description?: string
}>()
</script>

<style scoped lang="scss">
.page-header {
  margin-bottom: 20px;
}

.header-content {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
}

.page-title {
  font-size: 20px;
  font-weight: 600;
  color: #e3e8ef;
  margin: 0;
}

.page-description {
  font-size: 13px;
  color: #78909c;
  margin-top: 4px;
}

.header-actions {
  display: flex;
  align-items: center;
  gap: 8px;
}
</style>
