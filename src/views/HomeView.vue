<template>
  <div class="home">
    <SearchBar />
    <HotForums :forums="hotForums" />
    <HotPosts :posts="hotPosts" />
    <FollowedPosts v-if="isLoggedIn" :posts="followedPosts" />
  </div>
</template>

<script lang="ts" setup>
import { ref, onMounted } from 'vue'
import { useUserStore } from '@/stores/user'
import { getHotForums, getHotPosts, getFollowedPosts } from '@/api/forumApi'
import type { Forum, Post } from '@/types'

import SearchBar from '@/components/SearchBar.vue'
import HotForums from '@/components/HotForums.vue'
import HotPosts from '@/components/HotPosts.vue'
import FollowedPosts from '@/components/FollowedPosts.vue'

// 模拟用户登录状态
const userStore = useUserStore()
const isLoggedIn = ref(userStore.isLoggedIn)

// 使用类型注解
const hotForums = ref<Forum[]>([])
const hotPosts = ref<Post[]>([])
const followedPosts = ref<Post[]>([])

// 数据加载
onMounted(async () => {
  try {
    const [forums, posts, followed] = await Promise.all([
      getHotForums(),
      getHotPosts(),
      isLoggedIn.value ? getFollowedPosts(userStore.userInfo?.id) : Promise.resolve([]),
    ])
    hotForums.value = forums
    hotPosts.value = posts
    followedPosts.value = followed
  } catch (error) {
    console.error('Failed to load data:', error)
  }

  console.log(followedPosts.value)
})
</script>

<style scoped>
.home {
  max-width: 1200px;
  margin: 0 auto;
  padding: 20px;
  padding-left: 100px; /* 适配侧边栏宽度 (80px + 20px 间距) */
}

/* 响应式 */
@media (max-width: 576px) {
  .home {
    padding-left: 20px; /* 移动端移除侧边栏偏移 */
  }
}

/* 暗模式 */
@media (prefers-color-scheme: dark) {
  .home {
    background: #1a1a1a;
  }
}
</style>
