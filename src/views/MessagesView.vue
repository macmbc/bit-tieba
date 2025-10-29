<template>
  <div class="message">
    <h1>我的消息</h1>
    <section class="section">
      <h2>消息列表</h2>
      <div class="filter">
        <button :class="{ active: filter === 'all' }" @click="filter = 'all'">全部</button>
        <button :class="{ active: filter === 'post_reply' }" @click="filter = 'post_reply'">
          帖子回复
        </button>
        <button :class="{ active: filter === 'comment_reply' }" @click="filter = 'comment_reply'">
          评论回复
        </button>
        <button :class="{ active: filter === 'mention' }" @click="filter = 'mention'">@我</button>
        <button :class="{ active: filter === 'system' }" @click="filter = 'system'">
          系统通知
        </button>
        <button
          :class="{ active: filter === 'private_message' }"
          @click="filter = 'private_message'"
        >
          私信
        </button>
        <button :class="{ active: filter === 'like_collect' }" @click="filter = 'like_collect'">
          点赞收藏
        </button>
        <!-- 新增批量标记已读按钮 -->
        <button
          v-show="filteredMessages.some((msg) => !msg.isRead) || markingAll"
          class="mark-all-read"
          @click="markAllAsRead"
          :disabled="markingAll"
        >
          {{ markingAll ? '标记中...' : '全部标记为已读' }}
        </button>
      </div>
      <div v-if="loading">加载中...</div>
      <div v-else-if="error" class="error">加载失败：{{ error }}</div>
      <div v-else-if="!filteredMessages.length" class="empty">暂无消息</div>
      <ul v-else class="message-list">
        <li
          v-for="message in filteredMessages"
          :key="message.id"
          class="message-item"
          :class="{ unread: !message.isRead }"
        >
          <router-link :to="message.link" @click="markAsRead(message.id)">
            <div class="message-header">
              <span class="sender">{{ message.sender }}</span>
              <span class="time">{{ formatTime(message.timestamp) }}</span>
            </div>
            <p class="content">{{ message.content.substring(0, 50) }}...</p>
            <div class="meta">
              <span>来自: {{ message.source }}</span>
              <span>{{ messageTypeText(message.type) }}</span>
            </div>
          </router-link>
        </li>
      </ul>
    </section>
  </div>
</template>

<script lang="ts" setup>
import { ref, onMounted, computed, nextTick } from 'vue'
import { useUserStore, useMessageStore } from '@/stores/user'
import { getMessages, markMessageAsRead, markAllMessagesAsRead } from '@/api/forumApi'
import type { Message } from '@/types'
import { formatTime } from '@/utils/format'

const userStore = useUserStore()
const messageStore = useMessageStore()
const messages = ref<Message[]>([])
const loading = ref(true)
const error = ref('')
const filter = ref<Message['type'] | 'all'>('all')
const markingAll = ref(false)

const filteredMessages = computed(() => {
  return filter.value === 'all'
    ? messages.value
    : messages.value.filter((msg) => msg.type === filter.value)
})

// 消息列表
onMounted(async () => {
  if (!userStore.isLoggedIn) {
    error.value = '请先登录以查看消息'
    loading.value = false
    return
  }

  try {
    loading.value = true
    messages.value = await getMessages(userStore.userId)
  } catch (err) {
    error.value = (err as Error).message
  } finally {
    loading.value = false
  }
})

// 消息类型文本
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
      return ''
  }
}

// 标记单条消息为已读
const markAsRead = async (messageId: number) => {
  const msgIndex = messages.value.findIndex((m) => m.id === messageId)
  const msg = messages.value[msgIndex]
  if (!msg) return
  msg.isRead = true // 乐观更新
  try {
    await markMessageAsRead(userStore.userId!, messageId)
    await messageStore.refreshUnreadCount(userStore.userId!)
  } catch {
    msg.isRead = false // 出错回滚
  }
}

