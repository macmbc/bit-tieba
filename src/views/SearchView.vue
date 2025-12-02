<!-- eslint-disable @typescript-eslint/no-explicit-any -->
<template>
  <div class="search-page">
    <!-- 搜索关键字展示 -->
    <h2 class="search-title">
      搜索：<span class="keyword">{{ keyword }}</span>
    </h2>

    <!-- 搜索类型切换 -->
    <div class="search-tabs" :data-active="type">
      <button
        v-for="item in types"
        :key="item"
        :class="['tab-btn', { active: type === item }]"
        @click="changeType(item)"
      >
        {{ typeMap[item] }}
      </button>
    </div>

    <!-- 加载状态 -->
    <div v-if="loading" class="state loading">加载中...</div>

    <!-- 结果区域 -->
    <template v-else>
      <!-- 帖子结果 -->
      <ul v-if="type === 'post'" class="result-list">
        <li
          v-for="item in postResults"
          :key="item.post_id"
          class="result-item"
          @click="goToPost(item)"
          style="cursor: pointer"
        >
          <h3 class="title">{{ item.title }}</h3>
          <p class="meta">
            {{ item.author }} · 回复 {{ item.reply_count }} · 点赞 {{ item.like_count }}
          </p>
          <p class="preview">{{ item.content_preview.slice(0, 40) + '...' }}</p>
        </li>
        <div v-if="!postResults?.length" class="state empty">没有帖子搜索结果</div>
      </ul>

      <!-- 贴吧结果 -->
      <ul v-if="type === 'forum'" class="result-list">
        <li
          v-for="item in forumResults"
          :key="item.forum_id"
          class="result-item"
          @click="goToForum(item)"
          style="cursor: pointer"
        >
          <div class="forum-info">
            <img :src="getForumLogo(item.avatar)" alt="" class="forum-logo" />
            <div class="text-content">
              <h3 class="title">{{ item.name }}</h3>
              <p class="description">{{ item.description }}</p>
              <p class="meta">关注：{{ item.follower_count }} · 帖子：{{ item.post_count }}</p>
            </div>
          </div>
        </li>
        <div v-if="!forumResults?.length" class="state empty">没有贴吧搜索结果</div>
      </ul>

      <!-- 用户结果 -->
      <ul v-if="type === 'user'" class="result-list user-list">
        <li v-for="item in userResults" :key="item.uid" class="result-item user-item">
          <img :src="getUserAvatar(String(item.uid))" alt="avatar" class="user-avatar" />
          <div>
            <h3>{{ item.nick || item.name }}</h3>
            <p>{{ item.email }}</p>
            <p>性别：{{ item.sex === 1 ? '男' : '女' }}</p>
          </div>
        </li>
        <div v-if="!userResults?.length" class="state empty">没有用户搜索结果</div>
      </ul>

      <!-- 分页 -->
      <div
        class="pagination"
        v-if="
          (type === 'post' && postResults?.length) ||
          (type === 'forum' && forumResults?.length) ||
          (type === 'user' && userResults?.length)
        "
      >
        <button @click="prevPage" :disabled="page === 1" class="page-btn">上一页</button>
        <span class="page-info">第 {{ page }} 页</span>
        <button @click="nextPage" class="page-btn">下一页</button>
      </div>
    </template>
  </div>
</template>

