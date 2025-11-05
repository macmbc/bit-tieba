<template>
  <div class="tieba">
    <!-- 标题 -->
    <h1>我关注的吧</h1>

    <!-- 关注的吧列表 -->
    <section class="section">
      <h2>已关注</h2>
      <div v-if="loading">加载中...</div>
      <div v-else-if="error" class="error">加载失败：{{ error }}</div>
      <div v-else-if="!followedForums.length" class="empty">你还没有关注任何吧！</div>
      <ul v-else class="forum-list">
        <li v-for="forum in followedForums" :key="forum.id" class="forum-item">
          <router-link :to="`/forum/${forum.id}`" class="forum-link">
            <h3>{{ forum.name }}</h3>
            <p>{{ forum.description }}</p>
            <span>帖子数: {{ forum.postCount }}</span>
          </router-link>
          <button
            class="unfollow-btn"
            @click="unfollowForum(forum.id)"
            :disabled="unfollowing[forum.id]"
          >
            {{ unfollowing[forum.id] ? '取消中...' : '取消关注' }}
          </button>
        </li>
      </ul>
    </section>
  </div>
</template>

<script lang="ts" setup>
import { ref, onMounted } from 'vue'
import { useUserStore } from '@/stores/user'
import { getFollowedForums, unfollowForumApi } from '@/api/forumApi'
import type { Forum } from '@/types'

const userStore = useUserStore()
const followedForums = ref<Forum[]>([])
const loading = ref(true)
const error = ref('')
const unfollowing = ref<Record<number, boolean>>({})

onMounted(async () => {
  if (!userStore.isLoggedIn) {
    error.value = '请先登录以查看关注的吧'
    loading.value = false
    return
  }

  try {
    loading.value = true
    followedForums.value = await getFollowedForums(userStore.userId)
  } catch (err) {
    error.value = (err as Error).message
  } finally {
    loading.value = false
  }
})

// 取消关注
const unfollowForum = async (forumId: number) => {
  if (!userStore.isLoggedIn || !userStore.userId) return

  try {
    unfollowing.value[forumId] = true
    await unfollowForumApi(userStore.userId, forumId)
    followedForums.value = followedForums.value.filter((f) => f.id !== forumId)
  } catch (err) {
    error.value = `取消关注失败：${(err as Error).message}`
  } finally {
    unfollowing.value[forumId] = false
  }
}
</script>

<style scoped>
.tieba {
  max-width: 1200px;
  margin: 0 auto;
  padding: 20px;
  padding-left: 100px; /* 适配侧边栏宽度 (80px + 20px 间距) */
}

h1 {
  font-size: 2rem;
  color: #333;
  margin-bottom: 20px;
}

.section {
  margin-bottom: 30px;
}

.section h2 {
  font-size: 1.5rem;
  color: #333;
  margin-bottom: 15px;
}

.forum-list {
  list-style: none;
  padding: 0;
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(250px, 1fr));
  gap: 15px;
}

.forum-item {
  background: #fff;
  border: 1px solid #eee;
  border-radius: 6px;
  padding: 15px;
  display: flex;
  flex-direction: column;
  transition: box-shadow 0.2s ease;
}

.forum-item:hover {
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.forum-link {
  text-decoration: none;
  color: #333;
  flex: 1;
}

.forum-link h3 {
  font-size: 1.2rem;
  margin: 0 0 10px;
  color: #4c91d9; /* 贴吧蓝色 */
}

.forum-link p {
  font-size: 0.9rem;
  color: #666;
  margin: 0 0 10px;
}

.forum-link span {
  font-size: 0.85rem;
  color: #999;
}

.unfollow-btn {
  background: #ff4500; /* 贴吧红色 */
  color: white;
  border: none;
  border-radius: 4px;
  padding: 8px;
  font-size: 0.9rem;
  cursor: pointer;
  margin-top: 10px;
}

.unfollow-btn:hover:not(:disabled) {
  background: #e63900;
}

.unfollow-btn:disabled {
  background: #ccc;
  cursor: not-allowed;
}

.empty {
  color: #666;
  text-align: center;
  font-size: 1rem;
}

.error {
  color: red;
  text-align: center;
  font-size: 1rem;
}

/* 响应式 */
@media (max-width: 576px) {
  .tieba {
    padding-left: 20px; /* 移动端移除侧边栏偏移 */
  }
  .forum-list {
    grid-template-columns: 1fr;
  }
}

/* 暗模式 */
@media (prefers-color-scheme: dark) {
  .tieba {
    background: #1a1a1a;
  }
  h1,
  .section h2 {
    color: #ddd;
  }
  .forum-item {
    background: #222;
    border-color: #444;
  }
  .forum-link {
    color: #ddd;
  }
  .forum-link h3 {
    color: #6ab0ff;
  }
  .forum-link p {
    color: #aaa;
  }
  .forum-link span,
  .empty {
    color: #aaa;
  }
  .unfollow-btn {
    background: #ff6347;
  }
  .unfollow-btn:hover:not(:disabled) {
    background: #e55337;
  }
}
</style>
