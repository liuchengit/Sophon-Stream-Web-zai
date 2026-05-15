<template>
  <div class="line-chart">
    <v-chart :option="chartOption" autoresize :style="{ height: height }" />
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'

const props = withDefaults(defineProps<{
  data: { name: string; value: number }[] | number[]
  xAxis?: string[]
  title?: string
  series?: { name: string; data: number[] }[]
  height?: string
  smooth?: boolean
  areaStyle?: boolean
}>(), {
  height: '300px',
  smooth: true,
  areaStyle: false,
})

const chartOption = computed(() => {
  if (props.series && props.xAxis) {
    return {
      title: props.title
        ? {
            text: props.title,
            textStyle: { color: '#e3e8ef', fontSize: 14 },
            left: 'center',
          }
        : undefined,
      tooltip: {
        trigger: 'axis',
        backgroundColor: '#132f4c',
        borderColor: '#1e3a5f',
        textStyle: { color: '#e3e8ef' },
      },
      legend: {
        data: props.series.map((s) => s.name),
        textStyle: { color: '#b0bec5' },
        bottom: 0,
      },
      grid: { top: 40, right: 20, bottom: 40, left: 50 },
      xAxis: {
        type: 'category',
        data: props.xAxis,
        axisLine: { lineStyle: { color: '#1e3a5f' } },
        axisLabel: { color: '#78909c', fontSize: 11 },
      },
      yAxis: {
        type: 'value',
        splitLine: { lineStyle: { color: '#1e3a5f' } },
        axisLabel: { color: '#78909c', fontSize: 11 },
      },
      series: props.series.map((s, i) => ({
        name: s.name,
        type: 'line',
        data: s.data,
        smooth: props.smooth,
        symbol: 'circle',
        symbolSize: 4,
        lineStyle: { width: 2 },
        areaStyle: props.areaStyle
          ? {
              color: {
                type: 'linear',
                x: 0, y: 0, x2: 0, y2: 1,
                colorStops: [
                  { offset: 0, color: `rgba(64, 158, 255, ${0.3 - i * 0.1})` },
                  { offset: 1, color: 'rgba(64, 158, 255, 0)' },
                ],
              },
            }
          : undefined,
        itemStyle: {
          color: ['#409EFF', '#67C23A', '#E6A23C', '#F56C6C', '#909399'][i % 5],
        },
      })),
    }
  }

  const data = Array.isArray(props.data)
    ? props.data.map((v, i) => ({ name: `${i}`, value: typeof v === 'number' ? v : v }))
    : props.data

  return {
    title: props.title
      ? {
          text: props.title,
          textStyle: { color: '#e3e8ef', fontSize: 14 },
          left: 'center',
        }
      : undefined,
    tooltip: {
      trigger: 'axis',
      backgroundColor: '#132f4c',
      borderColor: '#1e3a5f',
      textStyle: { color: '#e3e8ef' },
    },
    grid: { top: 40, right: 20, bottom: 30, left: 50 },
    xAxis: {
      type: 'category',
      data: data.map((d) => d.name),
      axisLine: { lineStyle: { color: '#1e3a5f' } },
      axisLabel: { color: '#78909c', fontSize: 11 },
    },
    yAxis: {
      type: 'value',
      splitLine: { lineStyle: { color: '#1e3a5f' } },
      axisLabel: { color: '#78909c', fontSize: 11 },
    },
    series: [
      {
        type: 'line',
        data: data.map((d) => d.value),
        smooth: props.smooth,
        symbol: 'circle',
        symbolSize: 4,
        lineStyle: { width: 2, color: '#409EFF' },
        areaStyle: props.areaStyle
          ? {
              color: {
                type: 'linear',
                x: 0, y: 0, x2: 0, y2: 1,
                colorStops: [
                  { offset: 0, color: 'rgba(64, 158, 255, 0.3)' },
                  { offset: 1, color: 'rgba(64, 158, 255, 0)' },
                ],
              },
            }
          : undefined,
        itemStyle: { color: '#409EFF' },
      },
    ],
  }
})
</script>

<style scoped lang="scss">
.line-chart {
  width: 100%;
}
</style>
