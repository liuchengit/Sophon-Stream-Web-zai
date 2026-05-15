<template>
  <div class="dag-editor">
    <div class="dag-toolbar">
      <el-radio-group v-model="editMode" size="small">
        <el-radio-button value="select">选择</el-radio-button>
        <el-radio-button value="add">添加节点</el-radio-button>
        <el-radio-button value="connect">连接</el-radio-button>
      </el-radio-group>
      <el-divider direction="vertical" />
      <el-select v-if="editMode === 'add'" v-model="newNodeType" size="small" style="width: 130px">
        <el-option label="视频源" value="source" />
        <el-option label="解码" value="decode" />
        <el-option label="检测" value="detect" />
        <el-option label="跟踪" value="track" />
        <el-option label="OSD" value="osd" />
        <el-option label="编码" value="encode" />
        <el-option label="输出" value="sink" />
      </el-select>
      <el-divider direction="vertical" />
      <el-button size="small" @click="handleZoomIn" :icon="ZoomIn">放大</el-button>
      <el-button size="small" @click="handleZoomOut" :icon="ZoomOut">缩小</el-button>
      <el-button size="small" @click="handleResetView">重置</el-button>
      <el-divider direction="vertical" />
      <el-button size="small" type="success" @click="handleValidate">验证</el-button>
      <el-button size="small" type="primary" @click="handleExport">导出</el-button>
      <el-button size="small" @click="handleImport">导入</el-button>
    </div>

    <div class="dag-workspace">
      <!-- Canvas Area -->
      <div
        class="dag-canvas"
        ref="canvasRef"
        @mousedown="handleCanvasMouseDown"
        @mousemove="handleCanvasMouseMove"
        @mouseup="handleCanvasMouseUp"
        @wheel="handleWheel"
        @click="handleCanvasClick"
      >
        <svg class="dag-svg" :style="svgStyle">
          <defs>
            <pattern id="grid" width="40" height="40" patternUnits="userSpaceOnUse">
              <path d="M 40 0 L 0 0 0 40" fill="none" stroke="rgba(64,158,255,0.08)" stroke-width="1" />
            </pattern>
          </defs>
          <rect width="10000" height="10000" fill="url(#grid)" />
        </svg>

        <!-- Edges -->
        <DAGEdge
          v-for="edge in edges"
          :key="edge.id"
          :from-x="getNodeOutputX(edge.from)"
          :from-y="getNodeOutputY(edge.from)"
          :to-x="getNodeInputX(edge.to)"
          :to-y="getNodeInputY(edge.to)"
          :animated="true"
        />

        <!-- Temp edge while connecting -->
        <DAGEdge
          v-if="connectingFrom && tempEdgeEnd"
          :from-x="getNodeOutputX(connectingFrom)"
          :from-y="getNodeOutputY(connectingFrom)"
          :to-x="tempEdgeEnd.x"
          :to-y="tempEdgeEnd.y"
          color="#E6A23C"
        />

        <!-- Nodes -->
        <DAGNodeComp
          v-for="node in nodes"
          :key="node.id"
          :node="node"
          :selected="selectedNodeId === node.id"
          @select="handleNodeSelect"
          @mousedown.stop="handleNodeDragStart($event, node.id)"
          @port-drag-start="handlePortDragStart"
        />
      </div>

      <!-- Node Config Panel -->
      <div v-if="selectedNode" class="dag-config-panel">
        <div class="panel-header">
          <h3>节点配置</h3>
          <el-button text type="danger" size="small" @click="handleDeleteNode">
            <el-icon><Delete /></el-icon>删除
          </el-button>
        </div>
        <div class="panel-body">
          <el-form label-width="60px" size="small">
            <el-form-item label="名称">
              <el-input v-model="selectedNode.label" />
            </el-form-item>
            <el-form-item label="类型">
              <el-tag size="small">{{ selectedNode.type }}</el-tag>
            </el-form-item>
            <el-form-item label="配置">
              <JsonEditor v-model="nodeConfig" title="" :rows="8" />
            </el-form-item>
          </el-form>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed, onMounted, onUnmounted } from 'vue'
