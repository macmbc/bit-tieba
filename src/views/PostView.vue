<template>
  <div class="post-page">
    <div class="post-grid">
      <div class="post-main">
        <section v-if="mainPost" class="post-hero">
          <router-link :to="`/forum/${forumId}`">
            <p class="eyebrow">{{ mainPost.forumName || '未知贴吧' }}</p></router-link
          >

          <h1>{{ mainPost.title }}</h1>
          <div class="post-meta">
            <router-link :to="`/user/${mainPost.authorId}`" class="meta-author">
              <!-- 头像区域 -->
              <img
                :src="getUserAvatar(mainPost.authorId)"
                alt="作者头像"
                class="author-avatar"
                :title="mainPost.author || '匿名用户'"
              />
              {{ mainPost.author }}
            </router-link>
            <span>{{ formatTime(mainPost.createdAt) }}</span>
          </div>
        </section>

        <section class="post-body" :class="{ loading: loading.post }">
          <div v-if="loading.post" class="state-block">帖子加载中...</div>
          <template v-else-if="mainPost">
            <div class="post-content" v-html="safeMainContent"></div>
            <div class="action-row">
              <button
                class="action-btn"
                :class="{ active: mainPost.isLiked }"
                @click="toggleLike"
                :disabled="liking"
              >
                {{ mainPost.isLiked ? '已赞' : '点赞' }} · {{ mainPost.likeCount }}
              </button>
              <button
                class="action-btn"
                :class="{ active: mainPost.isCollected }"
                @click="toggleCollect"
                :disabled="collecting"
              >
                {{ mainPost.isCollected ? '已收藏' : '收藏' }} · {{ mainPost.collectCount }}
              </button>
            </div>
          </template>
          <p v-else class="state-block error">未找到该帖子</p>
        </section>

        <section class="replies section">
          <header class="replies-header">
            <div>
              <h3>回复 ({{ totalReplies }})</h3>
              <p>与吧友保持实时互动</p>
            </div>
            <div class="reply-filters">
              <button
                v-for="filter in replyFilters"
                :key="filter.value"
                type="button"
                class="filter-chip"
                :class="{ active: replySort === filter.value }"
                @click="replySort = filter.value"
              >
                {{ filter.label }}
              </button>
            </div>
          </header>
          <div v-if="loading.replies" class="state-block">回复加载中...</div>
          <div v-else-if="error.replies" class="state-block error">{{ error.replies }}</div>
          <div v-else-if="!displayedReplies.length" class="state-block empty">暂无回复</div>
          <ul v-else class="reply-list">
            <li v-for="reply in displayedReplies" :key="reply.id" class="reply-item">
              <ReplyItem
                :reply="reply"
                :post-id="postId"
                :loading="!!loadingSub[reply.id]"
                @reply="openSubReply"
                @load-more-sub="loadMoreSubReplies"
                @like="handleLike"
              />
            </li>
          </ul>
          <div v-if="hasMore" class="load-more" ref="loadMoreRef">
            <button @click="loadMoreReplies" :disabled="loading.more">
              {{ loading.more ? '加载中...' : '加载更多' }}
            </button>
          </div>
        </section>
      </div>

      <aside class="post-aside" v-if="mainPost">
        <div class="aside-card">
          <h4>帖子概览</h4>
          <ul>
            <li v-for="stat in postStats" :key="stat.label">
              <span>{{ stat.label }}</span>
              <strong>{{ stat.value }}</strong>
            </li>
          </ul>
          <button type="button" class="solid-btn" @click="openMainReply">快速回复</button>
        </div>
        <div class="aside-card">
          <h4>楼主信息</h4>
          <p class="author-name">{{ mainPost.author }}</p>
          <p class="author-meta">发表于 {{ formatTime(mainPost.createdAt) }}</p>
          <p class="author-meta">所在吧：{{ mainPost.forumName || "未知贴吧" }}</p>
        </div>
      </aside>
    </div>

    <button class="fab-reply" @click="openMainReply" title="发表回复">
      <svg
        xmlns="http://www.w3.org/2000/svg"
        width="24"
        height="24"
        viewBox="0 0 24 24"
        fill="none"
        stroke="currentColor"
        stroke-width="2.5"
        stroke-linecap="round"
        stroke-linejoin="round"
      >
        <line x1="12" y1="5" x2="12" y2="19"></line>
        <line x1="5" y1="12" x2="19" y2="12"></line>
      </svg>
    </button>

    <ReplyModal
      v-model:show="showReplyModal"
      :replying="replying"
      :target-user="targetUser"
      :parent-id="targetParentId"
      :post-id="postId"
      @submit="handleReplySubmit"
    />
  </div>
