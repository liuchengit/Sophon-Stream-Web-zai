<template>
  <svg class="dag-edge">
    <path :d="pathD" :stroke="color" stroke-width="2" fill="none" :stroke-dasharray="animated ? '6,4' : 'none'" />
    <circle :cx="endX" :cy="endY" r="3" :fill="color" />
  </svg>
</template>

<script setup lang="ts">
import { computed } from 'vue'

const props = defineProps<{
  fromX: number
  fromY: number
  toX: number
  toY: number
  color?: string
  animated?: boolean
}>()

const color = computed(() => props.color || '#409EFF')

const endX = computed(() => props.toX)
const endY = computed(() => props.toY)

const pathD = computed(() => {
  const dx = Math.abs(props.toX - props.fromX)
  const controlOffset = Math.max(50, dx * 0.5)
  return `M ${props.fromX} ${props.fromY} C ${props.fromX + controlOffset} ${props.fromY}, ${props.toX - controlOffset} ${props.toY}, ${props.toX} ${props.toY}`
})
</script>

<style scoped lang="scss">
.dag-edge {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  pointer-events: none;
  overflow: visible;
}
</style>
