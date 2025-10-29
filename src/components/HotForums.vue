<template>
  <section class="section">
    <h2>热门吧</h2>
    <ul class="forum-list">
      <li v-for="forum in forums" :key="forum.id" class="forum-item">
        <router-link :to="`/forum/${forum.id}`">
          <h3>{{ forum.name }}</h3>
          <p>{{ forum.description }}</p>
          <span>帖子数: {{ forum.postCount }}</span>
        </router-link>
      </li>
    </ul>
  </section>
</template>

<script lang="ts" setup>
import type { Forum } from '@/types'
defineProps<{
  forums: Forum[]
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

.forum-list {
  list-style: none;
  padding: 0;
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(250px, 1fr));
  gap: 15px;
}

.forum-item {
  background: #fff;
  border: 1px solid #eee;
  border-radius: 6px;
  padding: 15px;
  transition: box-shadow 0.2s ease;
}

.forum-item:hover {
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.forum-item a {
  text-decoration: none;
  color: #333;
}

.forum-item h3 {
  font-size: 1.2rem;
  margin: 0 0 10px;
  color: #4c91d9;
}

.forum-item p {
  font-size: 0.9rem;
  color: #666;
  margin: 0 0 10px;
}

.forum-item span {
  font-size: 0.85rem;
  color: #999;
}

/* 响应式 */
@media (max-width: 576px) {
  .forum-list {
    grid-template-columns: 1fr;
  }
}

/* 暗模式 */
@media (prefers-color-scheme: dark) {
  .section h2 {
    color: #ddd;
  }
  .forum-item {
    background: #222;
    border-color: #444;
  }
  .forum-item a {
    color: #ddd;
  }
  .forum-item h3 {
    color: #6ab0ff;
  }
  .forum-item p {
    color: #aaa;
  }
}
</style>
