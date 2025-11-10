<template>
  <div class="forum">
    <!-- 顶部贴吧信息 -->
    <section class="section forum-header">
      <div v-if="loading.forum" class="loading">加载中...</div>
      <div v-else-if="error.forum" class="error">{{ error.forum }}</div>
      <div v-else class="forum-info">
        <!-- 新增：贴吧头像 -->
        <div class="forum-avatar-wrapper">
          <img :src="forum.avatar || defaultForumAvatar" alt="贴吧头像" class="forum-avatar" />
        </div>

        <!-- 贴吧名称和描述 -->
        <div class="forum-text">
          <h1>{{ forum.name }}</h1>
          <p class="description">{{ forum.description }}</p>

          <!-- 新增：吧主信息 -->
          <p class="owner">
            吧主：
            <router-link v-if="forum.ownerId" :to="`/user/${forum.ownerId}`" class="owner-link">
              {{ forum.ownerName || '未知' }}
            </router-link>
            <span v-else>暂无吧主</span>
          </p>

          <!-- 原有元信息 -->
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
      </div>
    </section>

    <!-- 帖子列表 -->
    <section class="section posts">
      <div class="posts-header">
        <h2>帖子列表</h2>
        <select v-model="sortType" @change="loadPosts(true)" class="sort-select">
          <option value="newest">最新发布</option>
          <option value="reply">最新回复</option>
        </select>
        <button class="featured-btn" :class="{ active: showFeatured }" @click="toggleFeatured">
          {{ showFeatured ? '显示全部' : '只看精华' }}
        </button>
      </div>
      <div v-if="loading.posts" class="loading">加载中...</div>
      <div v-else-if="error.posts" class="error">{{ error.posts }}</div>
      <div v-else-if="!posts.length" class="empty">暂无帖子</div>
      <ul v-else class="post-list">
        <li v-for="post in posts" :key="post.id" class="post-item">
          <div class="post-content">
            <router-link :to="`/forum/${forum.id}/post/${post.id}`" class="post-link">
              <div class="post-header">
                <img
                  :src="post.authorAvatar || defaultUserAvatar"
                  alt="楼主头像"
                  class="author-avatar"
                />
                <div class="post-header-text">
                  <h3>
                    <span v-if="post.isFeatured" class="featured-tag">精</span>
                    {{ post.title }}
                  </h3>
                  <span class="time">{{ formatTime(post.lastRepliedAt) }}</span>
                </div>
              </div>
              <p class="content">{{ post.content.substring(0, 100) }}...</p>
            </router-link>

            <!-- meta 区块放在 router-link 外 -->
            <div class="meta">
              <span>作者: {{ post.author }}</span>
              <span>回复: {{ post.replyCount }}</span>

              <div class="like" @click="toggleLike(post)">
                <i :class="post.isLiked ? 'liked-icon' : 'unliked-icon'"></i>
                <span>{{ post.likeCount }}</span>
              </div>
            </div>
          </div>
        </li>
      </ul>
      <div v-if="hasMore && !loading.posts" class="load-more">
        <button @click="loadMorePosts" :disabled="loading.more">加载更多</button>
      </div>
    </section>

    <PostDialog
      :show="showPostDialog"
      :forumId="forumId"
      @close="showPostDialog = false"
      @postCreated="handlePostCreated"
    />

    <div class="floating-buttons">
      <FloatingButton
        icon="fa-solid fa-arrow-rotate-right"
        tooltip="刷新"
        @click="loadPosts(true)"
      />
      <FloatingButton icon="fa-solid fa-plus" tooltip="发帖" @click="openPostDialog" />
    </div>
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
import defaultForumAvatar from '@/assets/img/defaultForumAvatar.png'
import { REPLY_CONSTANTS } from '@/constants/forum'

import defaultUserAvatar from '@/assets/img/defaultUserAvatar.png'
import FloatingButton from '@/components/FloatingButton.vue'
import PostDialog from '@/components/PostDialog.vue'

import { formatTime } from '@/utils/format'

const route = useRoute()
const userStore = useUserStore()
const forum = ref<Forum>({
  id: 0,
  name: '',
  description: '',
  postCount: 0,
  avatar: '', // 新增：贴吧头像
  ownerId: 0, // 新增：吧主ID
  ownerName: '', // 新增：吧主名字
})
const sortType = ref<'newest' | 'reply'>('reply')

const posts = ref<Post[]>([])
const page = ref(1)
const hasMore = ref(true)
const loading = ref({ forum: true, posts: true, more: false })
const error = ref({ forum: '', posts: '' })
const following = ref(false)
const isFollowed = ref(false)
const showFeatured = ref(false)
const showPostDialog = ref(false)
// 获取贴吧 ID
const forumId = computed(() => Number(route.params.forumId))

