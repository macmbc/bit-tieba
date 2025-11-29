<template>
  <div class="me-page">
    <section class="profile-hero">
      <div class="profile-main">
        <img :src="userInfo.avatar || defaultAvatar" alt="头像" class="avatar" />
        <div class="profile-copy">
          <p class="eyebrow">个人中心</p>
          <h1>{{ userInfo.username || '未设置用户名' }}</h1>
          <p class="bio">{{ userInfo.bio || '这个人很懒，什么都没写~' }}</p>
          <div class="meta-row">
            <span>加入于 {{ formatTime(userInfo.createdAt) }}</span>
            <span v-if="authStore.user?.email">邮箱 {{ authStore.user?.email }}</span>
          </div>
        </div>
      </div>
      <div class="profile-actions">
        <router-link to="/settings" class="solid-btn">编辑资料</router-link>
        <button type="button" class="ghost-btn" @click="router.push('/message')">消息中心</button>
        <button type="button" class="danger-btn" @click="handleLogout">退出登录</button>
      </div>
    </section>

    <section class="stats-grid" aria-label="个人概览">
      <article v-for="card in statCards" :key="card.label" class="stat-card">
        <p class="label">{{ card.label }}</p>
        <p class="value">{{ card.value }}</p>
        <p class="hint">{{ card.hint }}</p>
      </article>
    </section>

    <div class="content-grid">
      <section class="panel posts-panel">
        <header class="panel-header">
          <div>
            <h2>我的帖子</h2>
            <p>追踪你在各个吧内的创作表现</p>
          </div>
        </header>
        <div v-if="loading.posts" class="state-block">加载中...</div>
        <div v-else-if="error.posts" class="state-block error">{{ error.posts }}</div>
        <div v-else-if="!posts.length" class="state-block empty">你还没有发布帖子</div>
        <ul v-else class="post-list">
          <li v-for="post in posts" :key="post.id" class="post-item">
            <router-link :to="`/forum/${post.forumId}/post/${post.id}`">
              <p class="post-forum">{{ post.forumName }}</p>
              <h3>{{ post.title }}</h3>
              <p class="post-preview">{{ post.content.substring(0, 80) }}...</p>
              <div class="post-meta">
                <span>{{ formatTime(post.createdAt) }}</span>
                <span>回复 {{ post.replyCount }}</span>
              </div>
            </router-link>
          </li>
        </ul>
      </section>

      <section class="panel forums-panel">
        <header class="panel-header">
          <div>
            <h2>关注的吧</h2>
            <p>常逛社区概况</p>
          </div>
        </header>
        <div v-if="loading.forums" class="state-block">加载中...</div>
        <div v-else-if="error.forums" class="state-block error">{{ error.forums }}</div>
        <div v-else-if="!followedForums.length" class="state-block empty">你还没有关注任何吧</div>
        <ul v-else class="forum-list">
          <li v-for="forum in followedForums" :key="forum.id" class="forum-item">
            <router-link :to="`/forum/${forum.id}`">
              <div>
                <h3>{{ forum.name }}</h3>
                <p>{{ forum.description }}</p>
              </div>
              <span>帖子 {{ forum.postCount }}</span>
            </router-link>
          </li>
        </ul>
      </section>

      <section class="panel messages-panel">
        <header class="panel-header">
          <div>
            <h2>最近消息</h2>
            <p>最新互动提醒</p>
          </div>
          <router-link to="/message" class="ghost-btn">查看全部</router-link>
        </header>
        <div v-if="loading.messages" class="state-block">加载中...</div>
        <div v-else-if="error.messages" class="state-block error">{{ error.messages }}</div>
        <div v-else-if="!messages.length" class="state-block empty">暂无消息</div>
        <ul v-else class="message-list">
          <li
            v-for="message in limitedMessages"
            :key="message.id"
            class="message-item"
            :class="{ unread: !message.isRead }"
          >
            <router-link :to="message.link" @click="markAsRead(message.id)">
              <div class="message-top">
                <span class="sender">{{ message.sender }}</span>
                <span class="time">{{ formatTime(message.timestamp) }}</span>
              </div>
              <p class="message-content">{{ message.content.substring(0, 60) }}...</p>
              <div class="message-meta">
                <span>{{ message.source }}</span>
                <span>{{ messageTypeText(message.type) }}</span>
              </div>
            </router-link>
          </li>
        </ul>
      </section>
    </div>
  </div>
</template>

