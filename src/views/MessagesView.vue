<template>
  <div class="messages-page">
    <section class="messages-hero">
      <div>
        <p class="eyebrow">消息中心</p>
        <h1>我的消息</h1>
        <p class="subtitle">集中查看帖子互动、系统通知与私信，及时跟进社区动态</p>
      </div>
      <div class="hero-actions">
        <span class="unread-pill" aria-live="polite">未读 <strong>{{ unreadCount }}</strong></span>
        <button
          v-if="canMarkAll || markingAll"
          type="button"
          class="primary-btn"
          @click="markAllAsRead"
          :disabled="markingAll || !canMarkAll"
        >
          {{ markingAll ? '标记中...' : '全部标记为已读' }}
        </button>
      </div>
    </section>

    <section class="messages-panel">
      <div class="filter-group" role="tablist" aria-label="消息类型筛选">
        <button
          v-for="option in filterOptions"
          :key="option.value"
          type="button"
          class="filter-chip"
          :class="{ active: filter === option.value }"
          role="tab"
          :aria-selected="filter === option.value"
          @click="filter = option.value"
        >
          <span>{{ option.label }}</span>
          <span class="chip-count">{{ getFilterCount(option.value) }}</span>
        </button>
      </div>

      <div class="state-wrapper" aria-live="polite">
        <ul v-if="loading" class="message-list skeleton-list">
          <li v-for="i in 4" :key="`skeleton-${i}`" class="message-item">
            <div class="avatar skeleton"></div>
            <div class="skeleton-text">
              <div class="skeleton-line" style="width: 60%"></div>
              <div class="skeleton-line" style="width: 80%"></div>
              <div class="skeleton-line" style="width: 40%"></div>
            </div>
          </li>
        </ul>

        <div v-else-if="error" class="state-card error">加载失败：{{ error }}</div>
        <div v-else-if="!filteredMessages.length" class="state-card empty">
          <h3>暂无消息</h3>
          <p>你还没有新的通知，去逛逛热门帖子吧～</p>
        </div>

        <ul v-else class="message-list">
          <li
            v-for="message in filteredMessages"
            :key="message.id"
            class="message-item"
            :class="{ unread: !message.isRead }"
          >
            <router-link :to="message.link" class="message-card" @click="markAsRead(message.id)">
              <div class="card-head">
                <div class="avatar" aria-hidden="true">{{ message.sender?.charAt(0) || '访' }}</div>
                <div class="sender-meta">
                  <p class="sender">{{ message.sender }}</p>
                  <p class="time">{{ formatTime(message.timestamp) }}</p>
                </div>
                <span v-if="!message.isRead" class="status-pill">未读</span>
              </div>

              <p class="content" :title="message.content">{{ message.content }}</p>

              <div class="card-meta">
                <span class="badge">{{ messageTypeText(message.type) }}</span>
                <span class="origin">来自 {{ message.source }}</span>
              </div>
            </router-link>
          </li>
        </ul>
      </div>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed, nextTick, onMounted, ref } from 'vue'
import { useUserStore, useMessageStore } from '@/stores/user'
import { getMessages, markAllMessagesAsRead, markMessageAsRead } from '@/api/forumApi'
import type { Message } from '@/types'
import { formatTime } from '@/utils/format'

type FilterOption = Message['type'] | 'all'

const filterOptions: Array<{ value: FilterOption; label: string }> = [
  { value: 'all', label: '全部' },
  { value: 'post_reply', label: '帖子回复' },
  { value: 'comment_reply', label: '评论回复' },
  { value: 'mention', label: '@我' },
  { value: 'system', label: '系统通知' },
  { value: 'private_message', label: '私信' },
  { value: 'like_collect', label: '点赞收藏' },
]

const userStore = useUserStore()
const messageStore = useMessageStore()

const messages = ref<Message[]>([])
const loading = ref(true)
const error = ref('')
const filter = ref<FilterOption>('all')
const markingAll = ref(false)

const filteredMessages = computed(() =>
  filter.value === 'all'
    ? messages.value
    : messages.value.filter((msg) => msg.type === filter.value),
)

const messageCounts = computed(() => {
  return messages.value.reduce((acc, msg) => {
    acc[msg.type] = (acc[msg.type] ?? 0) + 1
    return acc
  }, {} as Record<Message['type'], number>)
})

const unreadCount = computed(() => messages.value.filter((msg) => !msg.isRead).length)
const canMarkAll = computed(() => filteredMessages.value.some((msg) => !msg.isRead))

