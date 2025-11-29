<template>
  <section class="section" aria-labelledby="followed-posts-heading">
    <div class="section-head">
      <h2 id="followed-posts-heading">关注动态</h2>
      <p class="section-subtitle">来自你关注吧的实时更新</p>
    </div>
    <p v-if="!posts.length" class="empty-state">关注一些吧，就能在这里看到新帖啦。</p>
    <ul v-else class="post-list">
      <li v-for="post in posts" :key="post.id" class="post-item">
        <router-link :to="`/forum/${post.forumId}/post/${post.id}`" class="card-link">
          <h3>{{ post.title }}</h3>
          <p class="excerpt">{{ post.content.substring(0, 70) }}...</p>
          <div class="post-meta">
            <span>{{ post.forumName }}</span>
            <span>回复 {{ post.replyCount }}</span>
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
  margin-bottom: var(--sp-6);
}

.section-head {
  display: flex;
  align-items: baseline;
  justify-content: space-between;
  gap: var(--sp-3);
  margin-bottom: var(--sp-4);
}

.section-head h2 {
  font-size: var(--fz-h3);
  font-weight: var(--fw-bold);
  color: var(--color-text-1);
  margin: 0;
}

.section-subtitle {
  font-size: var(--fz-sub);
  color: var(--color-text-3);
}

.empty-state {
  padding: var(--sp-5);
  border-radius: var(--radius-md);
  border: 1px dashed var(--color-border);
  background: var(--color-card-bg);
  color: var(--color-text-2);
  text-align: center;
}

.post-list {
  list-style: none;
  padding: 0;
  margin: 0;
  display: flex;
  flex-direction: column;
  gap: var(--sp-3);
}

.post-item {
  border: 1px solid var(--color-border);
  border-radius: var(--radius-md);
  background: var(--color-background);
  transition: box-shadow var(--ease-fast) ease, transform var(--ease-fast) ease;
}

.post-item:hover {
  box-shadow: var(--shadow-sm);
  transform: translateY(-1px);
}

.card-link {
  display: flex;
  flex-direction: column;
  gap: var(--sp-2);
  padding: var(--sp-4);
  text-decoration: none;
  color: var(--color-text-1);
}

.card-link h3 {
  margin: 0;
  font-size: var(--fz-body);
}

.excerpt {
  margin: 0;
  font-size: var(--fz-sub);
  color: var(--color-text-2);
}

.post-meta {
  display: flex;
  gap: var(--sp-4);
  font-size: var(--fz-caption);
  color: var(--color-text-3);
}

.post-meta span::before {
  content: '•';
  margin-right: var(--sp-2);
  color: var(--color-border-strong);
}

.post-meta span:first-child::before {
  content: '';
  margin-right: 0;
}

@media (max-width: 768px) {
  .section-head {
    flex-direction: column;
    align-items: flex-start;
  }
}
</style>
