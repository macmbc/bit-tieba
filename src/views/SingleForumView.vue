<template>
  <div class="forum-page">
    <section class="forum-hero">
      <div v-if="loading.forum" class="state-block">贴吧信息加载中...</div>
      <template v-else-if="!error.forum">
        <div class="hero-content">
          <!-- 贴吧头像区域 -->
          <div class="forum-avatar">
            <img :src="getForumLogo(forum.avatar)" alt="贴吧头像" class="avatar-img" />
          </div>
          <h1>{{ forum.name }}</h1>
          <p class="description">{{ forum.description }}</p>
          <div class="hero-meta">
            <span>累计帖子 {{ forum.postCount }}</span>
            <span>当前展示 {{ posts.length }}</span>
          </div>
        </div>
        <div class="hero-actions" v-if="userStore.isLoggedIn">
          <button
            class="follow-btn"
            :class="{ followed: isFollowed }"
            @click="toggleFollow"
            :disabled="following"
          >
            {{ following ? '处理中...' : isFollowed ? '已关注' : '关注此吧' }}
          </button>
        </div>
      </template>
      <p v-else class="state-block error">{{ error.forum }}</p>
    </section>

    <section class="stats-grid" v-if="!loading.forum && !error.forum">
      <article v-for="card in forumStats" :key="card.label" class="stat-card">
        <p class="label">{{ card.label }}</p>
        <p class="value" :style="{ fontSize: getFontSize(card.value) }">{{ card.value }}</p>
        <p class="hint">{{ card.hint }}</p>
      </article>
    </section>

    <section class="section posts">
      <header class="section-header">
        <div>
          <h2>最新讨论</h2>
          <p>筛选吧内热帖，快速加入讨论</p>
        </div>
        <div class="post-filters">
          <button
            v-for="filter in postFilters"
            :key="filter.value"
            type="button"
            class="filter-chip"
            :class="{ active: activeFilter === filter.value }"
            @click="activeFilter = filter.value"
          >
            {{ filter.label }}
          </button>
        </div>
      </header>

      <div v-if="loading.posts" class="state-block">帖子加载中...</div>
      <div v-else-if="error.posts" class="state-block error">{{ error.posts }}</div>
      <div v-else-if="!filteredPosts.length" class="state-block empty">
        暂无帖子，来做第一个发帖人吧
      </div>
      <ul v-else class="post-list">
        <li v-for="post in filteredPosts" :key="post.id" class="post-item">
          <router-link :to="`/forum/${forum.id}/post/${post.id}`">
            <p class="post-meta">
              <span>{{ post.author }}</span>
              <span>{{ formatTime(post.createdAt) }}</span>
            </p>
            <h3>{{ post.title }}</h3>
            <p class="content">{{ post.content.substring(0, 96) }}...</p>
            <div class="post-stats">
              <span>回复 {{ post.replyCount }}</span>
              <span>点赞 {{ post.likeCount ?? 0 }}</span>
            </div>
          </router-link>
        </li>
      </ul>
      <div v-if="hasMore && !loading.posts" class="load-more">
        <button @click="loadMorePosts" :disabled="loading.more">
          {{ loading.more ? '加载中...' : '加载更多' }}
        </button>
      </div>
    </section>

    <!-- 右下角发帖按钮 -->
    <button class="floating-post-btn" @click="showPostModal = true" v-if="userStore.isLoggedIn">
      +
    </button>

    <!-- 简化的发帖弹窗（带渐变动画） -->
    <Transition name="modal">
      <div class="modal-overlay" v-if="showPostModal" @click="showPostModal = false">
        <div class="modal-content" @click.stop>
          <div class="modal-header">
            <h3>发布新帖</h3>
            <button class="close-btn" @click="showPostModal = false">×</button>
          </div>

          <form class="post-form" @submit.prevent="submitPost">
            <div class="form-group">
              <input type="text" v-model="postForm.title" placeholder="请输入标题" required />
            </div>

            <div class="form-group">
              <textarea
                v-model="postForm.content"
                placeholder="请输入内容"
                rows="6"
                required
              ></textarea>
            </div>

            <button type="submit" class="submit-btn" :disabled="submitting">
              {{ submitting ? '发布中...' : '发布' }}
            </button>
          </form>
        </div>
      </div>
    </Transition>
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
import { createPost } from '@/api/forumApi'
import { getForumLogo } from '@/utils/load_image'

