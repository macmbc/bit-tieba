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
import { ref } from 'vue';
import { useRouter } from 'vue-router';

const searchQuery = ref('');
const router = useRouter();

const handleSearch = () => {
  if (searchQuery.value.trim()) {
    router.push(`/search?q=${searchQuery.value}`);
  }
};
</script>

<style scoped>
.search-bar {
  display: flex;
  align-items: center;
  margin-bottom: 20px;
}

.search-input {
  width: 100%;
  max-width: 500px;
  padding: 10px;
  border: 1px solid #ddd;
  border-radius: 4px 0 0 4px;
  font-size: 1rem;
}

.search-btn {
  padding: 10px 15px;
  border: 1px solid #ddd;
  border-left: none;
  background: #4c91d9; /* 贴吧蓝色 */
  color: white;
  border-radius: 0 4px 4px 0;
  cursor: pointer;
}

.search-btn:hover {
  background: #3a7bc8;
}

/* 暗模式 */
@media (prefers-color-scheme: dark) {
  .search-input {
    background: #222;
    border-color: #444;
    color: #ddd;
  }
  .search-btn {
    background: #6ab0ff;
  }
  .search-btn:hover {
    background: #5a9be6;
  }
}
</style>