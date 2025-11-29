<template>
  <div class="home two-col-page">
    <div class="layout-grid">
      <section class="col feed-col" aria-label="推荐帖子">
        <HotPosts :posts="hotPosts" />
      </section>
      <aside class="col aside-col" aria-label="热门与关注">
        <HotForums :forums="hotForums" />
        <FollowedPosts v-if="isLoggedIn" :posts="followedPosts" />
      </aside>
    </div>
  </div>
</template>

<script lang="ts" setup>
import { ref, onMounted } from 'vue'
import { useUserStore } from '@/stores/user'
import { getHotForums, getHotPosts, getFollowedPosts } from '@/api/forumApi'
import type { Forum, Post } from '@/types'

import HotPosts from '@/components/HotPosts.vue'
import FollowedPosts from '@/components/FollowedPosts.vue'
import HotForums from '@/components/HotForums.vue'

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
      isLoggedIn.value ? getFollowedPosts(userStore.userId) : Promise.resolve([]),
    ])
    hotForums.value = forums
    hotPosts.value = posts
    followedPosts.value = followed
  } catch (error) {
    console.error('Failed to load data:', error)
  }

  // TODO: 接入骨架屏或空状态提示
})
</script>

<style scoped>
.home {
  max-width: var(--layout-heart-width);
  margin: 0 auto;
  padding: 0;
}

.layout-grid {
  display: grid;
  grid-template-columns: minmax(0, 7fr) minmax(280px, 4fr);
  gap: var(--sp-6);
  padding-top: var(--sp-4);
}

.col {
  display: flex;
  flex-direction: column;
  gap: var(--sp-5);
}

.aside-col {
  position: sticky;
  top: calc(64px + var(--sp-4));
  align-self: flex-start;
}

@media (max-width: 1024px) {
  .layout-grid {
    grid-template-columns: 1fr;
  }

  .aside-col {
    position: static;
  }
}
</style>
