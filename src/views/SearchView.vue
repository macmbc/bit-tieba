<!-- eslint-disable @typescript-eslint/no-explicit-any -->
<template>
  <div class="search-page">
    <!-- 搜索关键字展示 -->
    <h2 class="search-title">
      搜索：<span class="keyword">{{ keyword }}</span>
    </h2>

    <!-- 搜索类型切换 -->
    <div class="search-tabs">
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
  max-width: 800px;
  margin: auto;
  padding: 20px;
  font-family: 'Helvetica Neue', Arial, sans-serif;
}
.search-title {
  font-size: 24px;
  font-weight: bold;
  margin-bottom: 20px;
}
.keyword {
  color: var(--color-brand, #409eff);
}
.search-tabs {
  display: flex;
  gap: 12px;
  margin-bottom: 25px;
}
.tab-btn {
  padding: 6px 18px;
  border-radius: 20px;
  border: 1px solid #ccc;
  background: #f5f5f5;
  cursor: pointer;
  transition: 0.2s;
}
.tab-btn.active {
  background: var(--color-brand, #409eff);
  color: #fff;
  border: none;
}
.tab-btn:hover {
  background: #e0e0e0;
}
.result-list {
  display: flex;
  flex-direction: column;
  gap: 16px;
}
.result-item {
  padding: 16px;
  border-radius: 12px;
  background: #fff;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
  transition: 0.2s;
}
.result-item:hover {
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.12);
}
.title {
  font-size: 18px;
  font-weight: 600;
  margin-bottom: 6px;
}
.description {
  color: #555;
}
.meta {
  color: #888;
  font-size: 13px;
  margin-top: 4px;
}
.preview {
  color: #444;
  margin-top: 8px;
}
.user-item {
  display: flex;
  gap: 12px;
  align-items: center;
}
.user-avatar {
  width: 48px;
  height: 48px;
  border-radius: 50%;
  object-fit: cover;
  border: 1px solid #ddd;
}
.state {
  text-align: center;
  padding: 50px 0;
  font-size: 16px;
  color: #888;
}
.state.loading {
  color: var(--color-brand, #409eff);
}
.state.empty {
  color: #bbb;
  font-style: italic;
}
.pagination {
  margin-top: 30px;
  display: flex;
  justify-content: center;
  gap: 15px;
  align-items: center;
}
.page-btn {
  padding: 6px 16px;
  border: 1px solid #ccc;
  background: #f5f5f5;
  border-radius: 6px;
  cursor: pointer;
  transition: 0.2s;
}
.page-btn:disabled {
  cursor: not-allowed;
  opacity: 0.5;
}
.page-info {
  font-weight: 500;
}

.result-item {
  padding: 12px;
  border-bottom: 1px solid #eee;
}

.forum-info {
  display: flex;
  align-items: center; /* 图片和文字垂直居中 */
  gap: 12px; /* 图片和文字的间距 */
}

.forum-logo {
  width: 60px; /* 固定宽度 */
  height: 60px; /* 固定高度 */
  object-fit: cover; /* 保持图片比例填充 */
  border-radius: 50%; /* 圆形头像，可选 */
}

.text-content .title {
  margin: 0;
  font-size: 16px;
  line-height: 1.2;
}

.text-content .description {
  margin: 4px 0 0;
  font-size: 14px;
  color: #666;
}

.text-content .meta {
  margin: 4px 0 0;
  font-size: 12px;
  color: #999;
}

.result-list {
  list-style: none; /* 去掉默认的点 */
  padding: 0; /* 去掉默认内边距 */
  margin: 0; /* 去掉默认外边距 */
}

.result-list .result-item {
  margin-bottom: 12px; /* 每条结果的间距 */
}
</style>
