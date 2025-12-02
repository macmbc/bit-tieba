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

        <section class="post-card" :class="{ loading: loading.post }">
          <!-- 加载状态 -->
          <div v-if="loading.post" class="state-block">帖子加载中...</div>

          <template v-else-if="mainPost">
            <!-- 帖子正文 -->
            <div class="post-content" v-html="safeMainContent"></div>

            <!-- 交互操作栏（重点优化区域）-->
            <div class="post-actions">
              <div class="actions-left">
                <!-- 点赞 -->
                <button
                  class="action-btn like-btn"
                  :class="{ active: mainPost.isLiked }"
                  @click="toggleLike"
                  :disabled="liking"
                >
                  <svg class="icon" viewBox="0 0 24 24">
                    <path
                      fill="currentColor"
                      d="M12 21.35l-1.45-1.32C5.4 15.36 2 12.28 2 8.5 2 5.42 4.42 3 7.5 3c1.74 0 3.41.81 4.5 2.09C13.09 3.81 14.76 3 16.5 3 19.58 3 22 5.42 22 8.5c0 3.78-3.4 6.86-8.55 11.54L12 21.35z"
                    />
                  </svg>
                  <span class="count">{{ formatCount(mainPost.likeCount) }}</span>
                </button>

                <!-- 收藏 -->
                <button
                  class="action-btn collect-btn"
                  :class="{ active: mainPost.isCollected }"
                  @click="toggleCollect"
                  :disabled="collecting"
                >
                  <svg class="icon" viewBox="0 0 24 24">
                    <path
                      fill="currentColor"
                      d="M17 3H7c-1.1 0-1.99.9-1.99 2L5 21l7-3 7 3V5c0-1.1-.9-2-2-2z"
                    />
                  </svg>
                  <span class="count">{{ formatCount(mainPost.collectCount) }}</span>
                </button>

                <button class="action-btn copy-btn" @click="copyPostContent">
                  <svg class="icon" viewBox="0 0 24 24">
                    <path
                      fill="none"
                      stroke="currentColor"
                      stroke-width="2"
                      d="M8 8h12v12H8z M4 4h12v12H4z"
                    />
                  </svg>
                  <span>复制</span>
                </button>
              </div>

              <!-- 可选：右侧放评论/分享 -->
              <button class="action-btn share-btn">
                <svg class="icon" viewBox="0 0 24 24">
                  <path
                    fill="none"
                    stroke="currentColor"
                    stroke-width="2"
                    d="M19 11v9a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V11m14-4l-7-5-7 5m7-5v5"
                  />
                </svg>
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
          <p class="author-meta">所在吧：{{ mainPost.forumName || '未知贴吧' }}</p>
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

    <!-- 回到顶部按钮 -->
    <transition name="backtop">
      <button v-show="showBackTop" class="back-to-top" @click="scrollToTop" aria-label="回到顶部">
        <svg
          width="22"
          height="22"
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          stroke-width="2.4"
          stroke-linecap="round"
          stroke-linejoin="round"
        >
          <path d="M18 15l-6-6-6 6"></path>
        </svg>
      </button>
    </transition>
  </div>
</template>

<script lang="ts" setup>
import { ref, computed, onMounted, onBeforeUnmount, nextTick, onUnmounted } from 'vue'
import { useRoute } from 'vue-router'
import ReplyModal from '@/components/ReplyModal.vue'
import ReplyItem from '@/components/ReplyItem.vue'
import { useUserStore } from '@/stores/user'
import DOMPurify from 'dompurify'
import { formatCount } from '@/utils/format'
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

const showBackTop = ref(false)

// 控制出现/隐藏
const handleScroll = () => {
  showBackTop.value = window.scrollY > 300
}

// 平滑回顶
const scrollToTop = () => {
  window.scrollTo({
    top: 0,
    behavior: 'smooth',
  })
}

onMounted(() => {
  window.addEventListener('scroll', handleScroll, { passive: true })
})

onUnmounted(() => {
  window.removeEventListener('scroll', handleScroll)
})

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

const copyPostContent = async () => {
  try {
    // 获取纯文本（去掉 html 标签）
    const tempDiv = document.createElement('div')
    tempDiv.innerHTML = safeMainContent.value || mainPost.value?.content || ''
    const plainText = tempDiv.innerText

    await navigator.clipboard.writeText(plainText)

    // 提示
    alert('复制成功 ✅')
  } catch (err) {
    console.error('复制失败:', err)
    alert('复制失败，请手动选择复制')
  }
}

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
.action-row {
  display: flex;
  gap: 32px;
  margin-top: 16px;
  padding: 8px 0;
}

.action-btn {
  display: flex;
  align-items: center;
  gap: 6px;
  background: none;
  border: none;
  padding: 8px 12px;
  border-radius: 999px;
  font-size: 15px;
  font-weight: 500;
  color: #536471;
  cursor: pointer;
  transition: all 0.2s ease;
}

