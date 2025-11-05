<template>
  <div class="me">
    <h1>我的</h1>

    <!-- 个人信息 -->
    <section class="section profile">
      <h2>个人信息</h2>
      <div v-if="loading.profile" class="loading">加载中...</div>
      <div v-else-if="error.profile" class="error">{{ error.profile }}</div>
      <div v-else class="profile-content">
        <img :src="userInfo.avatar || defaultAvatar" alt="头像" class="avatar" />
        <div class="info">
          <h3>{{ userInfo.username || '未设置用户名' }}</h3>
          <p>{{ userInfo.bio || '暂无简介' }}</p>
          <p>注册时间: {{ formatTime(userInfo.createdAt) }}</p>
        </div>
        <router-link to="/settings" class="edit-btn">编辑资料</router-link>
      </div>
    </section>

    <!-- 我的帖子 -->
    <section class="section posts">
      <h2>我的帖子</h2>
      <div v-if="loading.posts" class="loading">加载中...</div>
      <div v-else-if="error.posts" class="error">{{ error.posts }}</div>
      <div v-else-if="!posts.length" class="empty">你还没有发布帖子</div>
      <ul v-else class="post-list">
        <li v-for="post in posts" :key="post.id" class="post-item">
          <router-link :to="`/forum/${post.forumId}/post/${post.id}`">
            <h3>{{ post.title }}</h3>
            <p>{{ post.content.substring(0, 50) }}...</p>
            <div class="meta">
              <span>来自: {{ post.forumName }}</span>
              <span>回复: {{ post.replyCount }}</span>
            </div>
          </router-link>
        </li>
      </ul>
    </section>

    <!-- 我的关注吧 -->
    <section class="section forums">
      <h2>我的关注吧</h2>
      <div v-if="loading.forums" class="loading">加载中...</div>
      <div v-else-if="error.forums" class="error">{{ error.forums }}</div>
      <div v-else-if="!followedForums.length" class="empty">你还没有关注任何吧</div>
      <ul v-else class="forum-list">
        <li v-for="forum in followedForums" :key="forum.id" class="forum-item">
          <router-link :to="`/forum/${forum.id}`">
            <h3>{{ forum.name }}</h3>
            <p>{{ forum.description }}</p>
            <span>帖子数: {{ forum.postCount }}</span>
          </router-link>
        </li>
      </ul>
    </section>

    <!-- 我的消息概览 -->
    <section class="section messages">
      <h2>最近消息 <router-link to="/message" class="view-all">查看全部</router-link></h2>
      <div v-if="loading.messages" class="loading">加载中...</div>
      <div v-else-if="error.messages" class="error">{{ error.messages }}</div>
      <div v-else-if="!messages.length" class="empty">暂无消息</div>
      <ul v-else class="message-list">
        <li
          v-for="message in messages.slice(0, 3)"
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
import { ref, onMounted } from 'vue'
import { useUserStore, useMessageStore } from '@/stores/user'
import {
  getUserInfo,
  getUserPosts,
  getFollowedForums,
  getMessages,
  markMessageAsRead,
} from '@/api/forumApi'
import type { UserInfo, Post, Forum, Message } from '@/types'
import defaultAvatar from '@/assets/default.png'
import { formatTime } from '@/utils/format'

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
  if (!userStore.isLoggedIn) {
    error.value.profile = '请先登录以查看个人资料'
    loading.value.profile = false
    loading.value.posts = false
    loading.value.forums = false
    loading.value.messages = false
    return
  }

  try {
    const [info, userPosts, forums, userMessages] = await Promise.all([
      getUserInfo(userStore.userId!).catch(() => ({}) as UserInfo),
      getUserPosts(userStore.userId!).catch(() => [] as Post[]),
      getFollowedForums(userStore.userId!).catch(() => [] as Forum[]),
      getMessages(userStore.userId!).catch(() => [] as Message[]),
    ])
    userInfo.value = info
    posts.value = userPosts
    followedForums.value = forums
    messages.value = userMessages
  } catch (err) {
    error.value.profile =
      error.value.posts =
      error.value.forums =
      error.value.messages =
        (err as Error).message
  } finally {
    loading.value.profile = false
    loading.value.posts = false
    loading.value.forums = false
    loading.value.messages = false
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
</script>

<style scoped>
.me {
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
  position: relative;
}

.view-all {
  position: absolute;
  right: 0;
  font-size: 0.9rem;
  color: #4c91d9;
  text-decoration: none;
}

.view-all:hover {
  text-decoration: underline;
}

.profile-content {
  display: flex;
  align-items: center;
  gap: 20px;
  background: #fff;
  border: 1px solid #eee;
  border-radius: 6px;
  padding: 15px;
}

.avatar {
  width: 80px;
  height: 80px;
  border-radius: 50%;
  object-fit: cover;
}

.info {
  flex: 1;
}

.info h3 {
  font-size: 1.3rem;
  margin: 0 0 10px;
}

.info p {
  font-size: 0.9rem;
  color: #666;
  margin: 0 0 5px;
}

.edit-btn {
  background: #4c91d9;
  color: white;
  border: none;
  border-radius: 4px;
  padding: 8px 12px;
  text-decoration: none;
  font-size: 0.9rem;
}

.edit-btn:hover {
  background: #3a7ab8;
}

.post-list,
.forum-list,
.message-list {
  list-style: none;
  padding: 0;
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(250px, 1fr));
  gap: 15px;
}

.post-item,
.forum-item,
.message-item {
  background: #fff;
  border: 1px solid #eee;
  border-radius: 6px;
  padding: 15px;
  transition: box-shadow 0.2s ease;
}

.post-item:hover,
.forum-item:hover,
.message-item:hover {
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.post-item a,
.forum-item a,
.message-item a {
  text-decoration: none;
  color: #333;
  display: block;
}

.post-item h3,
.forum-item h3 {
  font-size: 1.2rem;
  margin: 0 0 10px;
  color: #4c91d9;
}

.post-item p,
.forum-item p {
  font-size: 0.9rem;
  color: #666;
  margin: 0 0 10px;
}

.post-item .meta,
.forum-item span,
.message-item .meta {
  font-size: 0.85rem;
  color: #999;
}

.message-item.unread {
  background: #f0f8ff;
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
}

.loading,
.empty,
.error {
  color: #666;
  text-align: center;
  font-size: 1rem;
}

.error {
  color: red;
}

@media (max-width: 576px) {
  .me {
    padding-left: 20px;
  }
  .profile-content {
    flex-direction: column;
    align-items: flex-start;
  }
  .post-list,
  .forum-list,
  .message-list {
    grid-template-columns: 1fr;
  }
}

@media (prefers-color-scheme: dark) {
  .me {
    background: #1a1a1a;
  }
  h1,
  .section h2 {
    color: #ddd;
  }
  .profile-content,
  .post-item,
  .forum-item,
  .message-item {
    background: #222;
    border-color: #444;
  }
  .post-item a,
  .forum-item a,
  .message-item a {
    color: #ddd;
  }
  .post-item h3,
  .forum-item h3,
  .sender {
    color: #6ab0ff;
  }
  .post-item p,
  .forum-item p,
  .content {
    color: #aaa;
  }
  .meta,
  .time,
  .post-item span,
  .forum-item span,
  .loading,
  .empty {
    color: #aaa;
  }
  .edit-btn {
    background: #6ab0ff;
  }
  .edit-btn:hover {
    background: #5a9be6;
  }
  .view-all {
    color: #6ab0ff;
  }
  .message-item.unread {
    background: #2a3a4a;
  }
}
</style>
