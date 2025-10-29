<template>
  <div class="forum">
    <!-- 顶部贴吧信息 -->
    <section class="section forum-header">
      <div v-if="loading.forum" class="loading">加载中...</div>
      <div v-else-if="error.forum" class="error">{{ error.forum }}</div>
      <div v-else class="forum-info">
        <h1>{{ forum.name }}</h1>
        <p class="description">{{ forum.description }}</p>
        <div class="meta">
          <span>帖子数: {{ forum.postCount }}</span>
          <button
            v-if="userStore.isLoggedIn"
            class="follow-btn"
            :class="{ followed: isFollowed }"
            @click="toggleFollow"
            :disabled="following"
          >
            {{ following ? '处理中...' : isFollowed ? '取消关注' : '关注' }}
          </button>
        </div>
      </div>
    </section>

    <!-- 帖子列表 -->
    <section class="section posts">
      <h2>帖子列表</h2>
      <div v-if="loading.posts" class="loading">加载中...</div>
      <div v-else-if="error.posts" class="error">{{ error.posts }}</div>
      <div v-else-if="!posts.length" class="empty">暂无帖子</div>
      <ul v-else class="post-list">
        <li v-for="post in posts" :key="post.id" class="post-item">
          <router-link :to="`/forum/${forum.id}/post/${post.id}`">
            <div class="post-header">
              <h3>{{ post.title }}</h3>
              <span class="time">{{ formatTime(post.createdAt) }}</span>
            </div>
            <p class="content">{{ post.content.substring(0, 100) }}...</p>
            <div class="meta">
              <span>作者: {{ post.author }}</span>
              <span>回复: {{ post.replyCount }}</span>
            </div>
          </router-link>
        </li>
      </ul>
      <div v-if="hasMore && !loading.posts" class="load-more">
        <button @click="loadMorePosts" :disabled="loading.more">加载更多</button>
      </div>
    </section>
  </div>
</template>

