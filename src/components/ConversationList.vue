<!-- src/components/ConversationList.vue -->
<template>
  <div class="conversation-list">
    <div v-for="group in groups" :key="group.title" class="group">
      <p class="group-title">{{ group.title }}</p>
      <ul class="items" role="list">
        <li
          v-for="item in group.items"
          :key="item.id"
          :class="['item', { active: item.id === activeId }]"
          @click="$emit('select', item.id)"
        >
          <img :src="item.avatar || placeholder" class="avatar" alt="会话头像" />
          <div class="meta">
            <div class="row-1">
              <span class="name" :title="item.name">{{ item.name }}</span>
              <span class="time">{{ item.time }}</span>
            </div>
            <div class="row-2">
              <span class="brief" :title="item.brief">{{ item.brief }}</span>
              <span v-if="item.unread" class="badge">{{ formatUnread(item.unread) }}</span>
            </div>
          </div>
        </li>
      </ul>
    </div>
    <div v-if="!groups.length" class="empty">暂无会话，先添加好友开始聊天吧</div>
  </div>
</template>

<script setup lang="ts">
import placeholder from '@/assets/default.png'

export interface ConversationItem {
  id: number
  name: string
  brief: string
  time: string
  unread?: number
  avatar?: string
}

export interface ConversationGroup {
  title: string
  items: ConversationItem[]
}

withDefaults(
  defineProps<{
    groups: ConversationGroup[]
    activeId?: number
  }>(),
  {
    groups: () => [],
  },
)

const formatUnread = (value: number) => (value > 99 ? '99+' : value)
</script>

<style scoped>
.conversation-list {
  display: flex;
  flex-direction: column;
  height: 100%;
  min-height: 0;
}

.group {
  padding: var(--sp-4);
  border-bottom: 1px solid var(--color-border);
}

.group:last-of-type {
  border-bottom: none;
}

.group-title {
  font-size: var(--fz-caption);
  color: var(--color-text-3);
  margin-bottom: var(--sp-2);
}

.items {
  list-style: none;
  padding: 0;
  margin: 0;
  display: flex;
  flex-direction: column;
  gap: var(--sp-2);
}

.item {
  display: flex;
  gap: var(--sp-3);
  padding: var(--sp-3);
  border-radius: var(--radius-md);
  cursor: pointer;
  transition: background-color var(--ease-fast) ease, border-color var(--ease-fast) ease;
  border: 1px solid transparent;
}

.item:hover {
  background: color-mix(in srgb, var(--color-brand) 8%, transparent);
}

.item.active {
  border-color: var(--color-brand);
  background: color-mix(in srgb, var(--color-brand) 12%, transparent);
}

.avatar {
  width: 44px;
  height: 44px;
  border-radius: 50%;
  object-fit: cover;
  flex-shrink: 0;
  box-shadow: var(--shadow-sm);
}

.meta {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
  gap: var(--sp-1);
}

.row-1,
.row-2 {
  display: flex;
  align-items: center;
  gap: var(--sp-2);
}

.row-1 {
  justify-content: space-between;
}

.name {
  color: var(--color-text-1);
  font-weight: var(--fw-medium);
}

.time {
  color: var(--color-text-3);
  font-size: var(--fz-caption);
}

.brief {
  flex: 1;
  color: var(--color-text-2);
  font-size: var(--fz-caption);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.badge {
  background: var(--color-danger);
  color: #fff;
  border-radius: 999px;
  padding: 0 var(--sp-2);
  font-size: 11px;
  line-height: 18px;
  min-width: 24px;
  text-align: center;
}

.empty {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--color-text-3);
  font-size: var(--fz-sub);
  padding: var(--sp-4);
}

@media (prefers-reduced-motion: reduce) {
  .item {
    transition: none;
  }
}
</style>
