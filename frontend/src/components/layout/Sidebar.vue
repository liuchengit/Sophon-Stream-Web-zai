<template>
  <div class="sidebar" :class="{ collapsed }">
    <div class="sidebar-logo">
      <div class="logo-icon">
        <el-icon :size="24"><VideoCamera /></el-icon>
      </div>
      <span v-show="!collapsed" class="logo-text">Sophon-Stream</span>
    </div>

    <el-scrollbar class="sidebar-menu-wrap">
      <el-menu
        :default-active="activeRoute"
        :collapse="collapsed"
        :collapse-transition="true"
        background-color="#0d2137"
        text-color="#b0bec5"
        active-text-color="#409EFF"
        router
      >
        <!-- 监控中心 -->
        <el-menu-item-group>
          <template #title>
            <span v-if="!collapsed">监控中心</span>
          </template>
          <el-menu-item index="/dashboard">
            <el-icon><Odometer /></el-icon>
            <template #title>仪表盘</template>
          </el-menu-item>
          <el-menu-item index="/monitor">
            <el-icon><Monitor /></el-icon>
            <template #title>系统监控</template>
          </el-menu-item>
        </el-menu-item-group>

        <!-- 资源管理 -->
        <el-menu-item-group>
          <template #title>
            <span v-if="!collapsed">资源管理</span>
          </template>
          <el-menu-item index="/devices">
            <el-icon><VideoCamera /></el-icon>
            <template #title>设备管理</template>
          </el-menu-item>
          <el-menu-item index="/tasks">
            <el-icon><List /></el-icon>
            <template #title>任务管理</template>
          </el-menu-item>
        </el-menu-item-group>

        <!-- 算法管理 -->
        <el-menu-item-group>
          <template #title>
            <span v-if="!collapsed">算法管理</span>
          </template>
          <el-menu-item index="/algorithms">
            <el-icon><Cpu /></el-icon>
            <template #title>算法管理</template>
          </el-menu-item>
          <el-menu-item index="/orchestration">
            <el-icon><Share /></el-icon>
            <template #title>流程编排</template>
          </el-menu-item>
          <el-menu-item index="/roi-editor">
            <el-icon><Crop /></el-icon>
            <template #title>ROI编辑器</template>
          </el-menu-item>
        </el-menu-item-group>

        <!-- 报警管理 -->
        <el-menu-item-group>
          <template #title>
            <span v-if="!collapsed">报警管理</span>
          </template>
          <el-menu-item index="/alerts">
            <el-icon><Bell /></el-icon>
            <template #title>报警列表</template>
          </el-menu-item>
          <el-menu-item index="/alerts/rules">
            <el-icon><AlarmClock /></el-icon>
            <template #title>报警规则</template>
          </el-menu-item>
        </el-menu-item-group>

        <!-- 系统管理 -->
        <el-menu-item-group v-if="authStore.isAdmin || authStore.isOperator">
          <template #title>
            <span v-if="!collapsed">系统管理</span>
          </template>
          <el-menu-item index="/plugins">
            <el-icon><Connection /></el-icon>
            <template #title>插件管理</template>
          </el-menu-item>
          <el-menu-item index="/system">
            <el-icon><Setting /></el-icon>
            <template #title>系统设置</template>
          </el-menu-item>
          <el-menu-item v-if="authStore.isAdmin" index="/system/users">
            <el-icon><User /></el-icon>
            <template #title>用户管理</template>
          </el-menu-item>
          <el-menu-item v-if="authStore.isAdmin" index="/system/audit">
            <el-icon><Document /></el-icon>
            <template #title>审计日志</template>
          </el-menu-item>
          <el-menu-item v-if="authStore.isAdmin" index="/system/firmware">
            <el-icon><Upload /></el-icon>
            <template #title>固件管理</template>
          </el-menu-item>
        </el-menu-item-group>
      </el-menu>
    </el-scrollbar>

    <div class="sidebar-toggle" @click="$emit('toggle')">
      <el-icon :size="16">
        <DArrowLeft v-if="!collapsed" />
        <DArrowRight v-else />
      </el-icon>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useRoute } from 'vue-router'
import { useAuthStore } from '@/stores/auth'

defineProps<{
  collapsed: boolean
}>()

defineEmits<{
  toggle: []
}>()

const route = useRoute()
const authStore = useAuthStore()

const activeRoute = computed(() => {
  // For nested routes, return the first-level path
  const path = route.path
  if (path.startsWith('/tasks/') && path !== '/tasks') return '/tasks'
  if (path.startsWith('/devices/') && path !== '/devices') return '/devices'
  return path
})
</script>

<style scoped lang="scss">
.sidebar {
  position: fixed;
  left: 0;
  top: 0;
  bottom: 0;
  width: 220px;
  background: #0d2137;
  border-right: 1px solid #1e3a5f;
  display: flex;
  flex-direction: column;
  transition: width 0.3s ease;
  z-index: 100;
  overflow: hidden;

  &.collapsed {
    width: 64px;
  }
}

.sidebar-logo {
  height: 56px;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 10px;
  border-bottom: 1px solid #1e3a5f;
  padding: 0 16px;
  flex-shrink: 0;
}

.logo-icon {
  color: #409EFF;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.logo-text {
  font-size: 16px;
  font-weight: 700;
  color: #e3e8ef;
  white-space: nowrap;
  background: linear-gradient(135deg, #409EFF, #66B1FF);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
}

.sidebar-menu-wrap {
  flex: 1;
  overflow-y: auto;

  :deep(.el-menu) {
    border-right: none;
  }

  :deep(.el-menu-item-group__title) {
    color: #546e7a;
    font-size: 12px;
    padding: 12px 0 4px 16px;
    letter-spacing: 2px;
  }

  :deep(.el-menu-item) {
    height: 44px;
    line-height: 44px;

    &:hover {
      background: #132f4c !important;
    }

    &.is-active {
      background: linear-gradient(90deg, rgba(64, 158, 255, 0.15), transparent) !important;
      border-right: 2px solid #409EFF;
    }
  }
}

.sidebar-toggle {
  height: 40px;
  display: flex;
  align-items: center;
  justify-content: center;
  border-top: 1px solid #1e3a5f;
  cursor: pointer;
  color: #78909c;
  flex-shrink: 0;
  transition: all 0.3s ease;

  &:hover {
    color: #409EFF;
    background: #132f4c;
  }
}
</style>