<script setup lang="ts">
import { ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { search } from '@/api/backendForumApi'
import { getForumLogo, getUserAvatar } from '@/utils/load_image'

type SearchType = 'post' | 'forum' | 'user'

interface PostResult {
  post_id: number
  forum_id: number
  uid: number
  author: string
  title: string
  created_at: string
  reply_count: number
  like_count: number
  is_top: boolean
  is_essence: boolean
  content_preview: string
}
interface ForumResult {
  forum_id: number
  name: string
  avatar: string
  description: string
  follower_count: number
  post_count: number
}
interface UserResult {
  uid: number
  name: string
  nick: string
  email: string
  icon: string
  sex: number
}

const route = useRoute()
const router = useRouter()

const keyword = ref<string>('')
const type = ref<SearchType>('post')
const page = ref<number>(1)
const limit = ref<number>(10)
const loading = ref(false)

const postResults = ref<PostResult[]>([])
const forumResults = ref<ForumResult[]>([])
const userResults = ref<UserResult[]>([])

const types: SearchType[] = ['post', 'forum', 'user']
const typeMap: Record<SearchType, string> = { post: '帖子', forum: '贴吧', user: '用户' }

const goToForum = (forum: ForumResult) => {
  router.push({ name: 'singleForum', params: { forumId: forum.forum_id } })
}

const goToPost = (post: PostResult) => {
  router.push({
    name: 'post',
    params: { forumId: post.forum_id, postId: post.post_id },
  })
}

const fetchData = async () => {
  loading.value = true
  try {
    const { results } = await search({
      keyword: keyword.value,
      type: type.value,
      page: page.value,
      limit: limit.value,
    })
    postResults.value = []
    forumResults.value = []
    userResults.value = []
    console.log('res', results)
    if (type.value === 'post') postResults.value = results as PostResult[]
    if (type.value === 'forum') forumResults.value = results as ForumResult[]
    if (type.value === 'user') userResults.value = results as UserResult[]
  } finally {
    loading.value = false
  }
}

watch(
  () => route.query,
  () => {
    keyword.value = (route.query.q || '').toString()
    type.value = (route.query.type as SearchType) || 'post'
    page.value = Number(route.query.page || 1)
    if (keyword.value) fetchData()
  },
  { immediate: true },
)

const changeType = (t: SearchType) => {
  if (t === type.value) return
  router.replace({ path: '/search', query: { q: keyword.value, type: t, page: '1' } })
}

const nextPage = () => {
  router.replace({ query: { ...route.query, page: page.value + 1 } })
}
const prevPage = () => {
  if (page.value > 1) router.replace({ query: { ...route.query, page: page.value - 1 } })
}
</script>

<style scoped>
.search-page {
  max-width: 900px;
  margin: 0 auto;
  padding: 40px 24px;
  font-family:
    -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;

  background: var(--sp-bg);
  color: var(--sp-text);
}

/* ============ 浅色变量 ============ */
:root {
  --sp-bg: #f6f7f9;
  --sp-card: #ffffff;
  --sp-text: #1a1a1a;
  --sp-text-muted: #6b7280;
  --sp-border: #e5e7eb;
  --sp-hover: #f3f4f6;
  --sp-brand: #0969da;
  --sp-tag-bg: #f0f5ff;
}

/* ============ 暗黑变量 ============ */
.dark {
  --sp-bg: #09090b;
  --sp-card: #18181b;
  --sp-text: #f9fafb;
  --sp-text-muted: #a1a1aa;
  --sp-border: #27272a;
  --sp-hover: #1f2937;
  --sp-brand: #60a5fa;
  --sp-tag-bg: #1e293b;
}

/* 标题区 */
.search-title {
  font-size: 28px;
  font-weight: 700;
  margin-bottom: 8px;
  color: var(--sp-text);
}
.keyword {
  color: var(--sp-brand);
  padding: 2px 6px;
  background: var(--sp-tag-bg);
  border-radius: 6px;
  font-weight: 600;
}

/* Tab */
.search-tabs {
  display: flex;
  gap: 8px;
  margin: 24px 0 32px;
  padding: 6px;
  background: var(--sp-hover);
  border-radius: 16px;
  width: fit-content;
}
.tab-btn {
  padding: 10px 24px;
  border-radius: 12px;
  font-size: 15px;
  color: var(--sp-text-muted);
  background: transparent;
  border: none;
  cursor: pointer;
}
.tab-btn.active {
  color: var(--sp-text);
  font-weight: 600;
  background: var(--sp-card);
}

/* 列表 */
.result-list {
  list-style: none;
  padding: 0;
  margin: 0;
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.result-item {
  background: var(--sp-card);
  border-radius: 16px;
  padding: 18px 20px;
  border: 1px solid var(--sp-border);
  transition: all 0.2s ease;
  border: 1px solid rgba(255, 255, 255, 0.04);
  box-shadow:
    0 6px 20px rgba(0, 0, 0, 0.4),
    inset 0 1px 0 rgba(255, 255, 255, 0.03);
}

.result-item:hover {
  transform: translateY(-2px);
  background: var(--sp-hover);
}

/* 帖子 */
.result-item .title {
  font-size: 18px;
  font-weight: 600;
  color: var(--sp-text);
  margin-bottom: 8px;
}
.result-item .meta {
  font-size: 13px;
  color: var(--sp-text-muted);
}

/* 贴吧 */
.forum-info {
  display: flex;
  align-items: center;
  gap: 16px;
}
.forum-logo {
  width: 64px;
  height: 64px;
  border-radius: 16px;
  border: 1px solid var(--sp-border);
  background: var(--sp-hover);
}
.description {
  color: var(--sp-text-muted);
}

/* 用户 */
.user-list .result-item {
  display: flex;
  align-items: center;
  gap: 16px;
}
.user-avatar {
  width: 56px;
  height: 56px;
  border-radius: 50%;
}
.user-item h3 {
  color: var(--sp-text);
}
.user-item p {
  color: var(--sp-text-muted);
}

/* 状态 */
.state {
  text-align: center;
  padding: 80px 20px;
  color: var(--sp-text-muted);
}

/* 分页 */
.pagination {
  margin-top: 48px;
  display: flex;
  justify-content: center;
  gap: 12px;
}

.page-btn {
  min-width: 44px;
  height: 44px;
  padding: 0 16px;
  border-radius: 12px;
  background: var(--sp-card);
  color: var(--sp-text);
  border: 1px solid var(--sp-border);
  cursor: pointer;
}
.page-btn:hover:not(:disabled) {
  background: var(--sp-hover);
}
.page-btn:disabled {
  opacity: 0.4;
}

/* 响应式 */
@media (max-width: 640px) {
  .search-page {
    padding: 16px;
  }
}
@media (prefers-color-scheme: dark) {
  .search-page {
    --sp-bg: #09090b;
    --sp-card: #18181b;
    --sp-text: #f9fafb;
    --sp-text-muted: #a1a1aa;
    --sp-border: #27272a;
    --sp-hover: #1f2937;
    --sp-brand: #60a5fa;
    --sp-tag-bg: #1e293b;
  }
}
</style>
