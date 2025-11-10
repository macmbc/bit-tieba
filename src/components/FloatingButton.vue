<template>
  <button class="floating-btn" :title="tooltip" @click="$emit('click')">
    <i :class="icon"></i>
  </button>
</template>

<script setup lang="ts">
defineProps<{
  icon: string
  tooltip?: string
}>()
defineEmits(['click'])
</script>

<style>
.floating-btn {
  width: 50px;
  height: 50px;
  border: none;
  border-radius: 50%;
  background-color: #409eff;
  color: white;
  font-size: 20px;
  cursor: pointer;
  box-shadow: 0 4px 6px rgba(0, 0, 0, 0.2);
  transition: 0.2s;
}
.floating-btn:hover {
  background-color: #66b1ff;
}

/* 可选：确保 icon 继承颜色 */
.floating-btn i {
  color: inherit;
}
</style>