<script lang="ts" setup>
import { ref, onMounted, computed } from 'vue'
import { useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/authStore'
import { useUserStore, useMessageStore } from '@/stores/user'
import {
  getUserPosts,
  getFollowedForums,
  getMessages,
  markMessageAsRead,
} from '@/api/forumApi'
import type { UserInfo, Post, Forum, Message } from '@/types'
import defaultAvatar from '@/assets/default.png'
import { formatTime } from '@/utils/format'

const router = useRouter()
const authStore = useAuthStore()
const userStore = useUserStore()
const messageStore = useMessageStore()
const userInfo = ref<UserInfo>({ username: '', bio: '', createdAt: 0 })
const posts = ref<Post[]>([])
const followedForums = ref<Forum[]>([])
const messages = ref<Message[]>([])

const loading = ref({
  profile: true,
  posts: true,
  forums: true,
  messages: true,
})
const error = ref({
  profile: '',
  posts: '',
  forums: '',
  messages: '',
})

// 加载数据
onMounted(async () => {
  // 使用真实的登录用户信息
  const authStore = useAuthStore()

  if (!authStore.isLoggedIn) {
    error.value.profile = '请先登录以查看个人资料'
    loading.value.profile = false
    loading.value.posts = false
    loading.value.forums = false
    loading.value.messages = false
    return
  }

  // 从 authStore 获取真实用户信息
  userInfo.value = {
    username: authStore.user?.email?.split('@')[0] || '用户', // 用邮箱前缀作为用户名
    bio: '这个人很懒，什么都没写~',
    createdAt: Date.now(), // 或者从后端获取
    avatar: '', // 如果有头像URL
  }
  loading.value.profile = false

  if (!userStore.isLoggedIn) {
    error.value.profile = '请先登录以查看个人资料'
    loading.value.profile = false
    loading.value.posts = false
    loading.value.forums = false
    loading.value.messages = false
    return
  }

  try {
    const [userPosts, forums, userMessages] = await Promise.all([
      getUserPosts(userStore.userId!).catch(() => [] as Post[]),
      getFollowedForums(userStore.userId!).catch(() => [] as Forum[]),
      getMessages(userStore.userId!).catch(() => [] as Message[]),
    ])
    posts.value = userPosts
    followedForums.value = forums
    messages.value = userMessages
  } catch (err) {
    console.error(err)
  } finally {
    loading.value.posts = false
    loading.value.forums = false
    loading.value.messages = false
  }
})

const limitedMessages = computed(() => messages.value.slice(0, 4))

const unreadMessages = computed(() => messages.value.filter((m) => !m.isRead).length)

const memberDays = computed(() => {
  if (!userInfo.value.createdAt) return 0
  const diff = Date.now() - userInfo.value.createdAt
  return Math.max(1, Math.round(diff / (1000 * 60 * 60 * 24)))
})

const statCards = computed(() => [
  {
    label: '发帖数',
    value: loading.value.posts ? '--' : posts.value.length,
    hint: posts.value.length ? `最新帖子收到 ${posts.value[0]?.replyCount ?? 0} 条互动` : '快去发布你的第一条帖子',
  },
  {
    label: '关注的吧',
    value: loading.value.forums ? '--' : followedForums.value.length,
    hint: followedForums.value.length ? '保持活跃可以获得更多推荐' : '去发现更多感兴趣的社区',
  },
  {
    label: '未读消息',
    value: loading.value.messages ? '--' : unreadMessages.value,
    hint: unreadMessages.value ? '及时回复以保持互动热度' : '所有消息都已处理',
  },
  {
    label: '活跃天数',
    value: memberDays.value || '--',
    hint: memberDays.value ? '坚持打卡更容易获得等级徽章' : '完善资料后解锁更多能力',
  },
])

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

// 标记消息为已读
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

// 在最后添加退出登录函数
const handleLogout = () => {
  if (confirm('确定要退出登录吗？')) {
    authStore.logout()
    router.push('/login')
  }
}
</script>

<style scoped>
.me-page {
  max-width: var(--layout-heart-width);
  margin: 0 auto;
  padding: var(--sp-8) var(--sp-6) var(--sp-10);
  display: flex;
  flex-direction: column;
  gap: var(--sp-6);
}

.profile-hero {
  background: linear-gradient(135deg, rgba(25, 118, 210, 0.12), rgba(25, 118, 210, 0.04));
  border-radius: var(--radius-lg);
  padding: var(--sp-6);
  display: flex;
  justify-content: space-between;
  gap: var(--sp-6);
  border: 1px solid rgba(25, 118, 210, 0.15);
}

.profile-main {
  display: flex;
  gap: var(--sp-5);
  align-items: center;
}

.avatar {
  width: 96px;
  height: 96px;
  border-radius: 50%;
  object-fit: cover;
  border: 3px solid rgba(255, 255, 255, 0.9);
  box-shadow: var(--shadow-sm);
}

.profile-copy h1 {
  font-size: var(--fz-h1);
  margin-bottom: var(--sp-2);
}

.bio {
  color: var(--color-text-2);
  font-size: var(--fz-body);
}

.eyebrow {
  font-size: var(--fz-sub);
  text-transform: uppercase;
  letter-spacing: 0.08em;
  color: var(--color-brand);
  margin-bottom: var(--sp-1);
}

.meta-row {
  display: flex;
  flex-wrap: wrap;
  gap: var(--sp-3);
  margin-top: var(--sp-3);
  color: var(--color-text-3);
  font-size: var(--fz-sub);
}

.profile-actions {
  display: flex;
  gap: var(--sp-3);
  align-items: flex-start;
}

.solid-btn,
.ghost-btn,
.danger-btn {
  border-radius: var(--radius-md);
  padding: var(--sp-2) var(--sp-4);
  font-size: var(--fz-sub);
  font-weight: var(--fw-medium);
  border: 1px solid transparent;
  cursor: pointer;
  transition: background var(--ease-fast), color var(--ease-fast), border var(--ease-fast);
  text-decoration: none;
}

.solid-btn {
  background: var(--color-brand);
  color: #fff;
}

.solid-btn:hover {
  background: var(--color-brand-hover);
}

.ghost-btn {
  border-color: rgba(25, 118, 210, 0.4);
  color: var(--color-brand);
  background: transparent;
}

.ghost-btn:hover {
  border-color: var(--color-brand);
}

.danger-btn {
  background: rgba(244, 67, 54, 0.1);
  border-color: rgba(244, 67, 54, 0.2);
  color: #f44336;
}

.danger-btn:hover {
  background: rgba(244, 67, 54, 0.18);
}

.stats-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
  gap: var(--sp-4);
}

