<template>
  <div class="roi-editor">
    <div class="roi-toolbar">
      <el-radio-group v-model="drawMode" size="small">
        <el-radio-button value="select">选择</el-radio-button>
        <el-radio-button value="rectangle">矩形</el-radio-button>
        <el-radio-button value="polygon">多边形</el-radio-button>
      </el-radio-group>
      <el-divider direction="vertical" />
      <el-button size="small" @click="handleZoomIn" :icon="ZoomIn">放大</el-button>
      <el-button size="small" @click="handleZoomOut" :icon="ZoomOut">缩小</el-button>
      <el-button size="small" @click="handleResetView">重置视图</el-button>
      <el-divider direction="vertical" />
      <el-button size="small" type="primary" @click="handleExport">导出配置</el-button>
      <el-button size="small" @click="handleImport">导入配置</el-button>
    </div>

    <div class="roi-main">
      <!-- Canvas -->
      <div class="roi-canvas-wrap" ref="canvasWrap">
        <canvas
          ref="canvasRef"
          class="roi-canvas"
          @mousedown="handleMouseDown"
          @mousemove="handleMouseMove"
          @mouseup="handleMouseUp"
          @wheel="handleWheel"
        />
        <div v-if="regions.length === 0" class="canvas-empty">
          <el-icon :size="40"><Crop /></el-icon>
          <p>选择绘制工具后在画布上绘制ROI区域</p>
        </div>
      </div>

      <!-- Region List Panel -->
      <div class="roi-panel">
        <div class="panel-header">
          <h3>区域列表</h3>
          <el-button size="small" type="primary" @click="addRegion">添加区域</el-button>
        </div>

        <div class="region-list">
          <div
            v-for="(region, idx) in regions"
            :key="idx"
            class="region-item"
            :class="{ active: selectedRegionIndex === idx }"
            @click="selectedRegionIndex = idx"
          >
            <div class="region-color">
              <el-color-picker v-model="region.color" size="small" @change="renderCanvas" />
            </div>
            <div class="region-info">
              <el-input v-model="region.name" size="small" placeholder="区域名称" @change="renderCanvas" />
              <div class="region-meta">
                <el-select v-model="region.type" size="small" style="width: 90px" @change="handleRegionTypeChange(idx)">
                  <el-option label="矩形" value="rectangle" />
                  <el-option label="多边形" value="polygon" />
                </el-select>
                <el-switch v-model="region.enabled" size="small" @change="renderCanvas" />
              </div>
            </div>
            <el-button text type="danger" size="small" @click.stop="removeRegion(idx)">
              <el-icon><Delete /></el-icon>
            </el-button>
          </div>
        </div>

        <div class="panel-footer">
          <el-button size="small" @click="clearAll">清除全部</el-button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted, onUnmounted, nextTick, watch } from 'vue'
import { ZoomIn, ZoomOut } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'

interface ROIRegion {
  name: string
  type: 'rectangle' | 'polygon'
  color: string
  points: { x: number; y: number }[]
  enabled: boolean
}

const canvasRef = ref<HTMLCanvasElement | null>(null)
const canvasWrap = ref<HTMLDivElement | null>(null)
const drawMode = ref<'select' | 'rectangle' | 'polygon'>('select')
const regions = reactive<ROIRegion[]>([])
const selectedRegionIndex = ref(-1)
const isDrawing = ref(false)
const drawStart = reactive({ x: 0, y: 0 })
const currentPolygonPoints = reactive<{ x: number; y: number }[]>([])

// View transform
const viewTransform = reactive({
  offsetX: 0,
  offsetY: 0,
  scale: 1,
})

const defaultColors = ['#409EFF', '#67C23A', '#E6A23C', '#F56C6C', '#909399', '#66B1FF']
let colorIndex = 0

function addRegion() {
  const color = defaultColors[colorIndex % defaultColors.length]
  colorIndex++
  regions.push({
    name: `区域${regions.length + 1}`,
    type: 'rectangle',
    color,
    points: [],
    enabled: true,
  })
  selectedRegionIndex.value = regions.length - 1
}

function removeRegion(idx: number) {
  regions.splice(idx, 1)
  if (selectedRegionIndex.value >= regions.length) {
    selectedRegionIndex.value = regions.length - 1
  }
  renderCanvas()
}

function handleRegionTypeChange(idx: number) {
  regions[idx].points = []
  renderCanvas()
}

function clearAll() {
  regions.splice(0, regions.length)
  selectedRegionIndex.value = -1
  renderCanvas()
}

function handleZoomIn() {
  viewTransform.scale = Math.min(viewTransform.scale * 1.2, 5)
  renderCanvas()
}

function handleZoomOut() {
  viewTransform.scale = Math.max(viewTransform.scale / 1.2, 0.2)
  renderCanvas()
}

function handleResetView() {
  viewTransform.offsetX = 0
  viewTransform.offsetY = 0
  viewTransform.scale = 1
  renderCanvas()
}

