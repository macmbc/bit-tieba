<template>
  <div class="post-detail">
    <!-- 楼主帖子 -->
    <section class="section main-post" v-if="mainPost">
      <div class="post-header">
        <h1>{{ mainPost.title }}</h1>
        <div class="meta">
          <router-link :to="`/user/${mainPost.authorId}`" class="author">
            {{ mainPost.author }}
          </router-link>
          <span class="time">{{ formatTime(mainPost.createdAt) }}</span>
        </div>
      </div>
      <div class="content" v-html="safeMainContent"></div>
      <div class="actions">
        <button
          class="action-btn like"
          :class="{ active: mainPost.isLiked }"
          @click="toggleLike"
          :disabled="liking"
        >
          {{ mainPost.isLiked ? '已赞' : '点赞' }} {{ mainPost.likeCount }}
        </button>
        <button
          class="action-btn collect"
          :class="{ active: mainPost.isCollected }"
          @click="toggleCollect"
          :disabled="collecting"
        >
          {{ mainPost.isCollected ? '已收藏' : '收藏' }} {{ mainPost.collectCount }}
        </button>
      </div>
    </section>

    <!-- 回复列表（支持楼中楼） -->
    <section class="section replies">
      <h3>回复 ({{ totalReplies }})</h3>
      <div v-if="loading.replies" class="loading">加载中...</div>
      <div v-else-if="error.replies" class="error">{{ error.replies }}</div>
      <div v-else-if="!topLevelReplies.length" class="empty">暂无回复</div>
      <ul v-else class="reply-list">
        <li v-for="reply in topLevelReplies" :key="reply.id" class="reply-item">
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

      <!-- 加载更多一级回复 -->
      <div v-if="hasMore && !loading.more" class="load-more" ref="loadMoreRef">
        <button @click="loadMoreReplies" :disabled="loading.more">
          {{ loading.more ? '加载中...' : '加载更多' }}
        </button>
      </div>
    </section>

    <!-- 右下角发帖按钮 -->
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

    <!-- 回复模态框 -->
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
      getPost(forumId.value, postId.value, userStore.userInfo?.id),
      getReplies(postId.value, 1),
    ])

    mainPost.value = postData
    replies.value = replyData.replies
    totalReplies.value = replyData.total
    hasMore.value = replyData.replies.length === INITIAL_SUB_REPLY_COUNT
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
const topLevelReplies = computed(() => {
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
      userStore.userInfo?.id!,
      postId.value,
      payload.content,
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
      (parentId = parentId),
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
      await unlikePost(userStore.userInfo?.id!, postId.value)
      mainPost.value.isLiked = false
      mainPost.value.likeCount -= 1
    } else {
      await likePost(userStore.userInfo?.id!, postId.value)
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
      await uncollectPost(userStore.userInfo?.id!, postId.value)
      mainPost.value.isCollected = false
      mainPost.value.collectCount -= 1
    } else {
      await collectPost(userStore.userInfo?.id!, postId.value)
      mainPost.value.isCollected = true
      mainPost.value.collectCount += 1
    }
  } finally {
    collecting.value = false
  }
}
</script>

<style scoped>
.post-detail {
  max-width: 1000px;
  margin: 0 auto;
  padding: 20px;
  padding-left: 100px;
}

.section {
  margin-bottom: 30px;
  background: #fff;
  border: 1px solid #eee;
  border-radius: 6px;
  padding: 20px;
}

.main-post h1 {
  font-size: 1.8rem;
  color: #333;
  margin: 0 0 10px;
}

.post-header .meta {
  display: flex;
  align-items: center;
  gap: 15px;
  font-size: 0.9rem;
  color: #999;
  margin-bottom: 15px;
}

.author {
  color: #4c91d9;
  text-decoration: none;
  font-weight: bold;
}

.author:hover {
  text-decoration: underline;
}

.content {
  font-size: 1rem;
  line-height: 1.8;
  color: #333;
  margin-bottom: 20px;
  word-break: break-word;
}

.actions {
  display: flex;
  gap: 15px;
}

.action-btn {
  padding: 8px 16px;
  border: 1px solid #ddd;
  border-radius: 4px;
  background: #fff;
  cursor: pointer;
  font-size: 0.9rem;
  display: flex;
  align-items: center;
  gap: 5px;
}

.action-btn.active {
  background: #4c91d9;
  color: white;
  border-color: #4c91d9;
}

.action-btn:hover:not(:disabled) {
  background: #f0f8ff;
}

.action-btn:disabled {
  background: #f5f5f5;
  cursor: not-allowed;
  opacity: 0.6;
}

.replies h3 {
  font-size: 1.3rem;
  margin-bottom: 15px;
}

.reply-list {
  list-style: none;
  padding: 0;
}

.reply-item {
  border-bottom: 1px solid #eee;
  padding: 15px 0;
}

.reply-item:last-child {
  border-bottom: none;
}

.load-more {
  text-align: center;
  margin-top: 20px;
}

.load-more button {
  padding: 8px 16px;
  border: 1px solid #ddd;
  background: #fff;
  border-radius: 4px;
  cursor: pointer;
}

.loading,
.empty,
.error {
  text-align: center;
  color: #666;
  font-size: 1rem;
  padding: 20px;
}

.error {
  color: red;
}

.fab-reply {
  position: fixed;
  right: 50px;
  bottom: 50px;
  width: 56px;
  height: 56px;
  border-radius: 50%;
  background: #4c91d9;
  color: #fff;
  border: none;
  box-shadow: 0 4px 12px rgba(76, 145, 217, 0.4);
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 1000;
  transition:
    transform 0.2s,
    background 0.2s;
}

.fab-reply:hover {
  background: #3a7ab8;
  transform: scale(1.08);
}

.fab-reply:active {
  transform: scale(0.96);
}

/* 响应式 */
@media (max-width: 576px) {
  .post-detail {
    padding-left: 20px;
  }
  .post-header .meta {
    flex-direction: column;
    align-items: flex-start;
    gap: 5px;
  }
}

/* 暗黑模式 */
@media (prefers-color-scheme: dark) {
  .post-detail {
    background: #1a1a1a;
  }
  .section {
    background: #222;
    border-color: #444;
  }
  h1,
  .content {
    color: #ddd;
  }
  .meta,
  .time,
  .loading,
  .empty {
    color: #aaa;
  }
  .author {
    color: #6ab0ff;
  }
  .action-btn {
    background: #222;
    border-color: #444;
    color: #ddd;
  }
  .action-btn.active {
    background: #6ab0ff;
    border-color: #6ab0ff;
  }
  .action-btn:hover:not(:disabled) {
    background: #2a3a4a;
  }
  .load-more button {
    background: #222;
    border-color: #444;
    color: #ddd;
  }
  .load-more button:hover:not(:disabled) {
    background: #2a3a4a;
  }
  .fab-reply {
    background: #6ab0ff;
  }
  .fab-reply:hover {
    background: #5a9be6;
  }
  .error {
    color: #ff6347;
  }
}
</style>
