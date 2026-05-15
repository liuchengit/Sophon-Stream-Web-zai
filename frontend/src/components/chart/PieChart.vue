<template>
  <div class="pie-chart">
    <v-chart :option="chartOption" autoresize :style="{ height: height }" />
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'

const props = withDefaults(defineProps<{
  data: { name: string; value: number }[]
  title?: string
  height?: string
  roseType?: boolean
}>(), {
  height: '300px',
  roseType: false,
})

const colors = ['#409EFF', '#67C23A', '#E6A23C', '#F56C6C', '#909399', '#66B1FF', '#E6A23C']

const chartOption = computed(() => ({
  title: props.title
    ? {
        text: props.title,
        textStyle: { color: '#e3e8ef', fontSize: 14 },
        left: 'center',
      }
    : undefined,
  tooltip: {
    trigger: 'item',
    backgroundColor: '#132f4c',
    borderColor: '#1e3a5f',
    textStyle: { color: '#e3e8ef' },
    formatter: '{b}: {c} ({d}%)',
  },
  legend: {
    orient: 'horizontal',
    bottom: 0,
    textStyle: { color: '#b0bec5', fontSize: 12 },
  },
  series: [
    {
      type: 'pie',
      radius: props.roseType ? ['20%', '60%'] : ['40%', '65%'],
      center: ['50%', '45%'],
      roseType: props.roseType ? 'area' : undefined,
      data: props.data.map((d, i) => ({
        ...d,
        itemStyle: { color: colors[i % colors.length] },
      })),
      label: {
        color: '#b0bec5',
        fontSize: 12,
      },
      labelLine: {
        lineStyle: { color: '#1e3a5f' },
      },
      emphasis: {
        itemStyle: {
          shadowBlur: 10,
          shadowOffsetX: 0,
          shadowColor: 'rgba(0, 0, 0, 0.5)',
        },
      },
    },
  ],
}))
</script>

<style scoped lang="scss">
.pie-chart {
  width: 100%;
}
</style>
