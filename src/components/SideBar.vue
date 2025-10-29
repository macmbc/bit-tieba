<template>
  <div class="sidebar">
    <ul>
      <li v-for="item in menuItems" :key="item.label">
        <router-link :to="item.path" class="menu-link">
          <span class="icon">{{ item.icon }}</span>
          <span class="label">{{ item.label }}</span>
          <!-- 小红点显示未读消息数 -->
          <span v-if="item.path == '/message' && unreadCount" class="badge">
            {{ unreadCount > 99 ? '99+' : unreadCount }}
          </span>
        </router-link>
      </li>
    </ul>
  </div>
</template>

<script lang="ts" setup>
import { reactive, computed } from 'vue'
import { useMessageStore } from '@/stores/user'

const menuItems = reactive([
  { label: '首页', path: '/home', icon: '🏠' },
  { label: '贴吧', path: '/forums', icon: '🔳' },
  { label: '消息', path: '/message', icon: '✉️' },
  { label: '我的', path: '/me', icon: '👤' },
  { label: '调试', path: '/debug', icon: '🧩' },
])

// 模拟未读消息数
const messageStore = useMessageStore()
const unreadCount = computed(() => messageStore.unreadCount)
</script>

<style scoped>
.sidebar {
  width: 80px;
  height: 100vh;
  background-color: #fff;
  border-right: 1px solid #eee;
  display: flex;
  flex-direction: column;
  align-items: center;
  padding-top: 20px;
}

ul {
  list-style: none;
  padding: 0;
  width: 100%;
}

li {
  width: 100%;
  text-align: center;
  margin: 20px 0;
}

.menu-link {
  display: flex;
  flex-direction: column;
  align-items: center;
  color: #666;
  text-decoration: none;
  font-size: 0.9rem;
  position: relative; /* 为小红点定位 */
}

.menu-link:hover {
  color: #42b983;
  background-color: rgba(66, 185, 131, 0.1);
  border-radius: 6px;
}

.icon {
  font-size: 1.4rem;
  margin-bottom: 5px;
}

.label {
  font-size: 0.9rem;
}

.badge {
  position: absolute;
  top: 0;
  right: 10px;
  background: #ff4500; /* 贴吧红色 */
  color: white;
  border-radius: 10px;
  padding: 2px 6px;
  font-size: 0.7rem;
  line-height: 1;
}

.router-link-active {
  color: #42b983;
  font-weight: 600;
}
</style>
