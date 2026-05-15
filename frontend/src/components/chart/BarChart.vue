<template>
  <div class="bar-chart">
    <v-chart :option="chartOption" autoresize :style="{ height: height }" />
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'

const props = withDefaults(defineProps<{
  data: { name: string; value: number }[]
  title?: string
  height?: string
  horizontal?: boolean
}>(), {
  height: '300px',
  horizontal: false,
})

const colors = ['#409EFF', '#67C23A', '#E6A23C', '#F56C6C', '#909399', '#66B1FF']

const chartOption = computed(() => {
  const names = props.data.map((d) => d.name)
  const values = props.data.map((d) => d.value)

  if (props.horizontal) {
    return {
      title: props.title
        ? { text: props.title, textStyle: { color: '#e3e8ef', fontSize: 14 }, left: 'center' }
        : undefined,
      tooltip: {
        trigger: 'axis',
        axisPointer: { type: 'shadow' },
        backgroundColor: '#132f4c',
        borderColor: '#1e3a5f',
        textStyle: { color: '#e3e8ef' },
      },
      grid: { top: 40, right: 30, bottom: 30, left: 80 },
      xAxis: {
        type: 'value',
        splitLine: { lineStyle: { color: '#1e3a5f' } },
        axisLabel: { color: '#78909c', fontSize: 11 },
      },
      yAxis: {
        type: 'category',
        data: names,
        axisLine: { lineStyle: { color: '#1e3a5f' } },
        axisLabel: { color: '#b0bec5', fontSize: 12 },
      },
      series: [{
        type: 'bar',
        data: values.map((v, i) => ({
          value: v,
          itemStyle: {
            color: colors[i % colors.length],
            borderRadius: [0, 4, 4, 0],
          },
        })),
        barWidth: '60%',
      }],
    }
  }

  return {
    title: props.title
      ? { text: props.title, textStyle: { color: '#e3e8ef', fontSize: 14 }, left: 'center' }
      : undefined,
    tooltip: {
      trigger: 'axis',
      axisPointer: { type: 'shadow' },
      backgroundColor: '#132f4c',
      borderColor: '#1e3a5f',
      textStyle: { color: '#e3e8ef' },
    },
    grid: { top: 40, right: 20, bottom: 30, left: 50 },
    xAxis: {
      type: 'category',
      data: names,
      axisLine: { lineStyle: { color: '#1e3a5f' } },
      axisLabel: { color: '#b0bec5', fontSize: 12 },
    },
    yAxis: {
      type: 'value',
      splitLine: { lineStyle: { color: '#1e3a5f' } },
      axisLabel: { color: '#78909c', fontSize: 11 },
    },
    series: [{
      type: 'bar',
      data: values.map((v, i) => ({
        value: v,
        itemStyle: {
          color: colors[i % colors.length],
          borderRadius: [4, 4, 0, 0],
        },
      })),
      barWidth: '50%',
    }],
  }
})
</script>

<style scoped lang="scss">
.bar-chart {
  width: 100%;
}
</style>
