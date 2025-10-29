import { defineStore } from 'pinia'
import { getUnreadCount } from '@/api/forumApi'

const autoLogin = import.meta.env.VITE_AUTO_LOGIN === 'true'
const testUserId = import.meta.env.VITE_TEST_USER_ID || undefined

export const useUserStore = defineStore('user', {
  state: () => ({
    isLoggedIn: autoLogin,
    userId: autoLogin ? testUserId : undefined,
  }),
  actions: {
    login(userId: string) {
      this.isLoggedIn = true
      this.userId = userId
    },
    logout() {
      this.isLoggedIn = false
      this.userId = undefined
    },
  },
})

export const useMessageStore = defineStore('message', {
  state: () => ({
    unreadCount: 0,
  }),
  actions: {
    async refreshUnreadCount(userId?: string) {
      const count = await getUnreadCount(userId)
      this.unreadCount = count
    },
    setUnreadCount(count: number) {
      this.unreadCount = count
    },
  },
})