// 加载贴吧信息和帖子
onMounted(async () => {
  try {
    loading.value.forum = true
    loading.value.posts = true

    // 并行加载贴吧信息和帖子
    const forumData = await getForum(forumId.value)
    forum.value = forumData
    await loadPosts(true)

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

const toggleFeatured = () => {
  showFeatured.value = !showFeatured.value
  loadPosts(true)
}

const loadPosts = async (reset = false) => {
  try {
    if (reset) {
      page.value = 1
      posts.value = []
    }
    loading.value.posts = true
    let postData = await getForumPosts(forumId.value, page.value, sortType.value)
    if (showFeatured.value) {
      postData = postData.filter((p) => p.isFeatured)
    }
    posts.value = reset ? postData : [...posts.value, ...postData]

    hasMore.value = postData.length === REPLY_CONSTANTS.POST_LIST_PAGE_SIZE
  } catch (err) {
    error.value.posts = (err as Error).message
  } finally {
    loading.value.posts = false
  }
}

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

// 点赞切换逻辑
const toggleLike = (post: Post) => {
  if (post.isLiked) {
    post.likeCount--
  } else {
    post.likeCount++
  }
  post.isLiked = !post.isLiked

  // 可选：向后端发送请求
  // await api.post(`/posts/${post.id}/like`, { liked: post.liked })
}

// 新增：打开发帖弹窗
const openPostDialog = () => {
  if (!userStore.isLoggedIn) {
    error.value.forum = '请先登录以发布帖子'
    return
  }
  showPostDialog.value = true
}

// 新增：处理帖子创建成功
const handlePostCreated = () => {
  loadPosts(true) // 刷新帖子列表
  forum.value.postCount += 1 // 更新帖子计数
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
  float: right;
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

.forum-info {
  display: flex;
  align-items: flex-start;
  gap: 20px;
}

.forum-avatar-wrapper {
  flex-shrink: 0;
}

.forum-avatar {
  width: 100px;
  height: 100px;
  border-radius: 8px;
  object-fit: cover;
  border: 1px solid #eee;
  background-color: #fafafa;
}

.forum-text {
  flex: 1;
}

.owner {
  font-size: 0.95rem;
  color: #555;
  margin-bottom: 10px;
}

.owner-link {
  color: #4c91d9;
  text-decoration: none;
}

.owner-link:hover {
  text-decoration: underline;
}

@media (prefers-color-scheme: dark) {
  .forum-avatar {
    border-color: #444;
    background-color: #222;
  }
  .owner {
    color: #aaa;
  }
  .owner-link {
    color: #6ab0ff;
  }
}

.posts-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 10px;
}

.sort-select {
  padding: 6px 10px;
  border: 1px solid #ccc;
  border-radius: 4px;
  background: #fff;
  font-size: 0.9rem;
  cursor: pointer;
}

@media (prefers-color-scheme: dark) {
  .sort-select {
    background: #222;
    border-color: #444;
    color: #ddd;
  }
}

.author-avatar {
  width: 40px;
  height: 40px;
  border-radius: 50%;
  margin-right: 10px;
  object-fit: cover;
  border: 1px solid #ddd;
}

.post-header {
  display: flex;
  align-items: center;
  margin-bottom: 6px;
}

.post-header-text h3 {
  font-size: 1.1rem;
  margin: 0;
}

.post-header-text .time {
  color: #888;
  font-size: 0.85rem;
}

.author-avatar {
  width: 36px;
  height: 36px;
  border-radius: 50%;
  margin-right: 8px;
}

.like {
  display: inline-flex;
  align-items: center;
  gap: 4px;
  cursor: pointer;
  transition: transform 0.2s;
}

.like:hover {
  transform: scale(1.1);
}

.unliked-icon::before {
  content: '♡'; /* 未点赞 */
  color: #888;
  font-size: 18px;
}

.liked-icon::before {
  font-style: normal;
  content: '❤️'; /* 已点赞 */
  color: #e74c3c;
  font-size: 18px;
}

.post-filters {
  display: flex;
  align-items: center;
  gap: 10px;
}

.featured-btn {
  padding: 6px 12px;
  border: 1px solid #ccc;
  border-radius: 6px;
  background: #f7f7f7;
  cursor: pointer;
  transition: all 0.2s ease;
}

.featured-btn.active {
  background: #ffb400;
  color: white;
  border-color: #ffb400;
}

.featured-tag {
  display: inline-block;
  margin-left: 6px;
  margin-bottom: 2px;
  padding: 2px 6px;
  font-size: 0.9rem;
  color: #fff;
  background-color: #ffb400;
  border-radius: 4px;
  font-weight: bold;
  vertical-align: middle;
}
@media (prefers-color-scheme: dark) {
  .featured-tag {
    background-color: #ffc107;
    color: #222;
  }
}

.refresh-btn {
  position: fixed;
  bottom: 30px;
  right: 40px;
  width: 60px;
  height: 60px;
  border: none;
  border-radius: 50%;
  background-color: #4c91d9;
  color: white;
  font-size: 2rem;
  cursor: pointer;
  box-shadow: 0 4px 10px rgba(0, 0, 0, 0.3);
  transition:
    transform 0.2s,
    background-color 0.3s;
  z-index: 999;
}

.refresh-btn:hover {
  background-color: #357ac8;
  transform: rotate(90deg) scale(1.1);
}

.refresh-btn:active {
  transform: scale(0.95);
}

/* 深色模式下样式 */
@media (prefers-color-scheme: dark) {
  .refresh-btn {
    background-color: #6ab0ff;
    color: #111;
  }
  .refresh-btn:hover {
    background-color: #5a9be6;
  }
}

.floating-buttons {
  position: fixed;
  right: 20px;
  bottom: 20px;
  display: flex;
  flex-direction: column;
  gap: 12px;
  z-index: 1000;
}
</style>
