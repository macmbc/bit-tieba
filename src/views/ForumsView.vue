<template>
  <div class="forums-view two-col-page">
    <header class="page-header">
      <div>
        <p class="eyebrow">关注中心</p>
        <h1>我关注的吧</h1>
        <p class="description">集中管理你喜爱的版块，及时掌握最新动态。</p>
      </div>
      <button type="button" class="primary-btn" @click="goManage">管理关注</button>
    </header>

    <div class="content-layout">
      <aside class="filter-panel" aria-label="筛选">
        <label class="filter-label" for="forum-search">关键字</label>
        <div class="search-field">
          <span aria-hidden="true">🔍</span>
          <input
            id="forum-search"
            v-model="searchKeyword"
            type="search"
            placeholder="搜索已关注的吧"
          />
        </div>

        <div class="filter-group">
          <div class="group-head">
            <span>筛选标签</span>
            <button
              v-if="activeFilter !== 'all' || searchKeyword"
              type="button"
              class="ghost-btn"
              @click="resetFilters"
            >
              清除
            </button>
          </div>
          <div class="chip-group" role="group" aria-label="筛选标签">
            <button
              v-for="chip in filterChips"
              :key="chip.value"
              type="button"
              class="chip"
              :class="{ active: chip.value === activeFilter }"
              @click="activeFilter = chip.value"
            >
              {{ chip.label }}
            </button>
          </div>
        </div>
      </aside>

      <section class="forum-section" aria-live="polite">
        <div class="sort-row" role="group" aria-label="排序">
          <button
            v-for="option in sortOptions"
            :key="option.value"
            type="button"
            class="sort-btn"
            :class="{ active: sortMode === option.value }"
            @click="sortMode = option.value"
          >
            {{ option.label }}
          </button>
        </div>

        <div v-if="loading" class="forum-grid">
          <article v-for="id in skeletonPlaceholders" :key="id" class="forum-card skeleton-card">
            <div class="skeleton skeleton-line title"></div>
            <div class="skeleton skeleton-line sub"></div>
            <div class="skeleton skeleton-line sub"></div>
          </article>
        </div>
        <div v-else-if="error" class="error-state">
          <p>加载失败：{{ error }}</p>
          <button type="button" class="ghost-btn" @click="loadForums">重试</button>
        </div>
        <div v-else-if="!displayedForums.length" class="empty-state">
          <h3>还没有关注任何吧</h3>
          <p>去首页或热门榜逛逛，添加一些你感兴趣的版块吧。</p>
        </div>
        <div v-else class="forum-grid">
          <article v-for="forum in displayedForums" :key="forum.id" class="forum-card">
            <router-link :to="`/forum/${forum.id}`" class="card-link">
              <div class="card-head">
                <div
                  class="logo"
                  aria-hidden="true"
                  :style="{ backgroundImage: `url(${getForumLogo(forum.avatar)})` }"
                ></div>
                <div>
                  <h3>{{ forum.name }}</h3>
                  <p>{{ forum.description || '这个吧暂时没有简介～' }}</p>
                </div>
              </div>
              <div class="stats">
                <span>帖子 {{ formatNumber(forum.postCount) }}</span>
                <span>热度 {{ calcHeat(forum.postCount) }}</span>
              </div>
            </router-link>
            <div class="card-actions">
              <button
                type="button"
                class="ghost-btn"
                :disabled="unfollowing[forum.id]"
                @click="unfollowForum(forum.id)"
              >
                {{ unfollowing[forum.id] ? '取消中...' : '取消关注' }}
              </button>
            </div>
          </article>
        </div>
      </section>
    </div>
  </div>
</template>

<script lang="ts" setup>
import { ref, onMounted, computed } from 'vue'
import { useRouter } from 'vue-router'
import { useUserStore } from '@/stores/user'
import { getFollowedForums, unfollowForumApi } from '@/api/forumApi'
import { getForumLogo } from '@/utils/load_image'
import type { Forum } from '@/types'

const userStore = useUserStore()
const router = useRouter()
const followedForums = ref<Forum[]>([])
const loading = ref(true)
const error = ref('')
const unfollowing = ref<Record<number, boolean>>({})
type FilterValue = 'all' | 'active' | 'quiet'
type SortValue = 'hot' | 'recent'
const activeFilter = ref<FilterValue>('all')
const sortMode = ref<SortValue>('hot')
const searchKeyword = ref('')
const filterChips = [
  { label: '全部吧', value: 'all' as FilterValue },
  { label: '高活跃', value: 'active' as FilterValue },
  { label: '安静整理', value: 'quiet' as FilterValue },
]
const sortOptions = [
  { label: '热度排序', value: 'hot' as SortValue },
  { label: '最新关注', value: 'recent' as SortValue },
]
const skeletonPlaceholders = Array.from({ length: 6 }, (_, idx) => idx)

