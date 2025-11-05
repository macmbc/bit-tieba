<template>
  <div>
    <!-- 如果用户未登录，显示登录/注册页面 -->
    <router-view v-if="!userStore.isLoggedIn" />
    <!-- 如果用户已登录，显示主布局 -->
    <div class="app-container">
      <Sidebar class="sidebar" />
      <div class="main-content">
        <router-view />
      </div>
    </div>
  </div>
</template>

<script lang="ts" setup>
import Sidebar from '@/components/SideBar.vue'
import { onMounted } from 'vue'
import { useUserStore } from '@/stores/user'
import { useMessageStore } from '@/stores/user'

const userStore = useUserStore()
const messageStore = useMessageStore()

onMounted(() => {
  if (userStore.userId) {
    // 页面加载时刷新一次未读消息数
    messageStore.refreshUnreadCount(userStore.userId)
  }
})
</script>

<style scoped>
.app-container {
  display: flex;
  flex-direction: row;
  width: 100vw;
  height: 100vh;
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}

.sidebar {
  flex-shrink: 0;
  color: #fff;
  height: 100vh;
  overflow-y: auto;
}

.main-content {
  flex: 1;
  overflow-y: auto;
  background-color: #f8f9fa;
  height: 100vh;
  box-sizing: border-box;
  padding: 0;
  margin: 0;
}
</style>
