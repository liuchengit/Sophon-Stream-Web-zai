<template>
  <div class="login-page">
    <!-- Animated background -->
    <div class="bg-grid"></div>
    <div class="bg-particles">
      <span v-for="i in 20" :key="i" class="particle" :style="particleStyle(i)"></span>
    </div>

    <div class="login-container">
      <div class="login-card">
        <div class="login-header">
          <div class="login-logo">
            <el-icon :size="36"><VideoCamera /></el-icon>
          </div>
          <h1 class="login-title">Sophon-Stream</h1>
          <p class="login-subtitle">智能视频分析管理平台</p>
        </div>

        <el-form
          ref="loginFormRef"
          :model="loginForm"
          :rules="loginRules"
          class="login-form"
          @keyup.enter="handleLogin"
        >
          <el-form-item prop="username">
            <el-input
              v-model="loginForm.username"
              placeholder="请输入用户名"
              :prefix-icon="User"
              size="large"
            />
          </el-form-item>

          <el-form-item prop="password">
            <el-input
              v-model="loginForm.password"
              type="password"
              placeholder="请输入密码"
              :prefix-icon="Lock"
              size="large"
              show-password
            />
          </el-form-item>

          <el-form-item>
            <div class="login-options">
              <el-checkbox v-model="rememberMe">记住我</el-checkbox>
            </div>
          </el-form-item>

          <el-form-item>
            <el-button
              type="primary"
              size="large"
              :loading="loading"
              class="login-btn"
              @click="handleLogin"
            >
              {{ loading ? '登录中...' : '登 录' }}
            </el-button>
          </el-form-item>
        </el-form>

        <div class="login-footer">
          <span>© 2024 Sophon-Stream 智能视频分析平台</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { useAuthStore } from '@/stores/auth'
import { User, Lock } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import type { FormInstance, FormRules } from 'element-plus'

const router = useRouter()
const route = useRoute()
const authStore = useAuthStore()

const loginFormRef = ref<FormInstance>()
const loading = ref(false)
const rememberMe = ref(false)

const loginForm = reactive({
  username: '',
  password: '',
})

const loginRules: FormRules = {
  username: [
    { required: true, message: '请输入用户名', trigger: 'blur' },
    { min: 2, max: 32, message: '用户名长度在2到32个字符', trigger: 'blur' },
  ],
  password: [
    { required: true, message: '请输入密码', trigger: 'blur' },
    { min: 6, max: 32, message: '密码长度在6到32个字符', trigger: 'blur' },
  ],
}

function particleStyle(i: number) {
  const size = Math.random() * 4 + 2
  return {
    width: `${size}px`,
    height: `${size}px`,
    left: `${Math.random() * 100}%`,
    top: `${Math.random() * 100}%`,
    animationDelay: `${Math.random() * 6}s`,
    animationDuration: `${Math.random() * 4 + 4}s`,
  }
}

async function handleLogin() {
  const form = loginFormRef.value
  if (!form) return

  await form.validate()
  loading.value = true

  try {
    await authStore.login({
      username: loginForm.username,
      password: loginForm.password,
    })

    if (rememberMe.value) {
      localStorage.setItem('remembered_username', loginForm.username)
    } else {
      localStorage.removeItem('remembered_username')
    }

    ElMessage.success('登录成功')
    const redirect = (route.query.redirect as string) || '/dashboard'
    router.push(redirect)
  } catch (error: any) {
    // Error already handled by interceptor
  } finally {
    loading.value = false
  }
}

// Auto-fill remembered username
const remembered = localStorage.getItem('remembered_username')
if (remembered) {
  loginForm.username = remembered
  rememberMe.value = true
}
</script>

<style scoped lang="scss">
.login-page {
  width: 100vw;
  height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  background: #0a1929;
  position: relative;
  overflow: hidden;
}

// Grid background
.bg-grid {
  position: absolute;
  top: 0;
  left: 0;
  width: 200%;
  height: 200%;
  background-image:
    linear-gradient(rgba(64, 158, 255, 0.05) 1px, transparent 1px),
    linear-gradient(90deg, rgba(64, 158, 255, 0.05) 1px, transparent 1px);
  background-size: 40px 40px;
  animation: gridMove 20s linear infinite;
}

// Floating particles
.bg-particles {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
}

.particle {
  position: absolute;
  background: rgba(64, 158, 255, 0.4);
  border-radius: 50%;
  animation: float 6s ease-in-out infinite;
}

@keyframes float {
  0%, 100% {
    transform: translateY(0) translateX(0);
    opacity: 0.3;
  }
  50% {
    transform: translateY(-40px) translateX(20px);
    opacity: 0.8;
  }
}

.login-container {
  position: relative;
  z-index: 10;
}

.login-card {
  width: 420px;
  padding: 40px;
  background: rgba(19, 47, 76, 0.85);
  backdrop-filter: blur(20px);
  border-radius: 12px;
  border: 1px solid rgba(64, 158, 255, 0.2);
  box-shadow: 0 20px 60px rgba(0, 0, 0, 0.5), 0 0 40px rgba(64, 158, 255, 0.1);
}

.login-header {
  text-align: center;
  margin-bottom: 32px;
}

.login-logo {
  width: 64px;
  height: 64px;
  margin: 0 auto 16px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, #409EFF, #66B1FF);
  border-radius: 16px;
  color: white;
  box-shadow: 0 8px 24px rgba(64, 158, 255, 0.3);
}

.login-title {
  font-size: 26px;
  font-weight: 700;
  color: #e3e8ef;
  margin: 0 0 4px;
  background: linear-gradient(135deg, #409EFF, #66B1FF);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
}

.login-subtitle {
  font-size: 14px;
  color: #78909c;
  margin: 0;
}

.login-form {
  :deep(.el-input__wrapper) {
    background: rgba(10, 25, 41, 0.8) !important;
    border-radius: 8px;
    height: 44px;
    box-shadow: 0 0 0 1px #1e3a5f inset !important;
  }

  :deep(.el-input__wrapper:hover) {
    box-shadow: 0 0 0 1px #409EFF inset !important;
  }

  :deep(.el-input__inner) {
    color: #e3e8ef !important;
  }

  :deep(.el-input__prefix .el-icon) {
    color: #78909c;
  }
}

.login-options {
  display: flex;
  justify-content: space-between;
  align-items: center;
  width: 100%;

  :deep(.el-checkbox__label) {
    color: #78909c;
  }
}

.login-btn {
  width: 100%;
  height: 44px;
  font-size: 16px;
  border-radius: 8px;
  background: linear-gradient(135deg, #409EFF, #3A8EE6);
  border: none;
  letter-spacing: 4px;
  transition: all 0.3s;

  &:hover {
    transform: translateY(-1px);
    box-shadow: 0 6px 20px rgba(64, 158, 255, 0.4);
  }

  &:active {
    transform: translateY(0);
  }
}

.login-footer {
  text-align: center;
  margin-top: 24px;
  font-size: 12px;
  color: #546e7a;
}
</style>
