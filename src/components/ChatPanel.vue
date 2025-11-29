<!-- src/components/ChatPanel.vue -->
<template>
  <div class="chat-panel">
    <header v-if="participant" class="chat-header">
      <div class="peer">
        <img :src="placeholder" class="peer-avatar" alt="对话头像" />
        <div class="peer-info">
          <p class="peer-name">{{ participant }}</p>
          <p class="peer-status">最近活跃 · 在线</p>
        </div>
      </div>
      <div class="header-actions">
        <button class="ghost" type="button">资料</button>
        <button class="ghost" type="button">屏蔽</button>
      </div>
    </header>

    <section class="messages" ref="scrollContainer">
      <p v-if="!participant" class="placeholder">左侧选择一个会话开始聊天</p>
      <ul v-else class="flow" role="list">
        <li
          v-for="message in orderedMessages"
          :key="message.id"
          :class="['msg-row', message.sender === currentUser ? 'right' : 'left']"
        >
          <div class="name-line">
            <span class="name">{{ message.sender === currentUser ? '我' : message.sender }}</span>
            <span class="dot" aria-hidden="true" />
            <span class="time">{{ formatTime(message.timestamp) }}</span>
          </div>
          <article :class="['bubble', message.sender === currentUser ? 'out' : 'in']">
            <p class="text">{{ message.content }}</p>
            <footer class="bubble-footer">
              <span class="status">{{ message.sender === currentUser ? '已发送' : '已读' }}</span>
              <button class="link-btn" type="button">收藏</button>
            </footer>
          </article>
        </li>
      </ul>
    </section>

    <footer v-if="participant" class="composer">
      <textarea
        v-model="draft"
        class="composer-input"
        rows="3"
        placeholder="输入消息..."
        @keydown.enter.exact.prevent="submit"
        @keydown.enter.shift.stop
      ></textarea>
      <div class="toolbar">
        <button class="ghost" type="button">表情</button>
        <button class="ghost" type="button">图片</button>
      </div>
      <div class="composer-actions">
        <small class="hint">Enter 发送 · Shift + Enter 换行</small>
        <button class="primary" type="button" :disabled="!draft.trim()" @click="submit">
          发送
        </button>
      </div>
    </footer>
  </div>
</template>

<script setup lang="ts">
import { computed, nextTick, ref, watch } from 'vue'
import type { Message } from '@/types'
import { useUserStore } from '@/stores/user'
import { formatTime } from '@/utils/format'
import placeholder from '@/assets/default.png'

const props = defineProps<{
  messages: Message[]
  participant?: string
}>()

const emit = defineEmits<{
  (e: 'send', content: string): void
}>()

const draft = ref('')
const scrollContainer = ref<HTMLDivElement | null>(null)
const userStore = useUserStore()
const currentUser = userStore.userId || '我'

const orderedMessages = computed(() =>
  [...props.messages].sort((a, b) => a.timestamp - b.timestamp),
)

const submit = () => {
  const content = draft.value.trim()
  if (!content) return
  emit('send', content)
  draft.value = ''
}

watch(
  () => props.messages.length,
  async () => {
    await nextTick()
    if (scrollContainer.value) {
      scrollContainer.value.scrollTop = scrollContainer.value.scrollHeight
    }
  },
)
</script>

<style scoped>
.chat-panel {
  display: flex;
  flex-direction: column;
  height: 100%;
  min-height: 0;
  background: var(--color-card-bg);
  border-radius: var(--radius-lg);
  border: 1px solid var(--color-border);
  box-shadow: var(--shadow-sm);
}

.chat-header {
  padding: var(--sp-4) var(--sp-5);
  border-bottom: 1px solid var(--color-border);
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: var(--sp-4);
}

.peer {
  display: flex;
  align-items: center;
  gap: var(--sp-3);
}

.peer-avatar {
  width: 52px;
  height: 52px;
  border-radius: 50%;
  object-fit: cover;
  box-shadow: var(--shadow-sm);
}

.peer-info {
  display: flex;
  flex-direction: column;
  gap: var(--sp-1);
}

