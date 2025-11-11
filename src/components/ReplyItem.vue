<!-- src/components/ReplyItem.vue -->
<template>
  <div class="reply-wrapper">
    <!-- 一级回复 -->
    <div class="reply-main">
      <div class="reply-header">
        <router-link :to="`/user/${reply.authorId}`" class="author">
          {{ reply.author }}
        </router-link>
        <span class="floor" v-if="reply.floor">#{{ reply.floor }}</span>
        <span class="time">{{ formatTime(reply.createdAt) }}</span>
        <button class="reply-btn" @click="$emit('reply', reply)">回复</button>
      </div>
      <div class="reply-content" v-html="getSafeReplyContent(reply.content)"></div>

      <!-- 点赞按钮 -->
      <div class="reply-actions">
        <button
          class="action-btn like"
          :class="{ active: reply.isLiked, 'anim-like': animating[reply.id] }"
          @click="handleLike(reply.id)"
          :disabled="liking[reply.id]"
        >
          <span class="like-icon"> </span>
          {{ reply.likeCount }}
        </button>
      </div>
    </div>

    <!-- 二级回复 -->
    <div v-if="reply.children?.length" class="sub-replies">
      <template v-if="shouldCollapse && !expanded">
        <ReplyItem
          v-for="sub in displayedChildren"
          :key="sub.id"
          :reply="sub"
          :post-id="postId"
          :has-more-sub="false"
          :loading="false"
          @reply="$emit('reply', $event)"
          @like="handleChildLike"
          @load-more-sub="$emit('load-more-sub', reply.id)"
        />
        <div class="show-all" @click="handleShowAll">查看全部 {{ reply.replyCount }} 条回复</div>
      </template>

      <!-- 展开状态：显示全部（仍支持分页加载） -->
      <template v-else>
        <ReplyItem
          v-for="sub in reply.children"
          :key="sub.id"
          :reply="sub"
          :post-id="postId"
          :loading="false"
          @reply="$emit('reply', $event)"
          @like="handleChildLike"
          @load-more-sub="$emit('load-more-sub', reply.id)"
        />
      </template>
    </div>
  </div>
</template>

<script lang="ts" setup>
import { ref, computed } from 'vue'
import { formatTime } from '@/utils/format'
import { likeReply, unlikeReply } from '@/api/forumApi'
import DOMPurify from 'dompurify'
import { useUserStore } from '@/stores/user'
import type { Reply } from '@/types'

const props = defineProps<{
  reply: Reply
  postId: number
  loading: boolean // 加载子回复的 loading 状态
}>()

const emit = defineEmits<{
  (e: 'reply', reply: Reply): void
  (e: 'load-more-sub', parentId: number): void
  (e: 'like', payload: { replyId: number; isLiked: boolean }): void
}>()

const userStore = useUserStore()
const liking = ref<Record<number, boolean>>({})
const animating = ref<Record<number, boolean>>({})
const expanded = ref(false) // 控制「查看全部」

const getSafeReplyContent = (html: string) => DOMPurify.sanitize(html)

/* ---------- 折叠逻辑 ---------- */
const shouldCollapse = computed(() => props.reply.replyCount! > 2)
const displayedChildren = computed(() => props.reply.children!.slice(0, 2))

// 统一处理点赞（一级 + 二级）
const handleLike = async (replyId: number) => {
  if (!userStore.isLoggedIn || liking.value[replyId]) return

  // 1. 找到目标 reply（用于获取当前状态）
  const target = findReplyById(props.reply, replyId)
  if (!target) return

  // 2. 乐观更新 + 动画
  const newIsLiked = !target.isLiked
  animating.value[replyId] = true
  emit('like', { replyId, isLiked: newIsLiked })
  setTimeout(() => {
    animating.value[replyId] = false
  }, 400)

  liking.value[replyId] = true
  try {
    if (newIsLiked) {
      await likeReply(userStore.userInfo?.id ?? 0, replyId)
    } else {
      await unlikeReply(userStore.userInfo?.id ?? 0, replyId)
    }
  } catch (err) {
    // 失败回滚
    emit('like', { replyId, isLiked: !newIsLiked })
    console.error('点赞失败，已回滚', err)
  } finally {
    liking.value[replyId] = false
  }
}

const handleChildLike = (payload: { replyId: number; isLiked: boolean }) => {
  emit('like', payload)
}

// 递归查找 reply
const findReplyById = (node: Reply, id: number): Reply | null => {
  if (node.id === id) return node
  if (node.children) {
    for (const child of node.children) {
      const found = findReplyById(child, id)
      if (found) return found
    }
  }
  return null
}