import { ZoomIn, ZoomOut } from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import DAGNodeComp, { type DAGNodeData } from './DAGNode.vue'
import DAGEdge from './DAGEdge.vue'
import JsonEditor from '@/components/common/JsonEditor.vue'

interface DAGEdgeData {
  id: string
  from: string
  to: string
}

const canvasRef = ref<HTMLDivElement | null>(null)
const editMode = ref<'select' | 'add' | 'connect'>('select')
const newNodeType = ref<'source' | 'decode' | 'detect' | 'track' | 'osd' | 'encode' | 'sink'>('detect')
const nodes = reactive<DAGNodeData[]>([])
const edges = reactive<DAGEdgeData[]>([])
const selectedNodeId = ref<string | null>(null)
const connectingFrom = ref<string | null>(null)
const tempEdgeEnd = ref<{ x: number; y: number } | null>(null)

// View transform
const viewTransform = reactive({ offsetX: 0, offsetY: 0, scale: 1 })

// Drag state
let draggingNodeId: string | null = null
let dragStartX = 0
let dragStartY = 0
let nodeStartX = 0
let nodeStartY = 0
let isPanning = false
let panStartX = 0
let panStartY = 0

const svgStyle = computed(() => ({
  transform: `translate(${viewTransform.offsetX}px, ${viewTransform.offsetY}px) scale(${viewTransform.scale})`,
  transformOrigin: '0 0',
}))

const selectedNode = computed(() => nodes.find((n) => n.id === selectedNodeId.value) || null)

const nodeConfig = computed({
  get: () => selectedNode.value?.config || {},
  set: (val) => {
    const node = nodes.find((n) => n.id === selectedNodeId.value)
    if (node) node.config = val
  },
})

let nodeIdCounter = 0
function generateId() {
  return `node_${++nodeIdCounter}_${Date.now()}`
}

function getNodeOutputX(nodeId: string) {
  const node = nodes.find((n) => n.id === nodeId)
  return node ? (node.x + 120) * viewTransform.scale + viewTransform.offsetX : 0
}

function getNodeOutputY(nodeId: string) {
  const node = nodes.find((n) => n.id === nodeId)
  return node ? (node.y + 20) * viewTransform.scale + viewTransform.offsetY : 0
}

function getNodeInputX(nodeId: string) {
  const node = nodes.find((n) => n.id === nodeId)
  return node ? node.x * viewTransform.scale + viewTransform.offsetX : 0
}

function getNodeInputY(nodeId: string) {
  const node = nodes.find((n) => n.id === nodeId)
  return node ? (node.y + 20) * viewTransform.scale + viewTransform.offsetY : 0
}

function handleCanvasClick(e: MouseEvent) {
  if (editMode.value === 'add') {
    const rect = (e.currentTarget as HTMLElement).getBoundingClientRect()
    const x = (e.clientX - rect.left - viewTransform.offsetX) / viewTransform.scale
    const y = (e.clientY - rect.top - viewTransform.offsetY) / viewTransform.scale
    const labelMap: Record<string, string> = {
      source: '视频源', decode: '解码', detect: '检测', track: '跟踪',
      osd: 'OSD', encode: '编码', sink: '输出',
    }
    nodes.push({
      id: generateId(),
      type: newNodeType.value,
      label: labelMap[newNodeType.value] || newNodeType.value,
      x,
      y,
      config: {},
    })
  } else if (editMode.value === 'select') {
    selectedNodeId.value = null
  }
}

function handleNodeSelect(id: string) {
  if (editMode.value === 'connect') {
    if (!connectingFrom.value) {
      connectingFrom.value = id
      ElMessage.info('请点击目标节点完成连接')
    } else {
      if (connectingFrom.value !== id) {
        const exists = edges.some((e) => e.from === connectingFrom.value && e.to === id)
        if (!exists) {
          edges.push({
            id: `edge_${Date.now()}`,
            from: connectingFrom.value,
            to: id,
          })
        }
      }
      connectingFrom.value = null
      tempEdgeEnd.value = null
    }
  } else {
    selectedNodeId.value = id
  }
}