function handleWheel(e: WheelEvent) {
  e.preventDefault()
  const delta = e.deltaY > 0 ? 0.9 : 1.1
  viewTransform.scale = Math.max(0.2, Math.min(5, viewTransform.scale * delta))
  renderCanvas()
}

function screenToCanvas(e: MouseEvent) {
  const canvas = canvasRef.value
  if (!canvas) return { x: 0, y: 0 }
  const rect = canvas.getBoundingClientRect()
  return {
    x: (e.clientX - rect.left - viewTransform.offsetX) / viewTransform.scale,
    y: (e.clientY - rect.top - viewTransform.offsetY) / viewTransform.scale,
  }
}

function handleMouseDown(e: MouseEvent) {
  const pos = screenToCanvas(e)

  if (drawMode.value === 'select') {
    // Start panning
    isDrawing.value = true
    drawStart.x = e.clientX - viewTransform.offsetX
    drawStart.y = e.clientY - viewTransform.offsetY
    return
  }

  if (drawMode.value === 'rectangle') {
    isDrawing.value = true
    drawStart.x = pos.x
    drawStart.y = pos.y
    if (selectedRegionIndex.value === -1) addRegion()
    return
  }

  if (drawMode.value === 'polygon') {
    if (selectedRegionIndex.value === -1) addRegion()
    currentPolygonPoints.push(pos)
    const region = regions[selectedRegionIndex.value]
    if (region) {
      region.points = [...currentPolygonPoints]
    }
    renderCanvas()
  }
}

function handleMouseMove(e: MouseEvent) {
  if (drawMode.value === 'select' && isDrawing.value) {
    viewTransform.offsetX = e.clientX - drawStart.x
    viewTransform.offsetY = e.clientY - drawStart.y
    renderCanvas()
    return
  }

  if (drawMode.value === 'rectangle' && isDrawing.value) {
    const pos = screenToCanvas(e)
    const region = regions[selectedRegionIndex.value]
    if (region) {
      const x1 = Math.min(drawStart.x, pos.x)
      const y1 = Math.min(drawStart.y, pos.y)
      const x2 = Math.max(drawStart.x, pos.x)
      const y2 = Math.max(drawStart.y, pos.y)
      region.points = [
        { x: x1, y: y1 },
        { x: x2, y: y1 },
        { x: x2, y: y2 },
        { x: x1, y: y2 },
      ]
    }
    renderCanvas()
  }
}

function handleMouseUp() {
  isDrawing.value = false
  renderCanvas()
}

function renderCanvas() {
  const canvas = canvasRef.value
  if (!canvas) return
  const ctx = canvas.getContext('2d')
  if (!ctx) return

  const w = canvas.width
  const h = canvas.height

  ctx.clearRect(0, 0, w, h)

  // Draw background grid
  ctx.save()
  ctx.translate(viewTransform.offsetX, viewTransform.offsetY)
  ctx.scale(viewTransform.scale, viewTransform.scale)

  ctx.strokeStyle = 'rgba(64, 158, 255, 0.1)'
  ctx.lineWidth = 1 / viewTransform.scale
  for (let x = 0; x < w / viewTransform.scale; x += 40) {
    ctx.beginPath()
    ctx.moveTo(x, 0)
    ctx.lineTo(x, h / viewTransform.scale)
    ctx.stroke()
  }
  for (let y = 0; y < h / viewTransform.scale; y += 40) {
    ctx.beginPath()
    ctx.moveTo(0, y)
    ctx.lineTo(w / viewTransform.scale, y)
    ctx.stroke()
  }

  // Draw regions
  regions.forEach((region, idx) => {
    if (region.points.length < 2) return

    const isSelected = idx === selectedRegionIndex.value
    const alpha = region.enabled ? 0.3 : 0.1
    const strokeAlpha = region.enabled ? 0.9 : 0.3

    ctx.beginPath()
    ctx.moveTo(region.points[0].x, region.points[0].y)
    for (let i = 1; i < region.points.length; i++) {
      ctx.lineTo(region.points[i].x, region.points[i].y)
    }
    if (region.type === 'rectangle') {
      ctx.closePath()
    } else {
      ctx.closePath()
    }

    // Fill
    const hex = region.color.replace('#', '')
    const r = parseInt(hex.substring(0, 2), 16)
    const g = parseInt(hex.substring(2, 4), 16)
    const b = parseInt(hex.substring(4, 6), 16)
    ctx.fillStyle = `rgba(${r}, ${g}, ${b}, ${alpha})`
    ctx.fill()

    // Stroke
    ctx.strokeStyle = `rgba(${r}, ${g}, ${b}, ${strokeAlpha})`
    ctx.lineWidth = (isSelected ? 3 : 2) / viewTransform.scale
    if (isSelected) {
      ctx.setLineDash([6 / viewTransform.scale, 4 / viewTransform.scale])
    }
    ctx.stroke()
    ctx.setLineDash([])

    // Label
    const cx = region.points.reduce((sum, p) => sum + p.x, 0) / region.points.length
    const cy = region.points.reduce((sum, p) => sum + p.y, 0) / region.points.length
    ctx.fillStyle = region.color
    ctx.font = `${12 / viewTransform.scale}px sans-serif`
    ctx.textAlign = 'center'
    ctx.fillText(region.name, cx, cy - 6 / viewTransform.scale)

    // Draw points for selected
    if (isSelected) {
      region.points.forEach((p) => {
        ctx.beginPath()
        ctx.arc(p.x, p.y, 4 / viewTransform.scale, 0, Math.PI * 2)
        ctx.fillStyle = '#fff'
        ctx.fill()
        ctx.strokeStyle = region.color
        ctx.lineWidth = 2 / viewTransform.scale
        ctx.stroke()
      })
    }
  })

  ctx.restore()
}