// 发帖相关状态
const showPostModal = ref(false)
const submitting = ref(false)
const postForm = ref({
  title: '',
  content: '',
})

// eslint-disable-next-line @typescript-eslint/no-explicit-any
const getFontSize = (value: any) => {
  const length = String(value).length
  // 根据长度设置不同的字体大小
  if (length > 10) {
    return '20px'
  } else if (length > 5) {
    return '14px'
  } else {
    return '48px'
  }
}

const route = useRoute()
const userStore = useUserStore()
const forum = ref<Forum>({
  id: 0,
  name: '',
  description: '',
  postCount: 0,
  followerCount: 0,
  avatar: '',
})
const posts = ref<Post[]>([])
const page = ref(1)
const hasMore = ref(true)
const loading = ref({ forum: true, posts: true, more: false })
const error = ref({ forum: '', posts: '' })
const following = ref(false)
const isFollowed = ref(false)
type PostFilter = 'latest' | 'hot' | 'reply'
const postFilters: Array<{ label: string; value: PostFilter }> = [
  { label: '按最新', value: 'latest' },
  { label: '按热度', value: 'hot' },
  { label: '按回复', value: 'reply' },
]
const activeFilter = ref<PostFilter>('latest')

// 获取贴吧 ID
const forumId = computed(() => Number(route.params.forumId))

// 提交发帖
const submitPost = async () => {
  if (!postForm.value.title.trim() || !postForm.value.content.trim()) return

  try {
    submitting.value = true

    // 调用你的发帖API
    const newPost = await createPost(
      postForm.value.title,
      postForm.value.content,
      forumId.value,
      forum.value.name,
    )

    // 发布成功后的处理
    showPostModal.value = false
    postForm.value = { title: '', content: '' }
    posts.value.unshift(newPost)
    // 可以刷新帖子列表
    // fetchPosts();

    // alert('发布成功！') // 可替换为你的提示组件
  } catch (error) {
    console.error('发帖失败:', error)
    // alert('发布失败，请重试')
  } finally {
    submitting.value = false
  }
}

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

const filteredPosts = computed(() => {
  const list = [...posts.value]
  if (activeFilter.value === 'hot') {
    return list.sort((a, b) => b.likeCount - a.likeCount)
  }
  if (activeFilter.value === 'reply') {
    return list.sort((a, b) => b.replyCount - a.replyCount)
  }
  return list.sort((a, b) => b.createdAt - a.createdAt)
})

const forumStats = computed(() => {
  const avgReplies = posts.value.length
    ? Math.round(posts.value.reduce((sum, item) => sum + item.replyCount, 0) / posts.value.length)
    : 0
  const latestPost = posts.value[0]
  return [
    { label: '帖子总数', value: forum.value.postCount || '--', hint: '官方统计的累计帖子' },
    { label: '本页帖子', value: posts.value.length || '--', hint: '当前拉取的讨论数量' },
    {
      label: '平均回复',
      value: avgReplies || '--',
      hint: avgReplies ? '互动热度参考值' : '等待更多互动数据',
    },
    {
      label: '最新更新',
      value: latestPost ? formatTime(latestPost.createdAt) : '--',
      hint: latestPost ? latestPost.title.slice(0, 5) + '...' : '暂无最新更新',
    },
  ]
})

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
.forum-page {
  max-width: var(--layout-heart-width);
  margin: 0 auto;
  padding: var(--sp-8) var(--sp-5) var(--sp-10);
  display: flex;
  flex-direction: column;
  gap: var(--sp-6);
}

.forum-hero {
  background: linear-gradient(135deg, rgba(25, 118, 210, 0.14), rgba(25, 118, 210, 0.05));
  border-radius: var(--radius-lg);
  padding: var(--sp-6);
  border: 1px solid rgba(25, 118, 210, 0.2);
  display: flex;
  justify-content: space-between;
  gap: var(--sp-5);
  align-items: flex-start;
}

.hero-content h1 {
  font-size: var(--fz-h1);
  margin-bottom: var(--sp-2);
}

.description {
  color: var(--color-text-2);
  margin-bottom: var(--sp-3);
}

