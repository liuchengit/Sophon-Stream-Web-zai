<template>
  <div class="dag-node" :class="{ selected, [nodeType]: true }" :style="nodeStyle" @mousedown.stop="$emit('select', node.id)">
    <div class="node-header">
      <el-icon :size="16"><component :is="nodeIcon" /></el-icon>
      <span class="node-label">{{ node.label }}</span>
    </div>
    <div class="node-ports">
      <div v-if="hasInput" class="port port-in" :data-port="`in-${node.id}`" @mousedown.stop="$emit('port-drag-start', $event, node.id, 'in')">
        <span class="port-dot" />
      </div>
      <div v-if="hasOutput" class="port port-out" :data-port="`out-${node.id}`" @mousedown.stop="$emit('port-drag-start', $event, node.id, 'out')">
        <span class="port-dot" />
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'

export interface DAGNodeData {
  id: string
  type: 'source' | 'decode' | 'detect' | 'track' | 'osd' | 'encode' | 'sink'
  label: string
  x: number
  y: number
  config?: Record<string, any>
}

const props = defineProps<{
  node: DAGNodeData
  selected: boolean
}>()

defineEmits<{
  select: [id: string]
  'port-drag-start': [event: MouseEvent, nodeId: string, portType: 'in' | 'out']
}>()

const nodeStyle = computed(() => ({
  left: `${props.node.x}px`,
  top: `${props.node.y}px`,
}))

const nodeType = computed(() => props.node.type)

const nodeIcon = computed(() => {
  const map: Record<string, string> = {
    source: 'VideoCamera',
    decode: 'Film',
    detect: 'View',
    track: 'Aim',
    osd: 'PictureFilled',
    encode: 'VideoPlay',
    sink: 'Upload',
  }
  return map[props.node.type] || 'Cpu'
})

const hasInput = computed(() => !['source'].includes(props.node.type))
const hasOutput = computed(() => !['sink'].includes(props.node.type))
</script>

<style scoped lang="scss">
.dag-node {
  position: absolute;
  min-width: 120px;
  background: #132f4c;
  border: 2px solid #1e3a5f;
  border-radius: 8px;
  cursor: move;
  user-select: none;
  transition: box-shadow 0.2s;

  &:hover {
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
  }

  &.selected {
    border-color: #409EFF;
    box-shadow: 0 0 12px rgba(64, 158, 255, 0.3);
  }

  &.source { border-left: 3px solid #409EFF; }
  &.decode { border-left: 3px solid #67C23A; }
  &.detect { border-left: 3px solid #E6A23C; }
  &.track { border-left: 3px solid #66B1FF; }
  &.osd { border-left: 3px solid #909399; }
  &.encode { border-left: 3px solid #67C23A; }
  &.sink { border-left: 3px solid #F56C6C; }
}

.node-header {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  color: #e3e8ef;
  font-size: 13px;
}

.node-label {
  font-weight: 500;
}

.node-ports {
  position: relative;
  height: 0;
}

.port {
  position: absolute;
  top: 50%;
  transform: translateY(-50%);
  width: 16px;
  height: 16px;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: crosshair;
}

.port-in {
  left: -8px;
}

.port-out {
  right: -8px;
}

.port-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: #409EFF;
  border: 2px solid #132f4c;
  transition: transform 0.2s;

  &:hover {
    transform: scale(1.4);
  }
}

.port-in .port-dot {
  background: #67C23A;
}

.port-out .port-dot {
  background: #409EFF;
}
</style>