const displayedForums = computed(() => {
  let list = [...followedForums.value]
  const keyword = searchKeyword.value.trim().toLowerCase()
  if (keyword) {
    list = list.filter((forum) => {
      const desc = forum.description ?? ''
      return forum.name.toLowerCase().includes(keyword) || desc.toLowerCase().includes(keyword)
    })
  }

  if (activeFilter.value === 'active') {
    list = list.filter((forum) => forum.postCount >= 1000)
  } else if (activeFilter.value === 'quiet') {
    list = list.filter((forum) => forum.postCount < 100)
  }

  if (sortMode.value === 'hot') {
    list.sort((a, b) => b.postCount - a.postCount)
  } else {
    list.sort((a, b) => b.id - a.id)
  }

  return list
})

const resetFilters = () => {
  activeFilter.value = 'all'
  searchKeyword.value = ''
}

const formatNumber = (count: number) => {
  if (count >= 10000) {
    return `${(count / 10000).toFixed(1)}w+`
  }
  return count.toLocaleString('zh-CN')
}

const calcHeat = (count: number) => {
  if (count >= 5000) return '🔥 活跃'
  if (count >= 1000) return '🙂 稳定'
  return '🌱 新芽'
}

const loadForums = async () => {
  if (!userStore.isLoggedIn) {
    error.value = '请先登录以查看关注的吧'
    loading.value = false
    return
  }

  try {
    loading.value = true
    error.value = ''
    followedForums.value = await getFollowedForums(userStore.userId)
  } catch (err) {
    error.value = (err as Error).message
  } finally {
    loading.value = false
  }
}

onMounted(loadForums)

const goManage = () => {
  router.push('/me')
}

// 取消关注
const unfollowForum = async (forumId: number) => {
  if (!userStore.isLoggedIn || !userStore.userId) return

  try {
    unfollowing.value[forumId] = true
    await unfollowForumApi(userStore.userId, forumId)
    followedForums.value = followedForums.value.filter((f) => f.id !== forumId)
  } catch (err) {
    error.value = `取消关注失败：${(err as Error).message}`
  } finally {
    unfollowing.value[forumId] = false
  }
}
</script>

<style scoped>
.forums-view {
  max-width: var(--layout-heart-width);
  margin: 0 auto;
  padding: var(--sp-5) 0;
}

.page-header {
  display: flex;
  justify-content: space-between;
  gap: var(--sp-6);
  align-items: flex-start;
  margin-bottom: var(--sp-6);
}

.eyebrow {
  margin: 0;
  font-size: var(--fz-sub);
  color: var(--color-text-3);
  text-transform: uppercase;
}

h1 {
  margin: var(--sp-1) 0;
  font-size: var(--fz-h1);
  line-height: var(--lh-title);
}

.description {
  margin: 0;
  color: var(--color-text-2);
}

.primary-btn {
  border: none;
  border-radius: var(--radius-md);
  padding: var(--sp-3) var(--sp-4);
  background: var(--color-brand);
  color: #fff;
  font-weight: var(--fw-medium);
  cursor: pointer;
  transition:
    background-color var(--ease-fast) ease,
    transform var(--ease-fast) ease;
}

.primary-btn:hover {
  background: var(--color-brand-hover);
}

.primary-btn:active {
  transform: translateY(1px);
}

.content-layout {
  display: grid;
  grid-template-columns: minmax(240px, 280px) minmax(0, 1fr);
  gap: var(--sp-6);
}

.filter-panel {
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  padding: var(--sp-4);
  background: var(--color-card-bg);
  position: sticky;
  top: calc(64px + var(--sp-4));
  height: fit-content;
  display: flex;
  flex-direction: column;
  gap: var(--sp-4);
}

.filter-label {
  font-size: var(--fz-sub);
  color: var(--color-text-2);
}

.search-field {
  display: flex;
  align-items: center;
  gap: var(--sp-2);
  border: 1px solid var(--color-border);
  border-radius: var(--radius-md);
  padding: 0 var(--sp-3);
  height: 40px;
  background: var(--color-background);
}