</template>

<script lang="ts" setup>
import { ref, computed, onMounted, onBeforeUnmount, nextTick } from 'vue'
import { useRoute } from 'vue-router'
import ReplyModal from '@/components/ReplyModal.vue'
import ReplyItem from '@/components/ReplyItem.vue'
import { useUserStore } from '@/stores/user'
import DOMPurify from 'dompurify'
import {
  getPost,
  getReplies,
  createReply,
  likePost,
  unlikePost,
  collectPost,
  uncollectPost,
} from '@/api/forumApi'
import type { Post, Reply } from '@/types'
import { formatTime } from '@/utils/format'
import { REPLY_CONSTANTS } from '@/constants/forum'
import { getUserAvatar } from '@/utils/load_image'
const { SUB_REPLY_PAGE_SIZE, ROOT_REPLY_PAGE_SIZE, INITIAL_SUB_REPLY_COUNT } = REPLY_CONSTANTS

const route = useRoute()
const userStore = useUserStore()

// 路由参数
const forumId = computed(() => Number(route.params.forumId))
const postId = computed(() => Number(route.params.postId))

// 数据
const mainPost = ref<Post | null>(null)
const replies = ref<Reply[]>([])
const totalReplies = ref(0)
const page = ref(1)
const hasMore = ref(true)

// 楼中楼状态
const loadingSub = ref<Record<number, boolean>>({})

// 状态
const loading = ref({ post: true, replies: true, more: false })
const error = ref({ post: '', replies: '' })
const liking = ref(false)
const collecting = ref(false)
const replying = ref(false)
const showReplyModal = ref(false)
const targetUser = ref('')
const targetParentId = ref<number | undefined>()
type ReplySort = 'latest' | 'hot'
const replySort = ref<ReplySort>('latest')
const replyFilters: Array<{ label: string; value: ReplySort }> = [
  { label: '最新回复', value: 'latest' },
  { label: '最热互动', value: 'hot' },
]

// 安全内容
const safeMainContent = computed(() =>
  mainPost.value ? DOMPurify.sanitize(mainPost.value.content) : '',
)

// 加载更多触发器
const loadMoreRef = ref<HTMLElement | null>(null)
let observer: IntersectionObserver | null = null

// 初始化
onMounted(async () => {
  await loadInitialData()
  await nextTick()

  if (loadMoreRef.value) {
    observer = new IntersectionObserver(
      async (entries) => {
        const entry = entries[0]
        if (entry && entry.isIntersecting && hasMore.value && !loading.value.more) {
          await loadMoreReplies()
        }
      },
      { threshold: 0.1 },
    )
    observer.observe(loadMoreRef.value)
  }
})

onBeforeUnmount(() => {
  if (observer && loadMoreRef.value) {
    observer.unobserve(loadMoreRef.value)
  }
  observer = null
})

// 初始加载
const loadInitialData = async () => {
  try {
    loading.value.post = true
    loading.value.replies = true

    const [postData, replyData] = await Promise.all([
      getPost(forumId.value, postId.value, userStore.userId),
      getReplies(postId.value, 1),
    ])

    mainPost.value = postData
    replies.value = replyData.replies

    totalReplies.value = replyData.total
    hasMore.value = replyData.replies.length === ROOT_REPLY_PAGE_SIZE
  } catch (err) {
    error.value.post = error.value.replies = (err as Error).message
  } finally {
    loading.value.post = false
    loading.value.replies = false
  }
}

