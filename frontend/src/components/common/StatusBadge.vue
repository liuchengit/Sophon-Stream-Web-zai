<template>
  <span class="status-badge" :class="[`status-${type}`, sizeClass]">
    <span class="badge-dot" />
    <span class="badge-text">{{ label || statusLabel }}</span>
  </span>
</template>

<script setup lang="ts">
import { computed } from 'vue'

const props = defineProps<{
  type: 'online' | 'offline' | 'error' | 'running' | 'stopped' | 'paused' | 'active' | 'inactive' | 'info' | 'warning' | 'critical' | 'success'
  label?: string
  size?: 'small' | 'default'
}>()

const sizeClass = computed(() => (props.size === 'small' ? 'badge-small' : ''))

const statusLabel = computed(() => {
  const map: Record<string, string> = {
    online: '在线',
    offline: '离线',
    error: '异常',
    running: '运行中',
    stopped: '已停止',
    paused: '已暂停',
    active: '已激活',
    inactive: '未激活',
    info: '信息',
    warning: '警告',
    critical: '严重',
    success: '成功',
  }
  return map[props.type] || props.type
})
</script>

<style scoped lang="scss">
.status-badge {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 2px 10px;
  border-radius: 12px;
  font-size: 12px;
  white-space: nowrap;
}

.badge-small {
  padding: 1px 8px;
  font-size: 11px;
}

.badge-dot {
  width: 6px;
  height: 6px;
  border-radius: 50%;
  display: inline-block;
}

.status-online, .status-running, .status-active, .status-success {
  background: rgba(103, 194, 58, 0.15);
  color: #67C23A;

  .badge-dot {
    background: #67C23A;
    box-shadow: 0 0 4px #67C23A;
  }
}

.status-offline, .status-stopped, .status-inactive {
  background: rgba(144, 147, 153, 0.15);
  color: #909399;

  .badge-dot {
    background: #909399;
  }
}

.status-error, .status-critical {
  background: rgba(245, 108, 108, 0.15);
  color: #F56C6C;

  .badge-dot {
    background: #F56C6C;
    box-shadow: 0 0 4px #F56C6C;
  }
}

.status-paused, .status-warning {
  background: rgba(230, 162, 60, 0.15);
  color: #E6A23C;

  .badge-dot {
    background: #E6A23C;
    box-shadow: 0 0 4px #E6A23C;
  }
}

.status-info {
  background: rgba(64, 158, 255, 0.15);
  color: #409EFF;

  .badge-dot {
    background: #409EFF;
  }
}
</style>