const getFilterCount = (value: FilterOption) =>
  value === 'all' ? messages.value.length : messageCounts.value[value] ?? 0

const loadMessages = async () => {
  if (!userStore.isLoggedIn || !userStore.userId) {
    error.value = '请先登录以查看消息'
    messages.value = []
    loading.value = false
    return
  }

  try {
    loading.value = true
    error.value = ''
    messages.value = await getMessages(userStore.userId)
  } catch (err) {
    error.value = (err as Error).message || '获取消息失败'
  } finally {
    loading.value = false
  }
}

onMounted(loadMessages)

const messageTypeText = (type: Message['type']) => {
  switch (type) {
    case 'post_reply':
      return '回复了你的帖子'
    case 'comment_reply':
      return '回复了你的评论'
    case 'mention':
      return '@了你'
    case 'system':
      return '系统通知'
    case 'private_message':
      return '私信'
    case 'like_collect':
      return '点赞/收藏'
    default:
      return '消息'
  }
}

const markAsRead = async (messageId: number) => {
  const msg = messages.value.find((m) => m.id === messageId)
  if (!msg || msg.isRead) return

  msg.isRead = true
  try {
    await markMessageAsRead(userStore.userId!, messageId)
    await messageStore.refreshUnreadCount(userStore.userId!)
  } catch {
    msg.isRead = false
  }
}

const markAllAsRead = async () => {
  if (!userStore.isLoggedIn || !userStore.userId || !canMarkAll.value) return

  markingAll.value = true
  await nextTick()

  const originalMessages = [...messages.value]
  try {
    messages.value = messages.value.map((msg) =>
      (filter.value === 'all' || msg.type === filter.value) && !msg.isRead
        ? { ...msg, isRead: true }
        : msg,
    )

    await markAllMessagesAsRead(
      userStore.userId!,
      filter.value === 'all' ? undefined : filter.value,
    )

    await messageStore.refreshUnreadCount(userStore.userId!)
  } catch (err) {
    error.value = `批量标记已读失败：${(err as Error).message}`
    messages.value = originalMessages
  } finally {
    markingAll.value = false
  }
}
</script>

<style scoped>
.messages-page {
  max-width: var(--layout-heart-width);
  margin: 0 auto;
  padding: var(--sp-6) var(--sp-4) var(--sp-8);
  color: var(--color-text-1);
}

.messages-hero {
  background: linear-gradient(135deg, color-mix(in srgb, var(--color-brand) 12%, transparent),
      color-mix(in srgb, var(--color-brand) 4%, transparent));
  border: 1px solid color-mix(in srgb, var(--color-brand) 18%, transparent);
  border-radius: var(--radius-lg);
  padding: var(--sp-6);
  display: flex;
  justify-content: space-between;
  flex-wrap: wrap;
  gap: var(--sp-4);
  box-shadow: var(--shadow-sm);
}

.eyebrow {
  font-size: var(--fz-caption);
  text-transform: uppercase;
  letter-spacing: 0.08em;
  color: var(--color-text-3);
  margin-bottom: var(--sp-2);
}

.messages-hero h1 {
  font-size: var(--fz-h1);
  line-height: var(--lh-title);
  margin-bottom: var(--sp-2);
}

.subtitle {
  color: var(--color-text-2);
  font-size: var(--fz-sub);
}

.hero-actions {
  display: flex;
  align-items: center;
  gap: var(--sp-3);
}

.unread-pill {
  display: inline-flex;
  align-items: center;
  gap: var(--sp-2);
  padding: var(--sp-2) var(--sp-4);
  border-radius: 999px;
  background: var(--color-background);
  font-weight: var(--fw-medium);
  color: var(--color-text-2);
  box-shadow: var(--shadow-sm);
}

.primary-btn {
  min-width: 140px;
  height: 40px;
  border: none;
  border-radius: var(--radius-md);
  background: var(--color-brand);
  color: #fff;
  font-weight: var(--fw-medium);
  cursor: pointer;
  transition: background-color var(--ease-fast) ease;
}

.primary-btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.primary-btn:not(:disabled):hover {
  background: var(--color-brand-hover);
}

.messages-panel {
  margin-top: var(--sp-6);
  background: var(--color-card-bg);
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  padding: var(--sp-5);
  box-shadow: var(--shadow-sm);
}

.filter-group {
  display: flex;
  flex-wrap: wrap;
  gap: var(--sp-2);
  margin-bottom: var(--sp-4);
}