.stat-card {
  background: var(--color-card-bg);
  border-radius: var(--radius-lg);
  padding: var(--sp-4);
  border: 1px solid var(--color-border);
  box-shadow: var(--shadow-sm);
}

.stat-card .label {
  font-size: var(--fz-sub);
  color: var(--color-text-3);
  margin-bottom: var(--sp-2);
  text-transform: uppercase;
}

.stat-card .value {
  font-size: 32px;
  font-weight: var(--fw-bold);
  margin-bottom: var(--sp-2);
}

.stat-card .hint {
  color: var(--color-text-2);
  font-size: var(--fz-sub);
}

.content-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
  gap: var(--sp-5);
}

.panel {
  background: var(--color-background);
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  padding: var(--sp-5);
  box-shadow: var(--shadow-sm);
}

.panel-header {
  display: flex;
  justify-content: space-between;
  gap: var(--sp-4);
  margin-bottom: var(--sp-4);
}

.panel-header h2 {
  font-size: var(--fz-h3);
  margin-bottom: var(--sp-1);
}

.panel-header p {
  color: var(--color-text-2);
  font-size: var(--fz-sub);
}

.state-block {
  text-align: center;
  padding: var(--sp-5);
  color: var(--color-text-2);
  border-radius: var(--radius-md);
  background: var(--color-background-soft);
}

.state-block.error {
  color: var(--color-danger);
}

.state-block.empty {
  color: var(--color-text-3);
}

.post-list,
.forum-list,
.message-list {
  list-style: none;
  margin: 0;
  padding: 0;
  display: flex;
  flex-direction: column;
  gap: var(--sp-3);
}

.post-item,
.forum-item,
.message-item {
  border: 1px solid var(--color-border);
  border-radius: var(--radius-md);
  transition: border var(--ease-fast), box-shadow var(--ease-fast);
  background: var(--color-background);
}

.post-item a,
.forum-item a,
.message-item a {
  display: block;
  padding: var(--sp-4);
  text-decoration: none;
  color: inherit;
}

.post-item:hover,
.forum-item:hover,
.message-item:hover {
  border-color: var(--color-border-hover);
  box-shadow: var(--shadow-sm);
}

.post-forum {
  font-size: var(--fz-sub);
  color: var(--color-text-3);
  text-transform: uppercase;
  letter-spacing: 0.08em;
}

.post-item h3 {
  margin: var(--sp-2) 0;
  font-size: var(--fz-h3);
}

.post-preview {
  color: var(--color-text-2);
  font-size: var(--fz-body);
}

.post-meta {
  display: flex;
  justify-content: space-between;
  margin-top: var(--sp-3);
  color: var(--color-text-3);
  font-size: var(--fz-sub);
}

.forum-item a {
  display: flex;
  justify-content: space-between;
  gap: var(--sp-4);
  align-items: center;
}

.forum-item h3 {
  font-size: var(--fz-h3);
  margin-bottom: var(--sp-2);
}

.forum-item p {
  color: var(--color-text-2);
  font-size: var(--fz-body);
}

.message-top {
  display: flex;
  justify-content: space-between;
  gap: var(--sp-3);
  margin-bottom: var(--sp-2);
}

.sender {
  font-weight: var(--fw-medium);
  color: var(--color-brand);
}

.time {
  font-size: var(--fz-sub);
  color: var(--color-text-3);
}

.message-content {
  color: var(--color-text-2);
  margin-bottom: var(--sp-2);
}

.message-meta {
  display: flex;
  justify-content: space-between;
  font-size: var(--fz-sub);
  color: var(--color-text-3);
}

.message-item.unread {
  border-color: rgba(25, 118, 210, 0.35);
  background: rgba(25, 118, 210, 0.04);
}

@media (max-width: 960px) {
  .profile-hero {
    flex-direction: column;
  }
  .profile-actions {
    flex-wrap: wrap;
  }
  .content-grid {
    grid-template-columns: 1fr;
  }
}

@media (prefers-color-scheme: dark) {
  .profile-hero {
    border-color: rgba(255, 255, 255, 0.08);
  }
  .stat-card,
  .panel,
  .post-item,
  .forum-item,
  .message-item {
    background: var(--color-background-soft);
  }
  .avatar {
    border-color: rgba(0, 0, 0, 0.4);
  }
}
</style>
