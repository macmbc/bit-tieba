<template>
  <div class="app-root">
    <router-view v-if="!userStore.isLoggedIn" />
    <div v-else class="app-shell">
      <HeaderBar class="app-header" />
      <main class="main-content">
        <router-view />
      </main>
    </div>
  </div>
</template>

<script lang="ts" setup>
import { onMounted } from 'vue'
import { useUserStore } from '@/stores/user'
import { useMessageStore } from '@/stores/user'
import HeaderBar from '@/components/HeaderBar.vue'

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
.app-root {
  min-height: 100vh;
  background: var(--color-background);
}

.app-shell {
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  background: var(--color-background);
}

.app-header {
  flex-shrink: 0;
}

.main-content {
  flex: 1;
  width: 100%;
  max-width: var(--layout-heart-width);
  margin: 0 auto;
  padding: calc(64px + var(--sp-5)) var(--sp-5) var(--sp-5);
  box-sizing: border-box;
  background: var(--color-background);
}

@media (max-width: 768px) {
  .main-content {
    padding: calc(64px + var(--sp-4)) var(--sp-3) var(--sp-4);
  }
}
</style>