function handleNodeDragStart(e: MouseEvent, nodeId: string) {
  if (editMode.value !== 'select') return
  draggingNodeId = nodeId
  const node = nodes.find((n) => n.id === nodeId)
  if (node) {
    dragStartX = e.clientX
    dragStartY = e.clientY
    nodeStartX = node.x
    nodeStartY = node.y
  }
}

function handlePortDragStart(_e: MouseEvent, nodeId: string, portType: 'in' | 'out') {
  if (portType === 'out') {
    editMode.value = 'connect'
    connectingFrom.value = nodeId
  }
}

function handleCanvasMouseDown(e: MouseEvent) {
  if (editMode.value === 'select' && !draggingNodeId) {
    isPanning = true
    panStartX = e.clientX - viewTransform.offsetX
    panStartY = e.clientY - viewTransform.offsetY
  }
}

function handleCanvasMouseMove(e: MouseEvent) {
  if (draggingNodeId) {
    const node = nodes.find((n) => n.id === draggingNodeId)
    if (node) {
      node.x = nodeStartX + (e.clientX - dragStartX) / viewTransform.scale
      node.y = nodeStartY + (e.clientY - dragStartY) / viewTransform.scale
    }
  } else if (isPanning) {
    viewTransform.offsetX = e.clientX - panStartX
    viewTransform.offsetY = e.clientY - panStartY
  }

  if (connectingFrom.value) {
    const rect = canvasRef.value?.getBoundingClientRect()
    if (rect) {
      tempEdgeEnd.value = {
        x: e.clientX - rect.left,
        y: e.clientY - rect.top,
      }
    }
  }
}

function handleCanvasMouseUp() {
  draggingNodeId = null
  isPanning = false
}

function handleWheel(e: WheelEvent) {
  e.preventDefault()
  const delta = e.deltaY > 0 ? 0.9 : 1.1
  viewTransform.scale = Math.max(0.2, Math.min(3, viewTransform.scale * delta))
}

function handleZoomIn() {
  viewTransform.scale = Math.min(3, viewTransform.scale * 1.2)
}

function handleZoomOut() {
  viewTransform.scale = Math.max(0.2, viewTransform.scale / 1.2)
}

function handleResetView() {
  viewTransform.offsetX = 0
  viewTransform.offsetY = 0
  viewTransform.scale = 1
}

function handleDeleteNode() {
  if (!selectedNodeId.value) return
  const id = selectedNodeId.value
  const idx = nodes.findIndex((n) => n.id === id)
  if (idx !== -1) {
    nodes.splice(idx, 1)
    // Remove connected edges
    for (let i = edges.length - 1; i >= 0; i--) {
      if (edges[i].from === id || edges[i].to === id) {
        edges.splice(i, 1)
      }
    }
    selectedNodeId.value = null
  }
}

function handleValidate() {
  const errors: string[] = []
  if (nodes.length === 0) {
    errors.push('没有节点')
  }

  // Check for cycles using DFS
  const visited = new Set<string>()
  const recursionStack = new Set<string>()

  function hasCycle(nodeId: string): boolean {
    visited.add(nodeId)
    recursionStack.add(nodeId)
    const outEdges = edges.filter((e) => e.from === nodeId)
    for (const edge of outEdges) {
      if (!visited.has(edge.to)) {
        if (hasCycle(edge.to)) return true
      } else if (recursionStack.has(edge.to)) {
        return true
      }
    }
    recursionStack.delete(nodeId)
    return false
  }

  for (const node of nodes) {
    if (!visited.has(node.id)) {
      if (hasCycle(node.id)) {
        errors.push('存在循环依赖')
        break
      }
    }
  }

  // Check for disconnected nodes
  const connectedNodes = new Set<string>()
  edges.forEach((e) => {
    connectedNodes.add(e.from)
    connectedNodes.add(e.to)
  })
  if (nodes.length > 1) {
    const disconnected = nodes.filter((n) => !connectedNodes.has(n.id))
    if (disconnected.length > 0) {
      errors.push(`${disconnected.length} 个节点未连接: ${disconnected.map((n) => n.label).join(', ')}`)
    }
  }

  if (errors.length > 0) {
    ElMessage.error(`验证失败: ${errors.join('; ')}`)
  } else {
    ElMessage.success('DAG验证通过')
  }
}

