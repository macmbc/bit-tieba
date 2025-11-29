<template>
  <div class="search-bar">
    <input
      v-model="searchQuery"
      placeholder="搜索吧、帖子或用户"
      @input="handleSearch"
      class="search-input"
    />
    <button class="search-btn">🔍</button>
  </div>
</template>

<script lang="ts" setup>
import { ref } from 'vue'
import { useRouter } from 'vue-router'

const searchQuery = ref('')
const router = useRouter()

const handleSearch = () => {
  const query = searchQuery.value.trim()
  if (!query) return
  router.push({ path: '/search', query: { q: query } })
}
</script>

<style scoped>
.search-bar {
  display: flex;
  align-items: center;
  margin-bottom: var(--sp-5);
}

.icon {
  font-size: 1rem;
  color: var(--color-text-3);
}

.search-input {
  width: 100%;
  max-width: 500px;
  padding: var(--sp-3);
  border: 1px solid var(--color-border);
  border-radius: var(--radius-md) 0 0 var(--radius-md);
  font-size: var(--fz-body);
  background: var(--color-card-bg);
  color: var(--color-text-1);
}

.search-input::placeholder {
  color: var(--color-text-3);
}

.search-input:focus {
  outline: none;
}

.search-btn {
  padding: var(--sp-3) var(--sp-4);
  border: 1px solid var(--color-border);
  border-left: none;
  background: var(--color-brand);
  color: #fff;
  border-radius: 0 var(--radius-md) var(--radius-md) 0;
  cursor: pointer;
  transition: background var(--ease-fast);
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