// 加载更多回复
const loadMoreReplies = async () => {
  if (loading.value.more || !hasMore.value) return
  loading.value.more = true
  try {
    page.value += 1
    const { replies: newReplies, total } = await getReplies(postId.value, page.value)
    replies.value.push(...newReplies)
    totalReplies.value = total
    hasMore.value = newReplies.length === ROOT_REPLY_PAGE_SIZE
  } catch (err) {
    error.value.replies = (err as Error).message
  } finally {
    loading.value.more = false
  }
}

const handleLike = (payload: { replyId: number; isLiked: boolean }) => {
  const updateReply = (replies: Reply[]): boolean => {
    for (const r of replies) {
      if (r.id === payload.replyId) {
        r.isLiked = payload.isLiked
        r.likeCount += payload.isLiked ? 1 : -1
        return true
      }
      if (r.children && updateReply(r.children)) return true
    }
    return false
  }
  updateReply(replies.value)
}

// 楼中楼：构建树
const replyTree = computed(() => {
  const map = new Map<number, Reply>()
  const top: Reply[] = []

  replies.value.forEach((r) => {
    const reply = { ...r, children: r.children || ([] as Reply[]) }
    map.set(reply.id, reply)
    if (!reply.parentId) {
      top.push(reply)
    } else {
      const parent = map.get(reply.parentId)
      if (parent) parent.children!.push(reply)
    }
  })

  return top
})

const displayedReplies = computed(() => {
  const list = [...replyTree.value]
  if (replySort.value === 'hot') {
    return list.sort(
      (a, b) => b.likeCount + (b.children?.length ?? 0) - (a.likeCount + (a.children?.length ?? 0)),
    )
  }
  return list.sort((a, b) => b.createdAt - a.createdAt)
})

const postStats = computed(() => [
  { label: '楼层数', value: totalReplies.value },
  { label: '点赞', value: mainPost.value?.likeCount ?? 0 },
  { label: '收藏', value: mainPost.value?.collectCount ?? 0 },
])

// 打开回复
const openMainReply = () => {
  targetUser.value = ''
  targetParentId.value = undefined
  showReplyModal.value = true
}

const openSubReply = (reply: Reply) => {
  targetUser.value = reply.author
  targetParentId.value = reply.id
  showReplyModal.value = true
}

// 提交回复
const handleReplySubmit = async (payload: { content: string; parentId?: number }) => {
  if (!userStore.isLoggedIn) return
  replying.value = true
  try {
    const newReply = await createReply(
      userStore.userId!,
      postId.value,
      payload.content,
      replies.value.length,
      payload.parentId,
    )
    if (payload.parentId) {
      // 楼中楼：追加到父楼层的 children
      const parent = findParentReply(replies.value, payload.parentId)
      if (parent) {
        if (!parent.children) parent.children = []
        parent.children.push(newReply)
      }
    } else {
      // 主贴回复：插入顶部
      replies.value.unshift(newReply)
    }
    totalReplies.value += 1
    if (mainPost.value) mainPost.value.replyCount += 1
    showReplyModal.value = false
  } catch (err) {
    error.value.replies = `回复失败：${(err as Error).message}`
  } finally {
    replying.value = false
  }
}

const findParentReply = (roots: Reply[], parentId: number): Reply | undefined => {
  return roots.find((r) => r.id === parentId)
}

// 加载更多子回复
const loadMoreSubReplies = async (parentId: number) => {
  // console.log('loading more subreplies')
  if (loadingSub.value[parentId]) return
  loadingSub.value[parentId] = true

  try {
    const parent = findParentReply(replies.value, parentId)
    if (!parent) {
      console.warn('未找到 parentId 对应的楼层:', parentId)
      return
    }
    const currentCount = parent.children?.length ?? 0
    const page = Math.floor(currentCount / SUB_REPLY_PAGE_SIZE) + 1
    const { replies: newReplies } = await getReplies(
      postId.value,
      page,
      INITIAL_SUB_REPLY_COUNT,
      parentId,
    )
    const existingIds = new Set(parent.children?.map((c) => c.id) ?? [])
    const uniqueNewReplies = newReplies.filter((r) => !existingIds.has(r.id))

    if (!parent.children) parent.children = []
    parent.children.push(...uniqueNewReplies)
  } catch (err) {
    console.error('加载子回复失败', err)
  } finally {
    loadingSub.value[parentId] = false
  }
}

