import { createRouter, createWebHashHistory, RouteRecordRaw } from 'vue-router'
import NProgress from 'nprogress'
import 'nprogress/nprogress.css'

NProgress.configure({ showSpinner: false })

const routes: RouteRecordRaw[] = [
  {
    path: '/login',
    name: 'Login',
    component: () => import('@/views/Login.vue'),
    meta: { title: '登录', requiresAuth: false },
  },
  {
    path: '/',
    component: () => import('@/components/layout/AppLayout.vue'),
    redirect: '/dashboard',
    meta: { requiresAuth: true },
    children: [
      {
        path: 'dashboard',
        name: 'Dashboard',
        component: () => import('@/views/dashboard/Index.vue'),
        meta: { title: '仪表盘', icon: 'Odometer' },
      },
      {
        path: 'devices',
        name: 'DeviceList',
        component: () => import('@/views/device/Index.vue'),
        meta: { title: '设备管理', icon: 'VideoCamera' },
      },
      {
        path: 'devices/:id',
        name: 'DeviceDetail',
        component: () => import('@/views/device/Detail.vue'),
        meta: { title: '设备详情', hidden: true },
      },
      {
        path: 'tasks',
        name: 'TaskList',
        component: () => import('@/views/task/Index.vue'),
        meta: { title: '任务管理', icon: 'List' },
      },
      {
        path: 'tasks/create',
        name: 'TaskCreate',
        component: () => import('@/views/task/TaskForm.vue'),
        meta: { title: '创建任务', hidden: true },
      },
      {
        path: 'tasks/:id',
        name: 'TaskDetail',
        component: () => import('@/views/task/Detail.vue'),
        meta: { title: '任务详情', hidden: true },
      },
      {
        path: 'tasks/:id/edit',
        name: 'TaskEdit',
        component: () => import('@/views/task/TaskForm.vue'),
        meta: { title: '编辑任务', hidden: true },
      },
      {
        path: 'algorithms',
        name: 'AlgorithmList',
        component: () => import('@/views/algorithm/Index.vue'),
        meta: { title: '算法管理', icon: 'Cpu' },
      },
      {
        path: 'monitor',
        name: 'Monitor',
        component: () => import('@/views/monitor/Index.vue'),
        meta: { title: '系统监控', icon: 'Monitor' },
      },
      {
        path: 'alerts',
        name: 'AlertList',
        component: () => import('@/views/alert/Index.vue'),
        meta: { title: '报警管理', icon: 'Bell' },
      },
      {
        path: 'alerts/rules',
        name: 'AlertRules',
        component: () => import('@/views/alert/Rules.vue'),
        meta: { title: '报警规则', hidden: true },
      },
      {
        path: 'plugins',
        name: 'PluginList',
        component: () => import('@/views/plugin/Index.vue'),
        meta: { title: '插件管理', icon: 'Connection' },
      },
      {
        path: 'system',
        name: 'SystemSettings',
        component: () => import('@/views/system/Index.vue'),
        meta: { title: '系统设置', icon: 'Setting' },
      },
      {
        path: 'system/users',
        name: 'UserManagement',
        component: () => import('@/views/system/Users.vue'),
        meta: { title: '用户管理', hidden: true },
      },
      {
        path: 'system/audit',
        name: 'AuditLog',
        component: () => import('@/views/system/Audit.vue'),
        meta: { title: '审计日志', hidden: true },
      },
      {
        path: 'system/firmware',
        name: 'FirmwareManagement',
        component: () => import('@/views/system/Firmware.vue'),
        meta: { title: '固件管理', hidden: true },
      },
      {
        path: 'roi-editor',
        name: 'ROIEditor',
        component: () => import('@/components/roi/ROIEditor.vue'),
        meta: { title: 'ROI编辑器', icon: 'Crop' },
      },
      {
        path: 'orchestration',
        name: 'DAGOrchestration',
        component: () => import('@/components/dag/DAGEditor.vue'),
        meta: { title: '流程编排', icon: 'Share' },
      },
    ],
  },
  {
    path: '/:pathMatch(.*)*',
    redirect: '/dashboard',
  },
]

const router = createRouter({
  history: createWebHashHistory(),
  routes,
  scrollBehavior(to, from, savedPosition) {
    return new Promise((resolve) => {
      setTimeout(() => {
        resolve(savedPosition || { top: 0 })
      }, 100)
    })
  },
})

// Navigation guard
router.beforeEach((to, _from, next) => {
  NProgress.start()
  document.title = `${to.meta.title || 'Sophon-Stream'} - 智能视频分析平台`

  const token = localStorage.getItem('token')

  if (to.meta.requiresAuth !== false && !token) {
    next({ path: '/login', query: { redirect: to.fullPath } })
  } else if (to.path === '/login' && token) {
    next('/dashboard')
  } else {
    next()
  }
})

router.afterEach(() => {
  NProgress.done()
})

export default router
