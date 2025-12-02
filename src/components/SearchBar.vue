<template>
  <form class="search-bar" role="search" @submit.prevent="handleSearch">
    <span class="icon" aria-hidden="true">
      <svg
        width="20"
        height="20"
        viewBox="0 0 24 24"
        fill="none"
        stroke="currentColor"
        stroke-width="2"
      >
        <circle cx="11" cy="11" r="8"></circle>
        <line x1="21" y1="21" x2="16.65" y2="16.65"></line>
      </svg>
    </span>
    <input
      v-model="searchQuery"
      type="search"
      class="search-input"
      placeholder="搜索吧、帖子或用户"
      autocomplete="off"
      maxlength="50"
    />
    <button type="submit" class="search-btn">搜索</button>
  </form>
</template>

<script lang="ts" setup>
import { ref } from 'vue'
import { useRouter } from 'vue-router'

const searchQuery = ref('')
const router = useRouter()

const handleSearch = () => {
  const query = searchQuery.value.trim()
  if (!query) return

  router.push({
    path: '/search',
    query: {
      q: query,
      type: 'post',   // 默认搜索帖子
      page: '1',
    }
  })
}

</script>

<style scoped>
.search-bar {
  display: flex;
  align-items: center;
  gap: var(--sp-2);
  height: 44px;
  padding: 0 var(--sp-3);
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  background: var(--color-background);
  transition:
    border-color var(--ease-fast) ease,
    box-shadow var(--ease-fast) ease;
}

.search-bar:focus-within {
  border-color: var(--color-brand);
  box-shadow: 0 0 0 3px color-mix(in srgb, var(--color-brand) 15%, transparent);
}

.icon {
  font-size: 1rem;
  color: var(--color-text-3);
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--color-text-3);
}

.search-input {
  flex: 1;
  border: none;
  font-size: var(--fz-body);
  background: transparent;
  color: var(--color-text-1);
}

.search-input::placeholder {
  color: var(--color-text-3);
}

.search-input:focus {
  outline: none;
}

.search-btn {
  border: none;
  border-radius: var(--radius-md);
  padding: 0 var(--sp-3);
  height: 32px;
  font-size: var(--fz-sub);
  font-weight: var(--fw-medium);
  background: var(--color-brand);
  color: #fff;
  cursor: pointer;
  transition:
    transform var(--ease-fast) ease,
    background-color var(--ease-fast) ease;
}

.search-btn:hover {
  background: var(--color-brand-hover);
}

.search-btn:active {
  transform: scale(0.96);
}

@media (max-width: 576px) {
  .search-bar {
    width: 100%;
  }
  .search-btn {
    display: none;
  }
}
</style>
