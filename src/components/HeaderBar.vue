<template>
  <header class="header" role="banner">
    <div class="header-inner">
      <div class="left">
        <router-link to="/home" class="logo" aria-label="返回首页">
          🗨️ <span class="brand">BIT-Tieba</span>
        </router-link>
        <nav class="nav" aria-label="主导航">
          <router-link to="/home" class="nav-link">首页</router-link>
          <router-link to="/forums" class="nav-link">贴吧</router-link>
        </nav>
      </div>

      <div class="right">
        <div class="search-wrap">
          <SearchBar />
        </div>
        <router-link to="/message" class="icon-btn" aria-label="消息">
          <span class="icon" aria-hidden="true">🔔</span>
          <span v-if="unreadCount" class="badge">{{ unreadText }}</span>
        </router-link>
        <router-link to="/me" class="avatar" aria-label="个人中心">
          <span class="avatar-initials">我</span>
        </router-link>
      </div>
    </div>
  </header>
</template>

<script lang="ts" setup>
import { computed } from 'vue'
import { useMessageStore } from '@/stores/user'
import SearchBar from '@/components/SearchBar.vue'

const messageStore = useMessageStore()
const unreadCount = computed(() => messageStore.unreadCount)
const unreadText = computed(() => (unreadCount.value > 99 ? '99+' : unreadCount.value))
</script>

<style scoped>
.header {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  height: 64px;
  background: var(--color-card-bg);
  border-bottom: 1px solid var(--color-border);
  box-shadow: var(--shadow-sm);
  z-index: 1000;
  backdrop-filter: blur(8px);
}

.header-inner {
  max-width: 1200px;
  margin: 0 auto;
  display: flex;
  align-items: center;
  justify-content: space-between;
  height: 100%;
  padding: 0 var(--sp-5);
}

.left {
  display: flex;
  align-items: center;
  gap: var(--sp-6);
}

.logo {
  display: inline-flex;
  align-items: center;
  gap: var(--sp-2);
  color: var(--color-text-1);
  text-decoration: none;
  font-weight: var(--fw-bold);
}

.brand {
  font-size: 1.25rem;
  font-weight: var(--fw-bold);
}

.nav {
  display: flex;
  gap: var(--sp-5);
}

.nav-link {
  color: var(--color-text-2);
  text-decoration: none;
  font-size: var(--fz-body);
  padding: var(--sp-2) var(--sp-1);
  border-radius: var(--radius-sm);
  transition: color var(--ease-fast) ease, background-color var(--ease-fast) ease;
}

.nav-link:hover,
.router-link-active.nav-link {
  color: var(--color-brand);
  background: color-mix(in srgb, var(--color-brand) 12%, transparent);
}

.right {
  display: flex;
  align-items: center;
  gap: var(--sp-4);
}

.search-wrap :deep(.search-bar) {
  margin: 0;
  width: 320px;
}

.icon-btn {
  position: relative;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 36px;
  height: 36px;
  border-radius: 50%;
  text-decoration: none;
  color: var(--color-text-1);
  transition: background-color var(--ease-fast) ease;
}

.icon-btn:hover {
  background: color-mix(in srgb, var(--color-brand) 12%, transparent);
}

.badge {
  position: absolute;
  top: -2px;
  right: -2px;
  min-width: 18px;
  height: 18px;
  padding: 0 4px;
  border-radius: 9px;
  background: var(--color-danger);
  color: #fff;
  font-size: 11px;
  line-height: 18px;
  text-align: center;
}

.avatar {
  width: 36px;
  height: 36px;
  border-radius: 50%;
  background: var(--color-background);
  border: 1px solid var(--color-border);
  display: inline-flex;
  align-items: center;
  justify-content: center;
  color: var(--color-text-2);
  text-decoration: none;
  transition: box-shadow var(--ease-fast) ease;
}

.avatar:hover {
  box-shadow: var(--shadow-sm);
}

.avatar-initials {
  font-size: 14px;
}

@media (max-width: 768px) {
  .header-inner {
    padding: 0 var(--sp-3);
  }
  .nav {
    display: none;
  }
  .search-wrap :deep(.search-bar) {
    width: 220px;
  }
}
</style>