// 批量标记为已读
const markAllAsRead = async () => {
  if (!userStore.isLoggedIn || !userStore.userId) return
  markingAll.value = true
  await nextTick()
  // 乐观更新：先标记筛选出的未读消息为已读
  const originalMessages = [...messages.value]
  try {
    messages.value = messages.value.map((msg) =>
      (filter.value === 'all' || msg.type === filter.value) && !msg.isRead
        ? { ...msg, isRead: true }
        : msg,
    )
    // 调用 API
    await markAllMessagesAsRead(
      userStore.userId!,
      filter.value === 'all' ? undefined : filter.value,
    )

    // 更新未读消息数
    await messageStore.refreshUnreadCount(userStore.userId!)
  } catch (err) {
    error.value = `批量标记已读失败：${(err as Error).message}`
    messages.value = originalMessages // 出错回滚
  } finally {
    markingAll.value = false
  }
}
</script>

<style scoped>
.message {
  max-width: 1200px;
  margin: 0 auto;
  padding: 20px;
  padding-left: 100px;
}

h1 {
  font-size: 2rem;
  color: #333;
  margin-bottom: 20px;
}

.section {
  margin-bottom: 30px;
}

.section h2 {
  font-size: 1.5rem;
  color: #333;
  margin-bottom: 15px;
}

.filter {
  display: flex;
  gap: 10px;
  margin-bottom: 15px;
  flex-wrap: wrap;
}

.filter button {
  padding: 8px 12px;
  border: 1px solid #ddd;
  border-radius: 4px;
  background: #fff;
  cursor: pointer;
  font-size: 0.9rem;
}

.filter button.active {
  background: #4c91d9;
  color: white;
  border-color: #4c91d9;
}

.mark-all-read {
  background: #ff4500; /* 贴吧红色 */
  color: black;
  border: none;
  border-radius: 4px;
  padding: 8px 12px;
  cursor: pointer;
}

.mark-all-read:hover:not(:disabled) {
  background: #e63900;
  color: white;
}

.mark-all-read:disabled {
  background: #ccc;
  cursor: not-allowed;
}

.message-list {
  list-style: none;
  padding: 0;
}

.message-item {
  background: #fff;
  border: 1px solid #eee;
  border-radius: 6px;
  padding: 15px;
  margin-bottom: 10px;
  transition: box-shadow 0.2s ease;
}

.message-item:hover {
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.message-item.unread {
  background: #f0f8ff;
}

.message-item a {
  text-decoration: none;
  color: #333;
  display: block;
}

.message-header {
  display: flex;
  justify-content: space-between;
  margin-bottom: 8px;
}

.sender {
  font-weight: bold;
  color: #4c91d9;
}

.time {
  font-size: 0.85rem;
  color: #999;
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

.empty {
  color: #666;
  text-align: center;
  font-size: 1rem;
}

.error {
  color: red;
  text-align: center;
  font-size: 1rem;
}

@media (max-width: 576px) {
  .message {
    padding-left: 20px;
  }
}

@media (prefers-color-scheme: dark) {
  .message {
    background: #1a1a1a;
  }
  h1,
  .section h2 {
    color: #ddd;
  }
  .filter button {
    background: #222;
    border-color: #444;
    color: #ddd;
  }
  .filter button.active {
    background: #6ab0ff;
    border-color: #6ab0ff;
  }
  .mark-all-read {
    background: #ff6347;
  }
  .mark-all-read:hover:not(:disabled) {
    background: #e55337;
  }
  .mark-all-read:disabled {
    background: #666;
  }
  .message-item {
    background: #222;
    border-color: #444;
  }
  .message-item.unread {
    background: #2a3a4a;
  }
  .message-item a {
    color: #ddd;
  }
  .sender {
    color: #6ab0ff;
  }
  .content {
    color: #aaa;
  }
  .meta,
  .time,
  .empty {
    color: #aaa;
  }
}
</style>
