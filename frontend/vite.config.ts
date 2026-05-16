import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import AutoImport from 'unplugin-auto-import/vite'
import Components from 'unplugin-vue-components/vite'
import { ElementPlusResolver } from 'unplugin-vue-components/resolvers'
import { mockApiPlugin } from './mock-api'
import path from 'path'

export default defineConfig({
  
  plugins: [
    vue(),
    // Disable mock API when real backend is available
    // mockApiPlugin(),
    AutoImport({
      resolvers: [ElementPlusResolver()],
      imports: ['vue', 'vue-router', 'pinia'],
      dts: 'src/auto-imports.d.ts',
    }),
    Components({
      resolvers: [ElementPlusResolver()],
      dts: 'src/components.d.ts',
    }),
  ],
  resolve: {
    alias: {
      '@': path.resolve(__dirname, 'src'),
    },
  },
  server: {
    host: '0.0.0.0',
    port: 3000,
    allowedHosts: ['.monkeycode-ai.online'],
    proxy: {
      '/sophon/api': {
        target: 'http://localhost:8080',
        changeOrigin: true,
        rewrite: (path) => path.replace(/^\/sophon/, ''),
      },
      '/api': {
        target: 'http://localhost:8080',
        changeOrigin: true,
      },
    },
  },
  css: {
    preprocessorOptions: {
      scss: {
        additionalData: `@use "@/assets/styles/variables.scss" as *;`,
        api: 'modern-compiler',
      },
    },
  },
  build: {
    chunkSizeWarningLimit: 1500,
    rollupOptions: {
      output: {
        manualChunks(id) {
          if (id.includes('node_modules')) {
            if (id.includes('@element-plus/icons-vue')) {
              return 'el-icons'
            }
            if (id.includes('element-plus')) {
              return 'element-plus'
            }
            if (id.includes('echarts') || id.includes('zrender')) {
              return 'echarts'
            }
            if (id.includes('vue') || id.includes('pinia') || id.includes('@vueuse')) {
              return 'vue-vendor'
            }
            if (id.includes('axios') || id.includes('dayjs') || id.includes('nprogress')) {
              return 'utils-vendor'
            }
            return 'vendor-other'
          }
        },
      },
    },
  },
})