function handleExport() {
  const graph = {
    nodes: nodes.map((n) => ({
      id: n.id,
      type: n.type,
      label: n.label,
      config: n.config || {},
    })),
    edges: edges.map((e) => ({
      source: e.from,
      target: e.to,
    })),
  }
  const json = JSON.stringify(graph, null, 2)
  const blob = new Blob([json], { type: 'application/json' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = 'pipeline_config.json'
  a.click()
  URL.revokeObjectURL(url)
  ElMessage.success('Pipeline配置已导出')
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
        nodes.splice(0, nodes.length)
        edges.splice(0, edges.length)
        if (config.nodes) {
          config.nodes.forEach((n: any, i: number) => {
            nodes.push({
              id: n.id || generateId(),
              type: n.type,
              label: n.label || n.type,
              x: 100 + (i % 4) * 200,
              y: 80 + Math.floor(i / 4) * 120,
              config: n.config || {},
            })
          })
        }
        if (config.edges) {
          config.edges.forEach((e: any) => {
            edges.push({
              id: `edge_${Date.now()}_${Math.random()}`,
              from: e.source || e.from,
              to: e.target || e.to,
            })
          })
        }
        ElMessage.success('Pipeline配置已导入')
      } catch {
        ElMessage.error('配置文件格式错误')
      }
    }
    reader.readAsText(file)
  }
  input.click()
}

// Add default pipeline template
onMounted(() => {
  const defaultNodes: DAGNodeData[] = [
    { id: 'src_1', type: 'source', label: '视频源', x: 60, y: 100, config: {} },
    { id: 'dec_1', type: 'decode', label: '解码', x: 260, y: 100, config: {} },
    { id: 'det_1', type: 'detect', label: '检测', x: 460, y: 60, config: {} },
    { id: 'osd_1', type: 'osd', label: 'OSD', x: 660, y: 60, config: {} },
    { id: 'enc_1', type: 'encode', label: '编码', x: 660, y: 160, config: {} },
    { id: 'sink_1', type: 'sink', label: '输出', x: 860, y: 110, config: {} },
  ]
  const defaultEdges: DAGEdgeData[] = [
    { id: 'e1', from: 'src_1', to: 'dec_1' },
    { id: 'e2', from: 'dec_1', to: 'det_1' },
    { id: 'e3', from: 'det_1', to: 'osd_1' },
    { id: 'e4', from: 'osd_1', to: 'enc_1' },
    { id: 'e5', from: 'enc_1', to: 'sink_1' },
  ]
  nodes.push(...defaultNodes)
  edges.push(...defaultEdges)
})
</script>

<style scoped lang="scss">
.dag-editor {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.dag-toolbar {
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

.dag-workspace {
  display: flex;
  gap: 12px;
  flex: 1;
  min-height: 0;
}

.dag-canvas {
  flex: 1;
  position: relative;
  background: #0a1929;
  border: 1px solid #1e3a5f;
  border-radius: 8px;
  overflow: hidden;
  min-height: 500px;
  cursor: default;
}

.dag-svg {
  position: absolute;
  top: 0;
  left: 0;
  width: 10000px;
  height: 10000px;
  pointer-events: none;
}

.dag-config-panel {
  width: 300px;
  background: #132f4c;
  border: 1px solid #1e3a5f;
  border-radius: 8px;
  flex-shrink: 0;
  display: flex;
  flex-direction: column;
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

.panel-body {
  flex: 1;
  padding: 16px;
  overflow-y: auto;
}
</style>