function handleExport() {
  const config = {
    regions: regions.map((r) => ({
      name: r.name,
      type: r.type,
      color: r.color,
      points: r.points,
      enabled: r.enabled,
    })),
  }
  const json = JSON.stringify(config, null, 2)
  const blob = new Blob([json], { type: 'application/json' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = 'roi_config.json'
  a.click()
  URL.revokeObjectURL(url)
  ElMessage.success('ROI配置已导出')
}

function handleImport() {
  const input = document.createElement('input')
  input.type = 'file'
  input.accept = '.json'
  input.onchange = (e: Event) => {
    const file = (e.target as HTMLInputElement).files?.[0]
    if (!file) return
    const reader = new FileReader()
    reader.onload = (ev) => {
      try {
        const config = JSON.parse(ev.target?.result as string)
        if (config.regions && Array.isArray(config.regions)) {
          regions.splice(0, regions.length)
          config.regions.forEach((r: ROIRegion) => regions.push(r))
          selectedRegionIndex.value = 0
          renderCanvas()
          ElMessage.success('ROI配置已导入')
        }
      } catch {
        ElMessage.error('配置文件格式错误')
      }
    }
    reader.readAsText(file)
  }
  input.click()
}

function resizeCanvas() {
  const canvas = canvasRef.value
  const wrap = canvasWrap.value
  if (!canvas || !wrap) return
  canvas.width = wrap.clientWidth
  canvas.height = wrap.clientHeight
  renderCanvas()
}

onMounted(() => {
  nextTick(() => {
    resizeCanvas()
  })
  window.addEventListener('resize', resizeCanvas)
})

onUnmounted(() => {
  window.removeEventListener('resize', resizeCanvas)
})

watch(selectedRegionIndex, () => {
  renderCanvas()
})
</script>

<style scoped lang="scss">
.roi-editor {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.roi-toolbar {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 12px 16px;
  background: #132f4c;
  border: 1px solid #1e3a5f;
  border-radius: 8px;
  margin-bottom: 12px;
  flex-wrap: wrap;
}

.roi-main {
  display: flex;
  gap: 12px;
  flex: 1;
  min-height: 0;
}

.roi-canvas-wrap {
  flex: 1;
  position: relative;
  background: #0a1929;
  border: 1px solid #1e3a5f;
  border-radius: 8px;
  overflow: hidden;
  min-height: 500px;
}

.roi-canvas {
  width: 100%;
  height: 100%;
  cursor: crosshair;

  &.select-mode {
    cursor: grab;
  }
}

.canvas-empty {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  text-align: center;
  color: #546e7a;
  pointer-events: none;

  p {
    margin-top: 8px;
    font-size: 13px;
  }
}

.roi-panel {
  width: 280px;
  background: #132f4c;
  border: 1px solid #1e3a5f;
  border-radius: 8px;
  display: flex;
  flex-direction: column;
  flex-shrink: 0;
}

.panel-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 12px 16px;
  border-bottom: 1px solid #1e3a5f;

  h3 {
    font-size: 14px;
    font-weight: 600;
    color: #e3e8ef;
    margin: 0;
  }
}

.region-list {
  flex: 1;
  overflow-y: auto;
  padding: 8px;
}

.region-item {
  display: flex;
  align-items: flex-start;
  gap: 8px;
  padding: 10px;
  border-radius: 6px;
  margin-bottom: 6px;
  cursor: pointer;
  transition: background 0.2s;
  border: 1px solid transparent;

  &:hover {
    background: rgba(64, 158, 255, 0.05);
  }

  &.active {
    background: rgba(64, 158, 255, 0.1);
    border-color: #409EFF;
  }
}

.region-color {
  padding-top: 4px;
}

.region-info {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.region-meta {
  display: flex;
  align-items: center;
  gap: 8px;
}

.panel-footer {
  padding: 8px 16px;
  border-top: 1px solid #1e3a5f;
  text-align: center;
}
</style>