.filter-chip {
  border: 1px solid var(--color-border);
  background: transparent;
  color: var(--color-text-2);
  padding: var(--sp-2) var(--sp-4);
  border-radius: 999px;
  font-size: var(--fz-sub);
  display: inline-flex;
  align-items: center;
  gap: var(--sp-2);
  cursor: pointer;
  transition: border-color var(--ease-fast) ease, color var(--ease-fast) ease,
    background-color var(--ease-fast) ease;
}

.filter-chip.active {
  background: color-mix(in srgb, var(--color-brand) 12%, transparent);
  border-color: var(--color-brand);
  color: var(--color-brand);
}

.chip-count {
  font-size: var(--fz-caption);
  color: inherit;
}

.state-wrapper {
  min-height: 200px;
}

.message-list {
  list-style: none;
  margin: 0;
  padding: 0;
  display: flex;
  flex-direction: column;
  gap: var(--sp-3);
}

.message-item {
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  background: var(--color-background);
  box-shadow: var(--shadow-sm);
  transition: box-shadow var(--ease-fast) ease, transform var(--ease-fast) ease;
}

.message-item.unread {
  border-color: color-mix(in srgb, var(--color-brand) 35%, transparent);
}

.message-item:hover {
  box-shadow: var(--shadow-md);
  transform: translateY(-1px);
}

.message-card {
  display: flex;
  flex-direction: column;
  gap: var(--sp-3);
  padding: var(--sp-4);
  color: inherit;
  text-decoration: none;
}

.card-head {
  display: flex;
  align-items: center;
  gap: var(--sp-3);
}

.avatar {
  width: 48px;
  height: 48px;
  border-radius: 50%;
  background: color-mix(in srgb, var(--color-brand) 15%, transparent);
  color: var(--color-brand);
  display: inline-flex;
  align-items: center;
  justify-content: center;
  font-weight: var(--fw-medium);
}

.sender-meta {
  flex: 1;
}

.sender {
  font-weight: var(--fw-medium);
  color: var(--color-text-1);
  margin: 0;
}

.time {
  font-size: var(--fz-caption);
  color: var(--color-text-3);
  margin: 0;
}

.status-pill {
  margin-left: auto;
  padding: var(--sp-1) var(--sp-3);
  border-radius: 999px;
  font-size: var(--fz-caption);
  color: var(--color-brand);
  background: color-mix(in srgb, var(--color-brand) 18%, transparent);
  font-weight: var(--fw-medium);
}

.content {
  margin: 0;
  color: var(--color-text-1);
  font-size: var(--fz-body);
  line-height: var(--lh-body);
  display: -webkit-box;
  line-clamp: 3;
  -webkit-line-clamp: 3;
  -webkit-box-orient: vertical;
  overflow: hidden;
}

.card-meta {
  display: flex;
  flex-wrap: wrap;
  gap: var(--sp-3);
  font-size: var(--fz-caption);
  color: var(--color-text-3);
}

.badge {
  padding: var(--sp-1) var(--sp-3);
  border-radius: var(--radius-md);
  background: var(--color-card-bg);
  border: 1px solid var(--color-border);
  font-size: var(--fz-caption);
}

.origin {
  display: inline-flex;
  align-items: center;
  gap: var(--sp-2);
}

.state-card {
  border: 1px dashed var(--color-border);
  border-radius: var(--radius-lg);
  padding: var(--sp-6);
  text-align: center;
  color: var(--color-text-2);
}

.state-card h3 {
  margin-bottom: var(--sp-2);
  color: var(--color-text-1);
}

.state-card.error {
  border-color: color-mix(in srgb, var(--color-danger) 40%, transparent);
  color: var(--color-danger);
}

.skeleton-list .message-item {
  display: flex;
  align-items: center;
  gap: var(--sp-3);
  padding: var(--sp-4);
  box-shadow: none;
}

.skeleton-text {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: var(--sp-2);
}

@media (max-width: 768px) {
  .messages-page {
    padding: var(--sp-5) var(--sp-3);
  }

  .messages-hero {
    padding: var(--sp-4);
  }

  .hero-actions {
    width: 100%;
    justify-content: space-between;
  }

  .messages-panel {
    padding: var(--sp-4);
  }
}

@media (prefers-color-scheme: dark) {
  .messages-hero {
    background: color-mix(in srgb, var(--color-brand) 18%, var(--color-background));
  }

  .avatar {
    background: color-mix(in srgb, var(--color-brand) 35%, transparent);
  }

  .badge {
    background: var(--color-background);
  }
}

@media (prefers-reduced-motion: reduce) {
  .message-item,
  .primary-btn,
  .filter-chip {
    transition: none;
  }
}
</style>