.peer-name {
  font-size: var(--fz-h3);
  font-weight: var(--fw-medium);
  color: var(--color-text-1);
  margin: 0;
}

.peer-status {
  margin: 0;
  color: var(--color-text-3);
  font-size: var(--fz-caption);
}

.header-actions {
  display: flex;
  gap: var(--sp-2);
}

.ghost,
.link-btn,
.primary {
  font: inherit;
}

.ghost {
  border: 1px solid var(--color-border);
  background: transparent;
  color: var(--color-text-2);
  border-radius: var(--radius-md);
  padding: var(--sp-2) var(--sp-3);
  cursor: pointer;
}

.messages {
  flex: 1;
  min-height: 0;
  overflow: auto;
  padding: var(--sp-5);
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.92), rgba(247, 248, 251, 1));
}

.placeholder {
  color: var(--color-text-3);
  text-align: center;
  margin-top: var(--sp-8);
}

.flow {
  list-style: none;
  padding: 0;
  margin: 0;
  display: flex;
  flex-direction: column;
  gap: var(--sp-4);
}

.msg-row {
  max-width: 520px;
  display: flex;
  flex-direction: column;
}

.msg-row.left {
  align-self: flex-start;
}

.msg-row.right {
  align-self: flex-end;
}

.name-line {
  display: flex;
  align-items: center;
  gap: var(--sp-2);
  font-size: var(--fz-caption);
  color: var(--color-text-3);
  margin-bottom: var(--sp-1);
}

.dot {
  width: 4px;
  height: 4px;
  border-radius: 50%;
  background: var(--color-border);
}

.bubble {
  padding: var(--sp-4);
  border-radius: var(--radius-lg);
  border: 1px solid var(--color-border);
  background: var(--color-background);
  box-shadow: 0 12px 30px rgba(15, 23, 42, 0.08);
  display: flex;
  flex-direction: column;
  gap: var(--sp-2);
}

.bubble.out {
  background: linear-gradient(135deg, var(--color-brand), #5b9dff);
  border-color: transparent;
  color: #fff;
}

.bubble.out .text,
.bubble.out .bubble-footer,
.bubble.out .link-btn {
  color: #fff;
}

.text {
  margin: 0;
  font-size: var(--fz-body);
  line-height: var(--lh-body);
  color: var(--color-text-1);
  white-space: pre-wrap;
  word-break: break-word;
}

.bubble-footer {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: var(--fz-caption);
  color: var(--color-text-3);
}

.link-btn {
  border: none;
  background: none;
  color: inherit;
  cursor: pointer;
  text-decoration: underline;
  padding: 0;
}

.composer {
  border-top: 1px solid var(--color-border);
  padding: var(--sp-4) var(--sp-5);
  background: var(--color-card-bg);
  border-radius: 0 0 var(--radius-lg) var(--radius-lg);
}

.composer-input {
  width: 100%;
  border-radius: var(--radius-md);
  border: 1px solid var(--color-border);
  background: var(--color-background);
  padding: var(--sp-3);
  font: inherit;
  resize: none;
}

.composer-input:focus {
  outline: 2px solid color-mix(in srgb, var(--color-brand) 35%, transparent);
  border-color: transparent;
}

.toolbar {
  display: flex;
  gap: var(--sp-2);
  margin-top: var(--sp-2);
}

.composer-actions {
  display: flex;
  justify-content: flex-end;
  align-items: center;
  gap: var(--sp-3);
  margin-top: var(--sp-3);
}

.hint {
  margin-right: auto;
  color: var(--color-text-3);
}

.primary {
  border: none;
  border-radius: var(--radius-md);
  background: var(--color-brand);
  color: #fff;
  font-weight: var(--fw-medium);
  padding: var(--sp-2) var(--sp-4);
  cursor: pointer;
}

.primary:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

@media (max-width: 768px) {
  .chat-panel {
    border-radius: var(--radius-md);
  }

  .messages {
    padding: var(--sp-3);
  }
}

@media (prefers-reduced-motion: reduce) {
  .item,
  .bubble {
    transition: none;
  }
}
</style>