.eyebrow {
  font-size: var(--fz-sub);
  letter-spacing: 0.12em;
  text-transform: uppercase;
  color: var(--color-brand);
  margin-bottom: var(--sp-1);
}

.hero-meta {
  display: flex;
  gap: var(--sp-4);
  color: var(--color-text-3);
  font-size: var(--fz-sub);
}

.hero-actions {
  display: flex;
  gap: var(--sp-3);
}

.follow-btn {
  border-radius: var(--radius-md);
  padding: var(--sp-2) var(--sp-4);
  background: var(--color-brand);
  border: none;
  color: #fff;
  cursor: pointer;
  font-weight: var(--fw-medium);
}

.follow-btn.followed {
  background: rgba(25, 118, 210, 0.2);
  color: var(--color-brand);
}

.stats-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: var(--sp-4);
}

.stat-card {
  background: var(--color-background);
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  padding: var(--sp-4);
  box-shadow: var(--shadow-sm);
}

.stat-card .label {
  font-size: var(--fz-sub);
  text-transform: uppercase;
  color: var(--color-text-3);
}

.stat-card .value {
  font-size: 32px;
  font-weight: var(--fw-bold);
  margin: var(--sp-2) 0;
}

.stat-card .hint {
  color: var(--color-text-2);
  font-size: var(--fz-sub);
}

.section {
  background: var(--color-background);
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  padding: var(--sp-5);
  box-shadow: var(--shadow-sm);
}

.section-header {
  display: flex;
  justify-content: space-between;
  gap: var(--sp-4);
  flex-wrap: wrap;
  margin-bottom: var(--sp-4);
}

.section-header h2 {
  font-size: var(--fz-h2);
}

.section-header p {
  color: var(--color-text-2);
  font-size: var(--fz-sub);
}

.post-filters {
  display: flex;
  gap: var(--sp-2);
  flex-wrap: wrap;
}

.filter-chip {
  border-radius: 999px;
  border: 1px solid var(--color-border);
  background: transparent;
  padding: var(--sp-1) var(--sp-3);
  font-size: var(--fz-sub);
  cursor: pointer;
}

.filter-chip.active {
  border-color: var(--color-brand);
  background: rgba(25, 118, 210, 0.1);
  color: var(--color-brand);
}

.post-list {
  list-style: none;
  margin: 0;
  padding: 0;
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(260px, 1fr));
  gap: var(--sp-4);
}

.post-item {
  border: 1px solid var(--color-border);
  border-radius: var(--radius-md);
  background: var(--color-background);
  transition:
    border var(--ease-fast),
    box-shadow var(--ease-fast);
}

.post-item a {
  display: block;
  padding: var(--sp-4);
  text-decoration: none;
  color: inherit;
}

.post-item:hover {
  border-color: var(--color-border-hover);
  box-shadow: var(--shadow-sm);
}

.post-meta {
  display: flex;
  justify-content: space-between;
  font-size: var(--fz-sub);
  color: var(--color-text-3);
}

.post-item h3 {
  margin: var(--sp-2) 0;
  font-size: var(--fz-h3);
}

.post-item .content {
  color: var(--color-text-2);
  margin-bottom: var(--sp-3);
}

.post-stats {
  display: flex;
  justify-content: space-between;
  font-size: var(--fz-sub);
  color: var(--color-text-3);
}

.load-more {
  margin-top: var(--sp-5);
  text-align: center;
}

.load-more button {
  border-radius: var(--radius-md);
  border: 1px solid var(--color-border);
  padding: var(--sp-2) var(--sp-4);
  background: transparent;
  cursor: pointer;
}

.state-block {
  text-align: center;
  padding: var(--sp-5);
  border-radius: var(--radius-md);
  background: var(--color-background-soft);
  color: var(--color-text-2);
}

.state-block.error {
  color: var(--color-danger);
}

.state-block.empty {
  color: var(--color-text-3);
}

@media (max-width: 768px) {
  .forum-hero {
    flex-direction: column;
  }
  .hero-meta {
    flex-direction: column;
  }
}

@media (prefers-color-scheme: dark) {
  .forum-hero,
  .section,
  .stat-card,
  .post-item {
    background: var(--color-background-soft);
  }
  .follow-btn.followed {
    background: rgba(255, 255, 255, 0.08);
  }
}