// 点赞/收藏主贴
const toggleLike = async () => {
  if (!userStore.isLoggedIn || !mainPost.value) return
  liking.value = true
  try {
    if (mainPost.value.isLiked) {
      await unlikePost(userStore.userId!, postId.value)
      mainPost.value.isLiked = false
      mainPost.value.likeCount -= 1
    } else {
      await likePost(userStore.userId!, postId.value)
      mainPost.value.isLiked = true
      mainPost.value.likeCount += 1
    }
  } finally {
    liking.value = false
  }
}

const toggleCollect = async () => {
  if (!userStore.isLoggedIn || !mainPost.value) return
  collecting.value = true
  try {
    if (mainPost.value.isCollected) {
      await uncollectPost(userStore.userId!, postId.value)
      mainPost.value.isCollected = false
      mainPost.value.collectCount -= 1
    } else {
      await collectPost(userStore.userId!, postId.value)
      mainPost.value.isCollected = true
      mainPost.value.collectCount += 1
    }
  } finally {
    collecting.value = false
  }
}
</script>

<style scoped>
.post-page {
  max-width: 1200px;
  margin: 0 auto;
  padding: var(--sp-8) var(--sp-5) var(--sp-10);
}

.post-grid {
  display: grid;
  grid-template-columns: minmax(0, 2fr) minmax(280px, 1fr);
  gap: var(--sp-6);
}

.post-hero {
  background: linear-gradient(135deg, rgba(25, 118, 210, 0.16), rgba(25, 118, 210, 0.05));
  border-radius: var(--radius-lg);
  padding: var(--sp-5);
  border: 1px solid rgba(25, 118, 210, 0.2);
  margin-bottom: var(--sp-4);
}

.post-hero h1 {
  font-size: var(--fz-h1);
  margin-bottom: var(--sp-3);
}

.eyebrow {
  font-size: var(--fz-sub);
  letter-spacing: 0.1em;
  text-transform: uppercase;
  color: var(--color-brand);
  margin-bottom: var(--sp-2);
}

.post-meta {
  display: flex;
  gap: var(--sp-4);
  color: var(--color-text-3);
  font-size: var(--fz-sub);
}

.meta-author {
  color: var(--color-brand);
  text-decoration: none;
  font-weight: var(--fw-medium);
}

.meta-author:hover {
  text-decoration: underline;
}

.post-body {
  background: var(--color-background);
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  padding: var(--sp-5);
  box-shadow: var(--shadow-sm);
}

.post-content :deep(p) {
  margin-bottom: var(--sp-3);
  line-height: 1.8;
  color: var(--color-text);
}

.post-content :deep(img) {
  max-width: 100%;
  border-radius: var(--radius-md);
}

.action-row {
  margin-top: var(--sp-4);
  display: flex;
  gap: var(--sp-3);
}

.action-btn {
  flex: none;
  border-radius: var(--radius-md);
  border: 1px solid var(--color-border);
  padding: var(--sp-2) var(--sp-4);
  background: var(--color-background);
  cursor: pointer;
  transition:
    background var(--ease-fast),
    border var(--ease-fast);
}

.action-btn.active {
  border-color: var(--color-brand);
  background: rgba(25, 118, 210, 0.08);
}

.action-btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.section {
  margin-top: var(--sp-6);
  background: var(--color-background);
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  padding: var(--sp-5);
  box-shadow: var(--shadow-sm);
}

.replies-header {
  display: flex;
  justify-content: space-between;
  gap: var(--sp-4);
  margin-bottom: var(--sp-4);
}

