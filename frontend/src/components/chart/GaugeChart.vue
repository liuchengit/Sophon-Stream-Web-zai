<template>
  <div class="gauge-chart">
    <v-chart :option="chartOption" autoresize :style="{ height: height }" />
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'

const props = withDefaults(defineProps<{
  value: number
  max?: number
  title?: string
  color?: string
  height?: string
}>(), {
  max: 100,
  color: '#409EFF',
  height: '200px',
})

const chartOption = computed(() => ({
  series: [
    {
      type: 'gauge',
      startAngle: 220,
      endAngle: -40,
      min: 0,
      max: props.max,
      splitNumber: 5,
      radius: '90%',
      axisLine: {
        lineStyle: {
          width: 12,
          color: [
            [0.3, '#67C23A'],
            [0.7, '#E6A23C'],
            [1, '#F56C6C'],
          ],
        },
      },
      pointer: {
        icon: 'path://M12.8,0.7l12,40.1H0.7L12.8,0.7z',
        length: '55%',
        width: 8,
        offsetCenter: [0, '-10%'],
        itemStyle: {
          color: 'auto',
        },
      },
      axisTick: {
        length: 4,
        lineStyle: {
          color: 'auto',
          width: 1,
        },
      },
      splitLine: {
        length: 12,
        lineStyle: {
          color: 'auto',
          width: 2,
        },
      },
      axisLabel: {
        color: '#78909c',
        fontSize: 10,
        distance: 8,
        formatter: (value: number) => `${value}%`,
      },
      title: {
        offsetCenter: [0, '65%'],
        fontSize: 13,
        color: '#b0bec5',
      },
      detail: {
        fontSize: 24,
        offsetCenter: [0, '35%'],
        valueAnimation: true,
        formatter: (value: number) => `${value.toFixed(1)}%`,
        color: '#e3e8ef',
      },
      data: [
        {
          value: props.value,
          name: props.title || '',
        },
      ],
    },
  ],
}))
</script>

<style scoped lang="scss">
.gauge-chart {
  width: 100%;
}
</style>
