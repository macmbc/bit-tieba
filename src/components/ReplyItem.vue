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
        <div class="show-all" @click="handleShowAll">
          查看全部 {{ reply.replyCount }} 条回复
        </div>
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
const PREVIEW_CHILDREN = 2
const shouldCollapse = computed(() => (props.reply.replyCount ?? 0) > PREVIEW_CHILDREN)
const displayedChildren = computed(() => props.reply.children?.slice(0, PREVIEW_CHILDREN) ?? [])

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
      await likeReply(userStore.userId!, replyId)
    } else {
      await unlikeReply(userStore.userId!, replyId)
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
  if (
    !props.reply.children ||
    props.reply.children.length < props.reply.replyCount!
  ) {
    emit('load-more-sub', props.reply.id)
  }
}
</script>

<style scoped>
.reply-wrapper {
  margin-bottom: var(--sp-4);
}

.reply-main {
  background: var(--color-card-bg);
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  padding: var(--sp-4);
  box-shadow: var(--shadow-sm);
  transition: box-shadow var(--ease-fast) ease, transform var(--ease-fast) ease;
}

.reply-main:hover {
  box-shadow: var(--shadow-md);
  transform: translateY(-1px);
}

.reply-header {
  display: flex;
  align-items: center;
  flex-wrap: wrap;
  gap: var(--sp-2);
  font-size: var(--fz-sub);
  color: var(--color-text-3);
  margin-bottom: var(--sp-2);
}

.author {
  color: var(--color-brand);
  font-weight: var(--fw-medium);
  text-decoration: none;
}

.author:hover {
  text-decoration: underline;
}

.floor {
  color: var(--color-text-2);
}

.time {
  color: var(--color-text-3);
  font-size: var(--fz-caption);
}

.reply-btn {
  margin-left: auto;
  border: none;
  background: transparent;
  color: var(--color-brand);
  font-size: var(--fz-sub);
  font-weight: var(--fw-medium);
  padding: var(--sp-1) var(--sp-2);
  border-radius: var(--radius-sm);
  transition: background-color var(--ease-fast) ease, color var(--ease-fast) ease;
}

.reply-btn:hover {
  background: color-mix(in srgb, var(--color-brand) 12%, transparent);
  color: var(--color-brand-hover);
}

.reply-content {
  color: var(--color-text-1);
  line-height: var(--lh-body);
  word-break: break-word;
  margin-bottom: var(--sp-3);
}

.reply-actions {
  display: flex;
  justify-content: flex-end;
}

.action-btn.like {
  display: inline-flex;
  align-items: center;
  gap: var(--sp-2);
  padding: var(--sp-2) var(--sp-4);
  border: 1px solid var(--color-border);
  border-radius: var(--radius-md);
  background: var(--color-background);
  font-size: var(--fz-sub);
  color: var(--color-text-2);
  cursor: pointer;
  transition: background-color var(--ease-fast) ease, color var(--ease-fast) ease, border-color var(--ease-fast) ease;
}

.action-btn.like:hover:not(:disabled) {
  border-color: var(--color-brand);
  color: var(--color-brand);
}

.action-btn.like.active {
  background: color-mix(in srgb, var(--color-brand) 12%, transparent);
  border-color: var(--color-brand);
  color: var(--color-brand);
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
    transform: scale(1.4);
  }
}

.action-btn.like .like-icon::before {
  content: '❤';
  color: currentColor;
}

.sub-replies {
  margin-top: var(--sp-3);
  margin-left: var(--sp-6);
  border-left: 2px solid color-mix(in srgb, var(--color-brand) 25%, transparent);
  padding-left: var(--sp-3);
}

.sub-replies :deep(.reply-wrapper) {
  margin-bottom: var(--sp-3);
}

.sub-replies :deep(.reply-main) {
  background: var(--color-background);
  border: 1px dashed var(--color-border);
  box-shadow: none;
}

.show-all {
  margin: var(--sp-2) 0;
  padding: var(--sp-2) var(--sp-3);
  background: color-mix(in srgb, var(--color-brand) 10%, transparent);
  color: var(--color-brand);
  font-size: var(--fz-sub);
  text-align: center;
  border-radius: var(--radius-md);
  cursor: pointer;
  transition: background-color var(--ease-fast) ease;
}

.show-all:hover {
  background: color-mix(in srgb, var(--color-brand) 18%, transparent);
}

.load-more-sub {
  text-align: center;
  margin-top: var(--sp-2);
}

.load-more-sub button {
  border: 1px solid var(--color-border);
  border-radius: 999px;
  background: transparent;
  padding: var(--sp-1) var(--sp-4);
  color: var(--color-text-2);
  font-size: var(--fz-caption);
  cursor: pointer;
}

.load-more-sub button:hover {
  border-color: var(--color-brand);
  color: var(--color-brand);
}

@media (max-width: 576px) {
  .reply-main {
    padding: var(--sp-3);
  }

  .sub-replies {
    margin-left: var(--sp-3);
  }
}

@media (prefers-reduced-motion: reduce) {
  .reply-main,
  .action-btn.like.anim-like .like-icon {
    transition: none;
    animation: none;
  }
}
</style>