.replies-header h3 {
  font-size: var(--fz-h3);
}

.replies-header p {
  color: var(--color-text-2);
  font-size: var(--fz-sub);
}

.reply-filters {
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
  color: var(--color-brand);
  background: rgba(25, 118, 210, 0.08);
}

.reply-list {
  list-style: none;
  padding: 0;
  margin: 0;
  display: flex;
  flex-direction: column;
  gap: var(--sp-3);
}

.reply-item {
  border-bottom: 1px solid var(--color-border);
  padding-bottom: var(--sp-3);
}

.reply-item:last-child {
  border-bottom: none;
}

.load-more {
  margin-top: var(--sp-4);
  text-align: center;
}

.load-more button {
  border-radius: var(--radius-md);
  border: 1px solid var(--color-border);
  padding: var(--sp-2) var(--sp-4);
  cursor: pointer;
}

.post-aside {
  display: flex;
  flex-direction: column;
  gap: var(--sp-4);
}

.aside-card {
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  padding: var(--sp-4);
  background: var(--color-background);
  box-shadow: var(--shadow-sm);
}

.aside-card h4 {
  font-size: var(--fz-h3);
  margin-bottom: var(--sp-3);
}

.aside-card ul {
  list-style: none;
  padding: 0;
  margin: 0 0 var(--sp-4);
  display: flex;
  flex-direction: column;
  gap: var(--sp-2);
}

.aside-card li {
  display: flex;
  justify-content: space-between;
  font-size: var(--fz-body);
}

.solid-btn {
  width: 100%;
  border: none;
  border-radius: var(--radius-md);
  padding: var(--sp-2) var(--sp-4);
  background: var(--color-brand);
  color: #fff;
  cursor: pointer;
}

.author-name {
  font-weight: var(--fw-medium);
  margin-bottom: var(--sp-1);
}

.author-meta {
  color: var(--color-text-3);
  font-size: var(--fz-sub);
}

.aside-tip {
  margin-top: var(--sp-3);
  font-size: var(--fz-sub);
  color: var(--color-text-2);
}

.state-block {
  text-align: center;
  padding: var(--sp-4);
  background: var(--color-background-soft);
  border-radius: var(--radius-md);
  color: var(--color-text-2);
}

.state-block.error {
  color: var(--color-danger);
}

.state-block.empty {
  color: var(--color-text-3);
}

.fab-reply {
  position: fixed;
  right: 40px;
  bottom: 40px;
  width: 56px;
  height: 56px;
  border-radius: 50%;
  background: var(--color-brand);
  color: #fff;
  border: none;
  box-shadow: 0 12px 30px rgba(25, 118, 210, 0.25);
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  transition:
    transform var(--ease-fast),
    background var(--ease-fast);
}

.fab-reply:hover {
  transform: translateY(-2px);
  background: var(--color-brand-hover);
}

.meta-author {
  display: inline-flex;
  align-items: center;
  gap: 8px; /* 头像和文字间距 */
  text-decoration: none;
  color: var(--color-text-1);
}

.author-avatar {
  width: 24px; /* 头像尺寸 */
  height: 24px;
  border-radius: 50%; /* 圆形头像 */
  object-fit: cover; /* 保持图片比例 */
  vertical-align: middle;
  border: 1px solid var(--color-border);
}

.post-meta {
  display: flex;
  align-items: center;
  gap: 12px; /* 作者和时间的间距 */
  color: var(--color-text-3);
  font-size: var(--fz-sub);
}

.post-meta span::before {
  content: '•';
  margin-right: 8px;
  color: var(--color-border-strong);
}

@media (max-width: 960px) {
  .post-grid {
    grid-template-columns: 1fr;
  }
  .post-aside {
    order: -1;
  }
}

@media (prefers-color-scheme: dark) {
  .post-hero,
  .post-body,
  .section,
  .aside-card {
    background: var(--color-background-soft);
  }
  .filter-chip {
    border-color: rgba(255, 255, 255, 0.2);
  }
}
</style>
