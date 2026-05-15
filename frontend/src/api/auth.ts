import { get, post, put } from './request'

export interface LoginParams {
  username: string
  password: string
}

export interface LoginResult {
  token: string
  refreshToken: string
  user: UserInfo
}

export interface UserInfo {
  id: number
  username: string
  role: 'admin' | 'operator' | 'viewer'
}

export interface RegisterParams {
  username: string
  password: string
  role?: string
}

export function login(data: LoginParams) {
  return post<LoginResult>('/auth/login', data)
}

export function register(data: RegisterParams) {
  return post<LoginResult>('/auth/register', data)
}

export function refreshToken(refreshToken: string) {
  return post<LoginResult>('/auth/refresh', { refreshToken })
}

export function changePassword(data: { oldPassword: string; newPassword: string }) {
  return put('/auth/password', data)
}

export function getUsers() {
  return get<UserInfo[]>('/auth/users')
}

export function getUser(id: number) {
  return get<UserInfo>(`/auth/users/${id}`)
}

export function updateUser(id: number, data: Partial<UserInfo>) {
  return put<UserInfo>(`/auth/users/${id}`, data)
}

export function deleteUser(id: number) {
  return put(`/auth/users/${id}`, { deleted: true })
}