const handleShowAll = () => {
  // 展开折叠状态
  expanded.value = true

  // 如果子回复还没加载完，就通知父组件去加载更多
  if (!props.reply.children || props.reply.children.length < props.reply.replyCount!) {
    emit('load-more-sub', props.reply.id)
  }
}
</script>

<style scoped>
/* ==================== 基础 ==================== */
.reply-wrapper {
  margin-bottom: 16px;
}

/* 一级回复卡片 */
.reply-main {
  background: #fafafa;
  border-radius: 6px;
  padding: 12px;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.05);
  transition: box-shadow 0.2s ease;
}
.reply-main:hover {
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
}

/* 头部信息 */
.reply-header {
  display: flex;
  align-items: center;
  flex-wrap: wrap;
  gap: 6px;
  font-size: 0.9rem;
  color: #666;
  margin-bottom: 8px;
}
.author {
  color: #2a72d4;
  font-weight: 500;
  text-decoration: none;
}
.author:hover {
  text-decoration: underline;
}
.floor {
  color: #999;
}
.time {
  color: #aaa;
  font-size: 0.8rem;
}

/* 回复按钮 */
.reply-btn {
  margin-left: auto;
  background: transparent;
  border: none;
  color: #4c91d9;
  font-size: 0.85rem;
  cursor: pointer;
  padding: 2px 6px;
  border-radius: 4px;
}
.reply-btn:hover {
  background: #e8f1ff;
}

/* 内容 */
.reply-content {
  color: #333;
  line-height: 1.6;
  word-break: break-word;
  margin-bottom: 10px;
}

/* 点赞按钮 */
.reply-actions {
  display: flex;
  align-items: center;
  gap: 10px;
}
.action-btn.like {
  position: relative;
  padding: 6px 12px;
  border: 1px solid #ddd;
  border-radius: 4px;
  background: #fff;
  font-size: 0.85rem;
  display: flex;
  align-items: center;
  gap: 4px;
}
.action-btn.like.active {
  background: #ff4757;
  color: #fff;
  border-color: #ff4757;
}
.action-btn.like:hover:not(:disabled) {
  background: #fff0f0;
}
.action-btn.like:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}
.action-btn.like.anim-like .like-icon {
  animation: likePop 0.4s ease;
}
@keyframes likePop {
  0%,
  100% {
    transform: scale(1);
  }
  50% {
    transform: scale(1.5);
  }
}
.action-btn.like .like-icon::before {
  content: '❤';
  opacity: 0.6;
}
.action-btn.like.active .like-icon::before {
  content: '❤';
}

/* ==================== 子回复 ==================== */
.sub-replies {
  margin-top: 8px;
  margin-left: 40px;
  border-left: 3px solid #4c91d9;
  padding-left: 12px;
}

/* 每条子回复 */
.sub-replies :deep(.reply-wrapper) {
  margin-bottom: 8px;
}
.sub-replies :deep(.reply-main) {
  background: #f5f5f5;
  padding: 8px 12px;
  border-radius: 0 4px 4px 0;
  box-shadow: none;
}

/* ==================== 折叠按钮 ==================== */
.show-all {
  margin: 8px 0;
  padding: 6px 12px;
  background: #eef5ff;
  color: #4c91d9;
  font-size: 0.85rem;
  text-align: center;
  cursor: pointer;
  border-radius: 4px;
}
.show-all:hover {
  background: #ddeaff;
}

/* ==================== 加载更多 ==================== */
.load-more-sub {
  text-align: center;
  margin-top: 8px;
}
.load-more-sub button {
  background: #fff;
  border: 1px solid #ccc;
  border-radius: 20px;
  padding: 4px 14px;
  color: #555;
  font-size: 0.85rem;
  cursor: pointer;
}
.load-more-sub button:hover {
  background: #f0f6ff;
  border-color: #4c91d9;
  color: #4c91d9;
}

/* ==================== 暗黑模式 ==================== */
@media (prefers-color-scheme: dark) {
  .reply-main {
    background: #2a2a2a;
  }
  .reply-content {
    color: #ddd;
  }
  .author {
    color: #6ab0ff;
  }
  .action-btn.like {
    background: #333;
    border-color: #555;
    color: #ddd;
  }
  .action-btn.like.active {
    background: #ff4757;
  }
  .sub-replies :deep(.reply-main) {
    background: #333;
  }
  .show-all {
    background: #334;
    color: #6ab0ff;
  }
  .show-all:hover {
    background: #445;
  }
}
</style>