.floating-post-btn {
  position: fixed;
  right: var(--sp-4);
  bottom: var(--sp-4);
  width: 60px;
  height: 60px;
  border-radius: 50%;
  background: var(--color-brand);
  color: white;
  border: none;
  font-size: 0;
  font-weight: var(--fw-bold);
  cursor: pointer;
  box-shadow: var(--shadow-md);
  z-index: 99;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: transform 0.2s;
  padding: 0; /* 清除浏览器默认内边距 */
}

.floating-post-btn::after {
  content: '+';
  font-weight: var(--fw-bold); /* 显式继承加粗样式 */
  font-size: 48px;
  font-family: inherit; /* 确保字体一致 */
  line-height: 1;
  transform: translateY(-3px); /* 微调垂直偏移 */
}

.floating-post-btn:hover {
  transform: scale(1.05);
}

/* 弹窗样式 */
.modal-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.5);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 100;
  padding: var(--sp-3);
}

.modal-content {
  background: var(--color-card-bg);
  border-radius: var(--radius-lg);
  width: 100%;
  max-width: 500px;
  padding: var(--sp-4);
  box-shadow: var(--shadow-lg);
}

.modal-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: var(--sp-3);
}

.modal-header h3 {
  margin: 0;
}

.close-btn {
  background: transparent;
  border: none;
  font-size: var(--fz-h3);
  cursor: pointer;
  color: var(--color-text-3);
}

/* 表单样式 */
.post-form {
  display: flex;
  flex-direction: column;
  gap: var(--sp-3);
}

.form-group input,
.form-group textarea {
  width: 100%;
  padding: var(--sp-2);
  border: 1px solid var(--color-border);
  border-radius: var(--radius-md);
  font-size: var(--fz-body);
}

.submit-btn {
  background: var(--color-brand);
  color: white;
  border: none;
  padding: var(--sp-2);
  border-radius: var(--radius-md);
  cursor: pointer;
  font-weight: var(--fw-medium);
}

.submit-btn:disabled {
  background: var(--color-border);
  cursor: not-allowed;
}

.hero-content-wrapper {
  display: flex;
  align-items: center;
  gap: var(--sp-4);
  padding: var(--sp-4);
}

/* 贴吧头像样式 */
.forum-avatar {
  width: 80px;
  height: 80px;
  border-radius: 50%;
  overflow: hidden;
  flex-shrink: 0;
  border: 2px solid var(--color-border);
  background: var(--color-background);
}

.avatar-img {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

/* 内容区域自适应 */
.hero-content {
  flex: 1;
}

/* 响应式调整 */
@media (max-width: 768px) {
  .hero-content-wrapper {
    flex-direction: column;
    align-items: flex-start;
    gap: var(--sp-3);
  }

  .forum-avatar {
    width: 64px;
    height: 64px;
  }
}

/* 遮罩层背景 + 模糊 */
.modal-overlay {
  position: fixed;
  inset: 0;
  background: rgba(15, 23, 42, 0.45); /* 半透明暗色 */
  backdrop-filter: blur(6px); /* 背景模糊 */
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 999;
}

/* 弹窗主体 */
.modal-content {
  width: 100%;
  max-width: 520px;
  background: #ffffff;
  border-radius: 20px;
  padding: 28px;
  box-shadow: 0 20px 60px rgba(0, 0, 0, 0.25);
  transform-origin: center;
}

/* 进入动画 - 初始状态 */
.modal-enter-from .modal-content {
  opacity: 0;
  transform: scale(0.9) translateY(30px);
}

/* 离开动画 - 最终状态 */
.modal-leave-to .modal-content {
  opacity: 0;
  transform: scale(0.9) translateY(30px);
}

/* 进入动画 - 遮罩 */
.modal-enter-from {
  opacity: 0;
}

/* 离开动画 - 遮罩 */
.modal-leave-to {
  opacity: 0;
}

/* 动画时间控制 */
.modal-enter-active,
.modal-leave-active {
  transition: all 0.35s cubic-bezier(0.3, 0.9, 0.35, 1);
}

/* 防止内容被点击时选中 */
.modal-content * {
  user-select: none;
}

/* 关闭按钮美化 */
.close-btn {
  font-size: 22px;
  background: transparent;
  border: none;
  cursor: pointer;
  line-height: 1;
}
</style>
