import axios, { AxiosInstance, AxiosRequestConfig, InternalAxiosRequestConfig, AxiosResponse } from 'axios'
import { ElMessage } from 'element-plus'
import router from '@/router'

// Standard API response type
export interface ApiResponse<T = any> {
  code: number
  message: string
  data: T
}

// Create axios instance
const service: AxiosInstance = axios.create({
  baseURL: '/api/v1',
  timeout: 30000,
  headers: {
    'Content-Type': 'application/json',
  },
})

// Request interceptor
service.interceptors.request.use(
  (config: InternalAxiosRequestConfig) => {
    const token = localStorage.getItem('token')
    if (token && config.headers) {
      config.headers.Authorization = `Bearer ${token}`
    }
    return config
  },
  (error) => {
    return Promise.reject(error)
  }
)

// Response interceptor
service.interceptors.response.use(
  (response: AxiosResponse<ApiResponse>) => {
    const res = response.data
    if (res.code !== 0) {
      ElMessage.error(res.message || '请求失败')
      if (res.code === 401) {
        handleUnauthorized()
      }
      return Promise.reject(new Error(res.message || '请求失败'))
    }
    return response
  },
  (error) => {
    if (error.response) {
      const { status, data } = error.response
      switch (status) {
        case 401:
          handleUnauthorized()
          break
        case 403:
          ElMessage.error('没有权限执行此操作')
          break
        case 404:
          ElMessage.error('请求的资源不存在')
          break
        case 500:
          ElMessage.error(data?.message || '服务器内部错误')
          break
        default:
          ElMessage.error(data?.message || `请求失败 (${status})`)
      }
    } else if (error.code === 'ECONNABORTED') {
      ElMessage.error('请求超时，请稍后重试')
    } else {
      ElMessage.error('网络连接异常，请检查网络')
    }
    return Promise.reject(error)
  }
)

// Handle 401 - try refresh token, then redirect to login
let isRefreshing = false
let failedQueue: Array<{ resolve: (value: unknown) => void; reject: (reason?: unknown) => void }> = []

function handleUnauthorized() {
  const refreshToken = localStorage.getItem('refreshToken')
  if (!refreshToken || isRefreshing) {
    if (!isRefreshing) {
      clearAuthAndRedirect()
    }
    return
  }

  isRefreshing = true
  axios
    .post('/api/v1/auth/refresh', { refreshToken })
    .then((res) => {
      const { token: newToken, refreshToken: newRefreshToken } = res.data.data
      localStorage.setItem('token', newToken)
      if (newRefreshToken) localStorage.setItem('refreshToken', newRefreshToken)
      failedQueue.forEach((promise) => promise.resolve(newToken))
      failedQueue = []
    })
    .catch(() => {
      clearAuthAndRedirect()
    })
    .finally(() => {
      isRefreshing = false
    })
}

function clearAuthAndRedirect() {
  localStorage.removeItem('token')
  localStorage.removeItem('refreshToken')
  localStorage.removeItem('user')
  router.push('/login')
}

// Helper methods
export function get<T = any>(url: string, params?: Record<string, any>, config?: AxiosRequestConfig): Promise<T> {
  return service.get(url, { params, ...config }).then((res) => res.data?.data as T)
}

export function post<T = any>(url: string, data?: Record<string, any>, params?: Record<string, any>, config?: AxiosRequestConfig): Promise<T> {
  return service.post(url, data, { params, ...config }).then((res) => res.data?.data as T)
}

export function put<T = any>(url: string, data?: Record<string, any>, params?: Record<string, any>, config?: AxiosRequestConfig): Promise<T> {
  return service.put(url, data, { params, ...config }).then((res) => res.data?.data as T)
}

export function del<T = any>(url: string, params?: Record<string, any>, config?: AxiosRequestConfig): Promise<T> {
  return service.delete(url, { params, ...config }).then((res) => res.data?.data as T)
}

export default service