.search-field input {
  border: none;
  flex: 1;
  background: transparent;
  color: var(--color-text-1);
}

.search-field input:focus {
  outline: none;
}

.filter-group {
  display: flex;
  flex-direction: column;
  gap: var(--sp-3);
}

.group-head {
  display: flex;
  justify-content: space-between;
  font-size: var(--fz-sub);
  color: var(--color-text-2);
}

.chip-group {
  display: flex;
  flex-wrap: wrap;
  gap: var(--sp-2);
}

.chip {
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  padding: var(--sp-1) var(--sp-3);
  background: transparent;
  color: var(--color-text-2);
  font-size: var(--fz-sub);
  cursor: pointer;
  transition: all var(--ease-fast) ease;
}

.chip.active {
  border-color: var(--color-brand);
  background: color-mix(in srgb, var(--color-brand) 12%, transparent);
  color: var(--color-brand);
}

.forum-section {
  display: flex;
  flex-direction: column;
  gap: var(--sp-4);
}

.sort-row {
  display: inline-flex;
  border: 1px solid var(--color-border);
  border-radius: var(--radius-md);
  overflow: hidden;
  align-self: flex-start;
}

.sort-btn {
  border: none;
  background: transparent;
  padding: var(--sp-2) var(--sp-4);
  font-size: var(--fz-sub);
  color: var(--color-text-2);
  cursor: pointer;
}

.sort-btn.active {
  background: var(--color-brand);
  color: #fff;
}

.forum-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
  gap: var(--sp-4);
}

.forum-card {
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  background: var(--color-background);
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  box-shadow: 0 1px 2px rgba(0, 0, 0, 0.04);
}

.card-link {
  display: flex;
  flex-direction: column;
  gap: var(--sp-3);
  padding: var(--sp-4);
  text-decoration: none;
  color: var(--color-text-1);
}

.card-head {
  display: flex;
  gap: var(--sp-3);
}

.logo {
  width: 48px;
  height: 48px;
  border-radius: var(--radius-md);
  background: color-mix(in srgb, var(--color-brand) 15%, transparent);
  color: var(--color-brand);
  display: inline-flex;
  align-items: center;
  justify-content: center;
  font-weight: var(--fw-bold);
  background-size: cover;
}

.card-head h3 {
  margin: 0;
  font-size: var(--fz-h3);
}

.card-head p {
  margin: var(--sp-1) 0 0;
  color: var(--color-text-2);
  font-size: var(--fz-sub);
}

.stats {
  display: flex;
  gap: var(--sp-4);
  font-size: var(--fz-caption);
  color: var(--color-text-3);
}

.stats span::before {
  content: '•';
  margin-right: var(--sp-2);
  color: var(--color-border-strong);
}

.stats span:first-child::before {
  content: '';
  margin-right: 0;
}

.card-actions {
  padding: 0 var(--sp-4) var(--sp-4);
}

.ghost-btn {
  border: 1px solid var(--color-border);
  border-radius: var(--radius-md);
  background: transparent;
  padding: var(--sp-2) var(--sp-4);
  font-size: var(--fz-sub);
  color: var(--color-text-2);
  cursor: pointer;
  transition:
    border-color var(--ease-fast) ease,
    color var(--ease-fast) ease;
}

.ghost-btn:hover:not(:disabled) {
  border-color: var(--color-brand);
  color: var(--color-brand);
}

.ghost-btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.empty-state,
.error-state {
  border: 1px dashed var(--color-border);
  border-radius: var(--radius-md);
  padding: var(--sp-5);
  text-align: center;
  color: var(--color-text-2);
  background: var(--color-card-bg);
}

.error-state p {
  margin-bottom: var(--sp-3);
  color: var(--color-danger);
}

.skeleton-card {
  padding: var(--sp-4);
  display: flex;
  flex-direction: column;
  gap: var(--sp-2);
}

.skeleton-card .title {
  width: 60%;
}

.skeleton-card .sub {
  width: 80%;
}

@media (max-width: 992px) {
  .content-layout {
    grid-template-columns: 1fr;
  }

  .filter-panel {
    position: static;
  }
}

@media (max-width: 576px) {
  .page-header {
    flex-direction: column;
  }

  .sort-row {
    width: 100%;
  }

  .forum-grid {
    grid-template-columns: 1fr;
  }
}
</style>
