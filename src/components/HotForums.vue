<template>
  <section class="section" aria-labelledby="hot-forums-heading">
    <div class="section-head">
      <h2 id="hot-forums-heading">热门吧</h2>
      <p class="section-subtitle">大家都在讨论的版块</p>
    </div>
    <p v-if="!forums.length" class="empty-state">暂无热门吧，稍后再试。</p>
    <ul v-else class="forum-list">
      <li v-for="forum in forums" :key="forum.id" class="forum-item">
        <router-link :to="`/forum/${forum.id}`" class="card-link">
          <div class="forum-head">
            <span
              class="logo"
              aria-hidden="true"
              :style="{ backgroundImage: `url(${getForumLogo(forum.avatar)})` }"
            ></span>
            <div class="info">
              <h3>{{ forum.name }}</h3>
              <p>{{ forum.description }}</p>
            </div>
          </div>
          <div class="forum-meta">
            <span>帖子 {{ forum.postCount }}</span>
            <span>关注 {{ forum.followerCount }}</span>
          </div>
        </router-link>
      </li>
    </ul>
  </section>
</template>

<script lang="ts" setup>
import { getForumLogo } from '@/utils/load_image'

import type { Forum } from '@/types'
defineProps<{
  forums: Forum[]
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
  border: 1px dashed var(--color-border);
  border-radius: var(--radius-md);
  text-align: center;
  color: var(--color-text-2);
  background: var(--color-card-bg);
}

.forum-list {
  list-style: none;
  padding: 0;
  margin: 0;
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(260px, 1fr));
  gap: var(--sp-4);
}

.forum-item {
  border: 1px solid var(--color-border);
  border-radius: var(--radius-md);
  background: var(--color-background);
  transition:
    box-shadow var(--ease-fast) ease,
    transform var(--ease-fast) ease;
}

.forum-item:hover {
  box-shadow: var(--shadow-sm);
  transform: translateY(-2px);
}

.card-link {
  display: flex;
  flex-direction: column;
  gap: var(--sp-3);
  padding: var(--sp-4);
  text-decoration: none;
  color: var(--color-text-1);
}

.forum-head {
  display: flex;
  gap: var(--sp-3);
  align-items: center;
}

.logo {
  width: 40px;
  height: 40px;
  border-radius: var(--radius-md);
  background: color-mix(in srgb, var(--color-brand) 12%, transparent);
  color: var(--color-brand);
  display: inline-flex;
  align-items: center;
  justify-content: center;
  font-weight: var(--fw-bold);
  /* background-image: url("../assets/forums/default.png"); */
  background-size: cover;
}

.info h3 {
  margin: 0;
  font-size: var(--fz-body);
}

.info p {
  margin: 0;
  font-size: var(--fz-sub);
  color: var(--color-text-2);
}

.forum-meta {
  display: flex;
  justify-content: space-between;
  font-size: var(--fz-caption);
  color: var(--color-text-3);
}

.forum-meta span::before {
  content: '•';
  margin-right: var(--sp-2);
  color: var(--color-border-strong);
}

.forum-meta span:first-child::before {
  content: '';
  margin-right: 0;
}

@media (max-width: 576px) {
  .forum-meta {
    flex-direction: column;
    gap: var(--sp-1);
  }
}
</style>
