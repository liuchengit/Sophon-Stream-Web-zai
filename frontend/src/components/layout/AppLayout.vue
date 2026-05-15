<template>
  <el-container class="app-layout">
    <Sidebar :collapsed="sidebarCollapsed" @toggle="sidebarCollapsed = !sidebarCollapsed" />
    <el-container class="main-container" :class="{ 'sidebar-collapsed': sidebarCollapsed }">
      <Header @toggle-sidebar="sidebarCollapsed = !sidebarCollapsed" />
      <el-main class="main-content">
        <router-view v-slot="{ Component }">
          <transition name="fade" mode="out-in">
            <component :is="Component" />
          </transition>
        </router-view>
      </el-main>
      <el-footer class="app-footer" height="32px">
        <div class="footer-content">
          <span class="footer-left">Sophon-Stream 智能视频分析平台 v1.0</span>
          <span class="footer-right">
            <span class="system-status" :class="wsConnected ? 'online' : 'offline'">
              <span class="status-dot"></span>
              {{ wsConnected ? '系统在线' : '系统离线' }}
            </span>
          </span>
        </div>
      </el-footer>
    </el-container>
  </el-container>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue'
import Sidebar from './Sidebar.vue'
import Header from './Header.vue'
import { useMonitorStore } from '@/stores/monitor'

const sidebarCollapsed = ref(false)
const monitorStore = useMonitorStore()
const wsConnected = ref(false)
let ws: WebSocket | null = null
let reconnectTimer: ReturnType<typeof setTimeout> | null = null

function connectWebSocket() {
  const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:'
  const wsUrl = `${protocol}//${window.location.host}/ws/monitor`
  ws = new WebSocket(wsUrl)

  ws.onopen = () => {
    wsConnected.value = true
    monitorStore.setWsConnected(true)
  }

  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data)
      monitorStore.updateMetricsFromWS(data)
    } catch {
      // ignore parse errors
    }
  }

  ws.onclose = () => {
    wsConnected.value = false
    monitorStore.setWsConnected(false)
    scheduleReconnect()
  }

  ws.onerror = () => {
    ws?.close()
  }
}

function scheduleReconnect() {
  if (reconnectTimer) clearTimeout(reconnectTimer)
  reconnectTimer = setTimeout(() => {
    connectWebSocket()
  }, 5000)
}

onMounted(() => {
  connectWebSocket()
})

onUnmounted(() => {
  if (reconnectTimer) clearTimeout(reconnectTimer)
  ws?.close()
})
</script>

<style scoped lang="scss">
.app-layout {
  height: 100vh;
  overflow: hidden;
}

.main-container {
  margin-left: 220px;
  transition: margin-left 0.3s ease;
  height: 100vh;
  display: flex;
  flex-direction: column;

  &.sidebar-collapsed {
    margin-left: 64px;
  }
}

.main-content {
  flex: 1;
  overflow-y: auto;
  background: #0a1929;
  padding: 20px;
}

.app-footer {
  background: #0d2137;
  border-top: 1px solid #1e3a5f;
  display: flex;
  align-items: center;
  padding: 0 20px;
}

.footer-content {
  width: 100%;
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: 12px;
  color: #78909c;
}

.system-status {
  display: flex;
  align-items: center;
  gap: 6px;

  &.online .status-dot {
    background: #67C23A;
    box-shadow: 0 0 6px #67C23A;
  }

  &.offline .status-dot {
    background: #F56C6C;
    box-shadow: 0 0 6px #F56C6C;
  }
}

.status-dot {
  width: 6px;
  height: 6px;
  border-radius: 50%;
  display: inline-block;
}
</style>
