<template>
  <section class="section">
    <h2>推荐帖子</h2>
    <ul class="post-list">
      <li v-for="post in posts" :key="post.id" class="post-item">
        <router-link :to="`/forum/${post.forumId}/post/${post.id}`">
          <h3>{{ post.title }}</h3>
          <p>{{ post.content.substring(0, 50) }}...</p>
          <div class="post-meta">
            <span>来自: {{ post.forumName }}</span>
            <span>回复: {{ post.replyCount }}</span>
          </div>
        </router-link>
      </li>
    </ul>
  </section>
</template>

<script lang="ts" setup>
import type { Post } from '@/types'
defineProps<{
  posts: Post[]
}>()
</script>

<style scoped>
.section {
  margin-bottom: 30px;
}

.section h2 {
  font-size: 1.5rem;
  color: #333;
  margin-bottom: 15px;
}

.post-list {
  list-style: none;
  padding: 0;
}

.post-item {
  background: #fff;
  border: 1px solid #eee;
  border-radius: 6px;
  padding: 15px;
  margin-bottom: 10px;
  transition: box-shadow 0.2s ease;
}

.post-item:hover {
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.post-item a {
  text-decoration: none;
  color: #333;
}

.post-item h3 {
  font-size: 1.1rem;
  margin: 0 0 10px;
  color: #4c91d9;
}

.post-item p {
  font-size: 0.9rem;
  color: #666;
  margin: 0 0 10px;
}

.post-meta {
  display: flex;
  gap: 15px;
  font-size: 0.85rem;
  color: #999;
}

/* 暗模式 */
@media (prefers-color-scheme: dark) {
  .section h2 {
    color: #ddd;
  }
  .post-item {
    background: #222;
    border-color: #444;
  }
  .post-item a {
    color: #ddd;
  }
  .post-item h3 {
    color: #6ab0ff;
  }
  .post-item p,
  .post-meta {
    color: #aaa;
  }
}
</style>