<script lang="ts" setup>
import { ref, computed, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { useUserStore } from '@/stores/user'
import {
  getForum,
  getForumPosts,
  followForumApi,
  unfollowForumApi,
  getFollowedForums,
} from '@/api/forumApi'
import type { Forum, Post } from '@/types'
import { REPLY_CONSTANTS } from '@/constants/forum'

const route = useRoute()
const userStore = useUserStore()
const forum = ref<Forum>({ id: 0, name: '', description: '', postCount: 0 })
const posts = ref<Post[]>([])
const page = ref(1)
const hasMore = ref(true)
const loading = ref({ forum: true, posts: true, more: false })
const error = ref({ forum: '', posts: '' })
const following = ref(false)
const isFollowed = ref(false)

// 获取贴吧 ID
const forumId = computed(() => Number(route.params.forumId))

// 加载贴吧信息和帖子
onMounted(async () => {
  try {
    loading.value.forum = true
    loading.value.posts = true

    console.log('id:', forumId.value)

    // 并行加载贴吧信息和帖子
    const [forumData, postData] = await Promise.all([
      getForum(forumId.value),
      getForumPosts(forumId.value, page.value),
    ])
    forum.value = forumData
    posts.value = postData
    hasMore.value = postData.length === REPLY_CONSTANTS.POST_LIST_PAGE_SIZE

    // 检查关注状态
    if (userStore.isLoggedIn && userStore.userId) {
      const followedForums = await getFollowedForums(userStore.userId)
      isFollowed.value = followedForums.some((f) => f.id === forumId.value)
    }
  } catch (err) {
    error.value.forum = error.value.posts = (err as Error).message
  } finally {
    loading.value.forum = false
    loading.value.posts = false
  }
})

// 加载更多帖子
const loadMorePosts = async () => {
  if (loading.value.more || !hasMore.value) return
  loading.value.more = true
  try {
    page.value += 1
    const newPosts = await getForumPosts(forumId.value, page.value)
    posts.value = [...posts.value, ...newPosts]
    hasMore.value = newPosts.length === REPLY_CONSTANTS.POST_LIST_PAGE_SIZE
  } catch (err) {
    error.value.posts = (err as Error).message
  } finally {
    loading.value.more = false
  }
}

// 切换关注状态
const toggleFollow = async () => {
  if (!userStore.isLoggedIn || !userStore.userId) {
    error.value.forum = '请先登录以关注贴吧'
    return
  }
  following.value = true
  try {
    if (isFollowed.value) {
      await unfollowForumApi(userStore.userId, forumId.value)
      isFollowed.value = false
    } else {
      await followForumApi(userStore.userId, forumId.value)
      isFollowed.value = true
    }
  } catch (err) {
    error.value.forum = `操作失败：${(err as Error).message}`
  } finally {
    following.value = false
  }
}

// 格式化时间
const formatTime = (timestamp: number) => {
  const date = new Date(timestamp)
  return date.toLocaleString('zh-CN', {
    year: 'numeric',
    month: '2-digit',
    day: '2-digit',
    hour: '2-digit',
    minute: '2-digit',
  })
}
</script>

<style scoped>
.forum {
  max-width: 1200px;
  margin: 0 auto;
  padding: 20px;
  padding-left: 100px;
}

h1 {
  font-size: 2rem;
  color: #333;
  margin-bottom: 10px;
}

.section {
  margin-bottom: 30px;
}

.forum-header {
  background: #fff;
  border: 1px solid #eee;
  border-radius: 6px;
  padding: 20px;
}

.description {
  font-size: 1rem;
  color: #666;
  margin-bottom: 15px;
}

.meta {
  display: flex;
  align-items: center;
  gap: 20px;
  font-size: 0.9rem;
  color: #999;
}

.follow-btn {
  padding: 8px 16px;
  border: none;
  border-radius: 4px;
  background: #4c91d9;
  color: white;
  cursor: pointer;
}

.follow-btn.followed {
  background: #ccc;
}

.follow-btn:hover:not(:disabled) {
  background: #3a7ab8;
}

.follow-btn:disabled {
  background: #ccc;
  cursor: not-allowed;
}

.section h2 {
  font-size: 1.5rem;
  color: #333;
  margin-bottom: 15px;
}

.post-list {
  list-style: none;
  padding: 0;
}

.post-item {
  background: #fff;
  border: 1px solid #eee;
  border-radius: 6px;
  padding: 15px;
  margin-bottom: 10px;
  transition: box-shadow 0.2s ease;
}

.post-item:hover {
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.post-item a {
  text-decoration: none;
  color: #333;
  display: block;
}

.post-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 8px;
}

.post-header h3 {
  font-size: 1.2rem;
  color: #4c91d9;
  margin: 0;
}

.time {
  font-size: 0.85rem;
  color: #999;
}

.content {
  font-size: 0.9rem;
  color: #666;
  margin: 0 0 8px;
}

.meta {
  display: flex;
  gap: 15px;
  font-size: 0.85rem;
  color: #999;
}

.load-more {
  text-align: center;
  margin-top: 20px;
}

.load-more button {
  padding: 8px 16px;
  border: 1px solid #ddd;
  border-radius: 4px;
  background: #fff;
  cursor: pointer;
}

.load-more button:hover:not(:disabled) {
  background: #f0f8ff;
}

.load-more button:disabled {
  background: #ccc;
  cursor: not-allowed;
}

.loading,
.empty,
.error {
  color: #666;
  text-align: center;
  font-size: 1rem;
}

.error {
  color: red;
}

@media (max-width: 576px) {
  .forum {
    padding-left: 20px;
  }
  .meta {
    flex-direction: column;
    align-items: flex-start;
    gap: 10px;
  }
}

@media (prefers-color-scheme: dark) {
  .forum {
    background: #1a1a1a;
  }
  h1,
  .section h2 {
    color: #ddd;
  }
  .forum-header,
  .post-item {
    background: #222;
    border-color: #444;
  }
  .post-item a {
    color: #ddd;
  }
  .post-header h3 {
    color: #6ab0ff;
  }
  .description,
  .content {
    color: #aaa;
  }
  .meta,
  .time,
  .loading,
  .empty {
    color: #aaa;
  }
  .follow-btn {
    background: #6ab0ff;
  }
  .follow-btn.followed {
    background: #666;
  }
  .follow-btn:hover:not(:disabled) {
    background: #5a9be6;
  }
  .load-more button {
    background: #222;
    border-color: #444;
    color: #ddd;
  }
  .load-more button:hover:not(:disabled) {
    background: #2a3a4a;
  }
  .error {
    color: #ff6347;
  }
}
</style>