.action-btn:hover {
  background-color: rgba(15, 20, 25, 0.1);
}

.action-btn .icon {
  width: 20px;
  height: 20px;
  fill: none;
  stroke: currentColor;
  stroke-width: 2;
  transition: all 0.2s ease;
}

.action-btn .icon.filled {
  fill: currentColor;
  stroke: none;
}

/* 点赞 - 红色 */
.like-btn.active,
.like-btn.active .count {
  color: #f91880;
}
.like-btn:hover {
  background-color: rgba(249, 24, 128, 0.1);
}

/* 收藏 - 黄色/绿色（可自定义） */
.collect-btn.active,
.collect-btn.active .count {
  color: #1d9bf0; /* 或者 #ffd400 金色 */
}
.collect-btn:hover {
  background-color: rgba(29, 155, 240, 0.1);
}

/* 数字高亮 */
.count {
  transition: all 0.2s ease;
}
.count-active {
  font-weight: 700;
}

/* 点赞心跳动画（可选） */
.like-btn:active .icon {
  animation: heartBeat 0.4s ease;
}
@keyframes heartBeat {
  0%,
  100% {
    transform: scale(1);
  }
  50% {
    transform: scale(1.3);
  }
}

.post-card {
  background: var(--color-background-soft);
  color: var(--color-text-1);
  border-radius: 16px;
  padding: 20px 20px 0 20px; /* 下方留空给 actions */
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.08);
  overflow: hidden;
  transition: all 0.2s;
}
.post-card:hover {
  box-shadow: 0 8px 25px rgba(0, 0, 0, 0.1);
}

/* 正文区域 */
.post-content {
  font-size: 16px;
  line-height: 1.7;
  margin-bottom: 16px;
  word-break: break-word;
}

/* 底部操作栏 */
.post-actions {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 0;
  margin: 12px -20px 0 -20px; /* 抵消卡片 padding，铺满宽度 */
  background: var(--color-background-soft);
  border-top: 1px solid #eee;
}

.actions-left {
  display: flex;
  gap: 24px;
  padding-left: 4px;
}

.action-btn {
  display: flex;
  align-items: center;
  gap: 6px;
  background: transparent;
  border: none;
  border-radius: 999px;
  padding: 8px 14px;
  font-size: 15px;
  font-weight: 500;
  color: #666;
  cursor: pointer;
  transition: all 0.2s ease;
}

.action-btn:hover {
  background: rgba(0, 0, 0, 0.06);
}

.action-btn .icon {
  width: 22px;
  height: 22px;
}

/* 已点赞 - 红心 */
.like-btn.active {
  color: #e0245e;
}
.like-btn:hover {
  background: rgba(224, 36, 94, 0.1);
}

/* 已收藏 - 蓝色/黄色任选 */
.collect-btn.active {
  color: #1a91ff; /* 或者 #ffb400 金色 */
}
.collect-btn:hover {
  background: rgba(26, 145, 255, 0.1);
}

.share-btn {
  color: #888;
}
.share-btn:hover {
  background: rgba(0, 0, 0, 0.06);
  color: #333;
}

/* 数字格式化（1.2k、5.8w） */
.count {
  min-width: 24px;
  font-weight: 600;
  color: inherit;
}

.back-to-top {
  position: fixed;
  left: 48px;
  bottom: 64px;
  width: 48px;
  height: 48px;
  border-radius: 999px;
  border: none;
  background: linear-gradient(135deg, #1677ff, #4096ff);
  color: #fff;
  cursor: pointer;
  display: grid;
  place-items: center;
  box-shadow: 0 8px 24px rgba(22, 119, 255, 0.35);
  z-index: 999;
  transition: all 0.3s ease;
}

/* hover 动效 */
.back-to-top:hover {
  transform: translateY(-4px) scale(1.05);
  box-shadow: 0 12px 28px rgba(22, 119, 255, 0.45);
}

/* 点击时 */
.back-to-top:active {
  transform: scale(0.95);
}

/* 出现/消失动画 */
.backtop-enter-active,
.backtop-leave-active {
  transition: all 0.35s ease;
}

.backtop-enter-from {
  opacity: 0;
  transform: translateY(20px) scale(0.8);
}

.backtop-enter-to {
  opacity: 1;
  transform: translateY(0) scale(1);
}

.backtop-leave-from {
  opacity: 1;
  transform: translateY(0) scale(1);
}

.backtop-leave-to {
  opacity: 0;
  transform: translateY(20px) scale(0.85);
}

.copy-btn:hover {
  background: rgba(0, 0, 0, 0.04);
}

.copy-btn:active {
  transform: scale(0.95);
}

</style>
