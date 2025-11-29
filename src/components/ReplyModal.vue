<!-- src/components/ReplyModal.vue -->
<template>
  <Teleport to="body">
    <transition name="reply-modal">
      <div v-if="show" class="reply-modal-overlay" @click="close">
        <div
          class="reply-modal"
          role="dialog"
          :aria-labelledby="titleId"
          :aria-describedby="descriptionId"
          aria-modal="true"
          @click.stop
          @keydown.esc.prevent="close"
        >
          <div class="modal-header">
            <h3 :id="titleId">{{ targetUser ? `回复 @${targetUser}` : '发表回复' }}</h3>
            <button class="icon-btn" type="button" @click="close" aria-label="关闭回复弹窗">
              ×
            </button>
          </div>

          <p :id="descriptionId" class="sr-only">输入内容后可按快捷键 Ctrl + Enter 提交</p>

          <textarea
            ref="textareaRef"
            v-model="localContent"
            :placeholder="placeholder"
            maxlength="1000"
            :disabled="props.replying"
            class="reply-textarea"
            @keydown.ctrl.enter.prevent="submit"
          ></textarea>

          <div class="modal-footer">
            <button class="ghost-btn" type="button" @click="close" :disabled="props.replying">取消</button>
            <button
              class="primary-btn"
              type="button"
              @click="submit"
              :disabled="props.replying || !localContent.trim()"
            >
              {{ props.replying ? '发送中...' : '发表' }}
            </button>
          </div>
        </div>
      </div>
    </transition>
  </Teleport>
</template>

<script setup lang="ts">
import { computed, nextTick, ref, watch } from 'vue'

const props = defineProps<{
  show: boolean
  replying?: boolean
  targetUser?: string
  parentId?: number
  postId: number
}>()

const emit = defineEmits<{
  (e: 'update:show', value: boolean): void
  (e: 'submit', payload: { content: string; parentId?: number }): void
}>()

const instanceId = Math.random().toString(36).slice(2, 8)
const titleId = `reply-modal-title-${instanceId}`
const descriptionId = `reply-modal-description-${instanceId}`

const textareaRef = ref<HTMLTextAreaElement | null>(null)
const localContent = ref('')

const placeholder = computed(() =>
  props.targetUser ? `回复 @${props.targetUser}...` : '说点什么吧...'
)

watch(
  () => props.show,
  (visible) => {
    if (visible) {
      localContent.value = props.targetUser ? `@${props.targetUser} ` : ''
      nextTick(() => textareaRef.value?.focus())
    } else {
      localContent.value = ''
    }
  },
  { immediate: true }
)

watch(
  () => props.targetUser,
  (name) => {
    if (!props.show) return
    if (!localContent.value.trim()) {
      localContent.value = name ? `@${name} ` : ''
      nextTick(() => textareaRef.value?.focus())
    }
  }
)

const close = () => {
  if (props.replying) return
  emit('update:show', false)
}

const submit = () => {
  if (props.replying || !localContent.value.trim()) return

  emit('submit', {
    content: localContent.value.trim(),
    parentId: props.parentId,
  })
}
</script>

<style scoped>
.reply-modal-overlay {
  position: fixed;
  inset: 0;
  background: rgba(0, 0, 0, 0.5);
  backdrop-filter: blur(2px);
  display: flex;
  align-items: center;
  justify-content: center;
  padding: var(--sp-4);
  z-index: 1000;
}

.reply-modal {
  width: min(520px, calc(100% - var(--sp-6)));
  background: var(--color-card-bg);
  border-radius: var(--radius-lg);
  border: 1px solid var(--color-border);
  box-shadow: var(--shadow-md);
  display: flex;
  flex-direction: column;
  gap: var(--sp-3);
  padding: var(--sp-4);
}

.modal-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: var(--sp-3);
}

.modal-header h3 {
  font-size: var(--fz-h3);
  color: var(--color-text-1);
  font-weight: var(--fw-medium);
}

.icon-btn {
  border: none;
  background: transparent;
  color: var(--color-text-2);
  font-size: 1.5rem;
  line-height: 1;
  width: 36px;
  height: 36px;
  border-radius: 50%;
  transition: background-color var(--ease-fast) ease, color var(--ease-fast) ease;
  cursor: pointer;
}

.icon-btn:hover {
  background: color-mix(in srgb, var(--color-brand) 12%, transparent);
  color: var(--color-brand);
}

.reply-textarea {
  width: 100%;
  min-height: 140px;
  padding: var(--sp-3);
  border-radius: var(--radius-md);
  border: 1px solid var(--color-border);
  background: var(--color-background);
  color: var(--color-text-1);
  font-size: var(--fz-body);
  font-family: inherit;
  resize: vertical;
  transition: border-color var(--ease-fast) ease, box-shadow var(--ease-fast) ease;
}

.reply-textarea:focus {
  outline: none;
  border-color: var(--color-brand);
  box-shadow: 0 0 0 1px color-mix(in srgb, var(--color-brand) 40%, transparent);
}

.reply-textarea:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

.modal-footer {
  display: flex;
  justify-content: flex-end;
  gap: var(--sp-3);
}

.ghost-btn,
.primary-btn {
  min-width: 96px;
  height: 40px;
  border-radius: var(--radius-md);
  font-weight: var(--fw-medium);
  border: 1px solid transparent;
  cursor: pointer;
  transition: background-color var(--ease-fast) ease, color var(--ease-fast) ease, border-color var(--ease-fast) ease;
}

.ghost-btn {
  background: transparent;
  border-color: var(--color-border);
  color: var(--color-text-2);
}

.ghost-btn:hover:not(:disabled) {
  border-color: var(--color-text-2);
  color: var(--color-text-1);
}

.primary-btn {
  background: var(--color-brand);
  color: #fff;
}

.primary-btn:hover:not(:disabled) {
  background: var(--color-brand-hover);
}

.ghost-btn:disabled,
.primary-btn:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

.sr-only {
  position: absolute;
  width: 1px;
  height: 1px;
  padding: 0;
  margin: -1px;
  overflow: hidden;
  clip: rect(0, 0, 0, 0);
  white-space: nowrap;
  border: 0;
}

.reply-modal-enter-active,
.reply-modal-leave-active {
  transition: opacity var(--ease-normal) ease,
    transform var(--ease-normal) ease;
}

.reply-modal-enter-from,
.reply-modal-leave-to {
  opacity: 0;
  transform: translateY(12px) scale(0.98);
}

.reply-modal-enter-to,
.reply-modal-leave-from {
  opacity: 1;
  transform: translateY(0) scale(1);
}

@media (max-width: 640px) {
  .reply-modal {
    width: 100%;
    padding: var(--sp-3);
  }
}

@media (prefers-color-scheme: dark) {
  .reply-modal-overlay {
    background: rgba(0, 0, 0, 0.65);
  }

  .reply-modal {
    border-color: var(--color-border);
  }
}

@media (prefers-reduced-motion: reduce) {
  .reply-modal-overlay,
  .reply-modal {
    transition: none;
  }

  .reply-modal-enter-active,
  .reply-modal-leave-active {
    transition: none;
  }
}
</style>
