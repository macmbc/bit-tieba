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
          <div class="post-content">
            <div class="post-head">
              <h3>{{ post.title }}</h3>
              <span class="badge">回复 {{ post.replyCount }}</span>
            </div>

            <p class="excerpt">{{ post.content.substring(0, 90) }}...</p>

            <!-- 优化post-meta的布局结构，确保“作者”与其他元素对齐 -->
            <div class="post-meta">
              <!-- 单独的“作者”区域，和其他信息同级对齐 -->
              <div class="author-wrap no-dot">
                作者：
                <div class="post-avatar">
                  <img
                    :src="getUserAvatar(post.authorId)"
                    alt="作者头像"
                    class="avatar-img"
                    :title="post.author || '匿名用户'"
                  />
                </div>
                <span class="author-name">{{ post.author || '匿名用户' }}</span>
              </div>
              <span>{{ post.forumName || '未知吧名' }}</span>
              <span>{{ formatTime(post.createdAt) }}</span>
            </div>
          </div>
        </router-link>
      </li>
    </ul>
  </section>
</template>

<script lang="ts" setup>
import type { Post } from '@/types'
import { formatTime } from '@/utils/format'
import { getUserAvatar } from '@/utils/load_image'

defineProps<{
  posts: Post[]
}>()
</script>

<style scoped>
/* 基础布局样式 */
.section {
  margin-bottom: var(--sp-8); /* 增加底部间距 */
  padding: var(--sp-5); /* 增加内边距 */
  background: var(--color-card-bg);
  border-radius: var(--radius-lg);
  border: 1px solid var(--color-border);
}

.section-head {
  display: flex;
  align-items: baseline;
  justify-content: space-between;
  gap: var(--sp-3);
  margin-bottom: var(--sp-5); /* 增加间距 */
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

/* 空状态样式 */
.empty-state {
  padding: var(--sp-6); /* 增加内边距 */
  text-align: center;
  color: var(--color-text-2);
  background: var(--color-background);
  border-radius: var(--radius-md);
  border: 1px dashed var(--color-border);
}

/* 帖子列表样式 */
.post-list {
  list-style: none;
  padding: 0;
  margin: 0;
  display: flex;
  flex-direction: column;
  gap: var(--sp-5); /* 增加列表项间距 */
}

.post-item {
  border-radius: var(--radius-md);
  background: var(--color-background);
  border: 1px solid var(--color-border);
  transition:
    box-shadow var(--ease-fast) ease,
    transform var(--ease-fast) ease;
}

.post-item:hover {
  box-shadow: var(--shadow-md); /* 增强hover效果 */
  transform: translateY(-2px);
}

.card-link {
  display: flex;
  flex-direction: column;
  gap: var(--sp-4); /* 增加内部间距 */
  text-decoration: none;
  color: var(--color-text-1);
  padding: var(--sp-5); /* 增加卡片内边距 */
}

/* 帖子头部样式 */
.post-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: var(--sp-3);
}

.post-head h3 {
  font-size: var(--fz-h3);
  font-weight: 600;
  margin: 0;
  line-height: 1.4; /* 增加行高 */
}

.badge {
  padding: 2px var(--sp-2); /* 优化内边距 */
  border-radius: var(--radius-sm);
  background: color-mix(in srgb, var(--color-brand) 12%, transparent);
  color: var(--color-brand);
  font-size: var(--fz-caption);
  font-weight: var(--fw-medium);
  white-space: nowrap;
}

/* 摘要文本样式 */
.excerpt {
  font-size: var(--fz-body) * 0.8;
  line-height: var(--lh-body);
  color: var(--color-text-2);
  margin: 0;
  padding-bottom: var(--sp-2); /* 增加底部间距 */
}

/* 元数据区域样式（合并重复样式） */
.post-meta {
  display: flex;
  align-items: center;
  gap: var(--sp-4); /* 使用设计系统变量 */
  font-size: var(--fz-sub);
  color: var(--color-text-3);
  padding-top: var(--sp-2); /* 增加顶部间距 */
  flex-wrap: wrap; /* 适配小屏幕 */
}

/* .post-meta span::before {
  content: '•';
  margin-right: var(--sp-2);
  color: var(--color-border-strong);
} */
/* 
.post-meta span:first-child::before {
  content: '';
  margin-right: 0;
} */

/* 作者信息样式（合并） */
.author-wrap {
  display: flex;
  align-items: center;
  gap: var(--sp-1); /* 使用设计系统变量 */
}

.post-avatar {
  display: inline-block;
}

.avatar-img {
  width: 18px;
  height: 18px;
  border-radius: 50%;
  object-fit: cover;
  vertical-align: middle;
  margin-right: 2px;
}

.author-name {
  vertical-align: middle;
}

/* 响应式样式 */
@media (max-width: 768px) {
  .section {
    padding: var(--sp-4);
    margin-bottom: var(--sp-6);
  }

  .post-head {
    flex-direction: column;
    align-items: flex-start;
    gap: var(--sp-2);
  }

  .card-link {
    padding: var(--sp-4);
  }

  .post-meta {
    gap: var(--sp-3);
    font-size: var(--fz-caption);
  }
}
</style>
