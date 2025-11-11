import { defineStore } from 'pinia'
import { ref } from 'vue'
import { getUnreadCount } from '@/api/forumApi'
import type { UserInfo } from '@/types'

// 环境变量
const autoLogin = import.meta.env.VITE_AUTO_LOGIN === 'true'
const testToken = import.meta.env.VITE_TEST_TOKEN || undefined

const testUserInfo: UserInfo = {
  id: 1,
  username: '测试用户',
  email: 'test@example.com',
  desc: '自动登录测试',
  sex: 1,
  avatar: undefined,
}

/**
 * ✅ 用户 Store（组合式写法）
 */
export const useUserStore = defineStore('user', () => {
  const isLoggedIn = ref(autoLogin)
  const userInfo = ref<UserInfo | null>(autoLogin ? testUserInfo : null)
  const token = ref<string | undefined>(autoLogin ? testToken : undefined)

  const login = (info: UserInfo, newToken: string) => {
    userInfo.value = info
    token.value = newToken
    isLoggedIn.value = true
  }

  const logout = () => {
    userInfo.value = null
    token.value = undefined
    isLoggedIn.value = false
  }

  return {
    isLoggedIn,
    userInfo,
    token,
    login,
    logout,
  }
})

/**
 * ✅ 消息 Store（组合式写法）
 */
export const useMessageStore = defineStore('message', () => {
  const unreadCount = ref(0)

  const refreshUnreadCount = async (userId?: number) => {
    const count = await getUnreadCount(userId)
    unreadCount.value = count
  }

  const setUnreadCount = (count: number) => {
    unreadCount.value = count
  }

  return {
    unreadCount,
    refreshUnreadCount,
    setUnreadCount,
  }
})
