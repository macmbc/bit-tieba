// src/stores/authStore.ts
import { defineStore } from 'pinia'
import { ref } from 'vue'
import type { LoginResponse } from '@/api/authApi'

export interface UserSession {
  uid: number
  email: string
  token: string
  host: string
  port: string
}

export const useAuthStore = defineStore('auth', () => {
  const user = ref<UserSession | null>(null)
  const isLoggedIn = ref(false)

  // 从 localStorage 恢复登录状态
  const initAuth = () => {
    const savedUser = localStorage.getItem('user_session')
    if (savedUser) {
      try {
        const userData = JSON.parse(savedUser)
        user.value = userData
        isLoggedIn.value = true
      } catch (error) {
        console.error('解析用户数据失败:', error)
        logout()
      }
    }
  }

  // 登录成功后保存用户信息
  const login = (loginData: LoginResponse) => {
    const session: UserSession = {
      uid: loginData.uid,
      email: loginData.email,
      token: loginData.token,
      host: loginData.host,
      port: loginData.port,
    }
    user.value = session
    isLoggedIn.value = true
    localStorage.setItem('user_session', JSON.stringify(session))
  }

  // 退出登录
  const logout = () => {
    user.value = null
    isLoggedIn.value = false
    localStorage.removeItem('user_session')
  }

  // 更新用户信息
  const updateUser = (updates: Partial<UserSession>) => {
    if (user.value) {
      user.value = { ...user.value, ...updates }
      localStorage.setItem('user_session', JSON.stringify(user.value))
    }
  }

  return {
    user,
    isLoggedIn,
    initAuth,
    login,
    logout,
    updateUser,
  }
})
