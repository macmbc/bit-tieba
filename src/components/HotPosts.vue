<template>
  <section class="section" aria-labelledby="hot-posts-heading">
    <div class="section-head">
      <h2 id="hot-posts-heading">推荐帖子</h2>
      <p class="section-subtitle">热门讨论，实时刷新</p>
    </div>
    <p v-if="!posts.length" class="empty-state">暂无推荐帖子，稍后再来看看。</p>
    <ul v-else class="post-list">
      <li v-for="post in posts" :key="post.id" class="post-item">
        <router-link :to="`/forum/${post.forumId}/post/${post.id}`" class="card-link">
          <div class="post-head">
            <h3>{{ post.title }}</h3>
            <span class="badge">回复 {{ post.replyCount }}</span>
          </div>
          <p class="excerpt">{{ post.content.substring(0, 90) }}...</p>
          <div class="post-meta">
            <span>{{ post.forumName }}</span>
            <span>发布 · {{ formatTime(post.createdAt) }}</span>
          </div>
        </router-link>
      </li>
    </ul>
  </section>
</template>

<script lang="ts" setup>
import type { Post } from '@/types'
import { formatTime } from '@/utils/format'
defineProps<{
  posts: Post[]
}>()
</script>

<style scoped>
.section {
  margin-bottom: var(--sp-6);
  padding: var(--sp-4);
  background: var(--color-card-bg);
  border-radius: var(--radius-lg);
  border: 1px solid var(--color-border);
}

.section-head {
  display: flex;
  align-items: baseline;
  justify-content: space-between;
  gap: var(--sp-3);
  margin-bottom: var(--sp-4);
}

.section-head h2 {
  font-size: var(--fz-h2);
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
  text-align: center;
  color: var(--color-text-2);
  background: var(--color-background);
  border-radius: var(--radius-md);
  border: 1px dashed var(--color-border);
}

.post-list {
  list-style: none;
  padding: 0;
  margin: 0;
  display: flex;
  flex-direction: column;
  gap: var(--sp-4);
}

.post-item {
  border-radius: var(--radius-md);
  background: var(--color-background);
  border: 1px solid var(--color-border);
  transition: box-shadow var(--ease-fast) ease, transform var(--ease-fast) ease;
}

.post-item:hover {
  box-shadow: var(--shadow-sm);
  transform: translateY(-2px);
}

.card-link {
  display: flex;
  flex-direction: column;
  gap: var(--sp-3);
  text-decoration: none;
  color: var(--color-text-1);
  padding: var(--sp-4);
}

.post-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: var(--sp-3);
}

.post-head h3 {
  font-size: var(--fz-h3);
  margin: 0;
}

.badge {
  padding: 0 var(--sp-2);
  border-radius: var(--radius-sm);
  background: color-mix(in srgb, var(--color-brand) 12%, transparent);
  color: var(--color-brand);
  font-size: var(--fz-caption);
  font-weight: var(--fw-medium);
}

.excerpt {
  font-size: var(--fz-body);
  line-height: var(--lh-body);
  color: var(--color-text-2);
  margin: 0;
}

.post-meta {
  display: flex;
  gap: var(--sp-4);
  font-size: var(--fz-sub);
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
  .section {
    padding: var(--sp-3);
  }

  .post-head {
    flex-direction: column;
    align-items: flex-start;
  }
}
</style>
