import { defineStore } from 'pinia'
import { login as loginApi, refreshToken as refreshTokenApi, type LoginParams, type UserInfo } from '@/api/auth'
import router from '@/router'

interface AuthState {
  token: string
  refreshToken: string
  user: UserInfo | null
  isAuthenticated: boolean
}

export const useAuthStore = defineStore('auth', {
  state: (): AuthState => ({
    token: localStorage.getItem('token') || '',
    refreshToken: localStorage.getItem('refreshToken') || '',
    user: JSON.parse(localStorage.getItem('user') || 'null'),
    isAuthenticated: !!localStorage.getItem('token'),
  }),

  getters: {
    userRole: (state) => state.user?.role || 'viewer',
    isAdmin: (state) => state.user?.role === 'admin',
    isOperator: (state) => state.user?.role === 'operator' || state.user?.role === 'admin',
  },

  actions: {
    async login(params: LoginParams) {
      const result = await loginApi(params)
      this.token = result.token
      this.refreshToken = result.refreshToken
      this.user = result.user
      this.isAuthenticated = true
      localStorage.setItem('token', result.token)
      localStorage.setItem('refreshToken', result.refreshToken)
      localStorage.setItem('user', JSON.stringify(result.user))
    },

    logout() {
      this.token = ''
      this.refreshToken = ''
      this.user = null
      this.isAuthenticated = false
      localStorage.removeItem('token')
      localStorage.removeItem('refreshToken')
      localStorage.removeItem('user')
      router.push('/login')
    },

    async refreshTokenAction() {
      if (!this.refreshToken) return
      try {
        const result = await refreshTokenApi(this.refreshToken)
        this.token = result.token
        this.refreshToken = result.refreshToken
        this.user = result.user
        this.isAuthenticated = true
        localStorage.setItem('token', result.token)
        localStorage.setItem('refreshToken', result.refreshToken)
        localStorage.setItem('user', JSON.stringify(result.user))
      } catch {
        this.logout()
      }
    },

    checkAuth() {
      this.isAuthenticated = !!this.token
      if (!this.isAuthenticated) {
        router.push('/login')
      }
    },
  },
})
