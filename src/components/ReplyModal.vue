<!-- src/components/ReplyModal.vue -->
<template>
  <Transition name="modal-fade">
    <div v-if="show" class="reply-modal-mask" @click.self="close">
      <div
        class="reply-modal"
        role="dialog"
        aria-modal="true"
        :aria-labelledby="titleId"
        :aria-describedby="descriptionId"
        tabindex="0"
        @keydown.esc.exact.prevent="close"
      >
        <!-- 右上角关闭按钮 -->
        <button class="modal-close-btn" @click="close" aria-label="关闭">×</button>

        <h3 :id="titleId" class="modal-title">
          {{ parentId ? '回复评论' : '发表评论' }}
        </h3>

        <p :id="descriptionId" class="sr-only">输入你的回复内容，按 Enter 发送，Esc 取消</p>

        <textarea
          ref="textareaRef"
          v-model="localContent"
          :placeholder="placeholder"
          class="reply-textarea"
          @keydown.enter.exact.prevent="submit"
          :disabled="sending || replying"
        />

        <div class="modal-actions">
          <button
            class="btn submit"
            @click="submit"
            :disabled="sending || replying || !localContent.trim()"
          >
            <span v-if="!sending && !replying">发送 (Enter)</span>

            <span v-else class="loading-box">
              <span class="loader"></span>
              发送中...
            </span>
          </button>
        </div>
      </div>
    </div>
  </Transition>
</template>

<script setup lang="ts">
import { computed, nextTick, ref, watch, onMounted, onUnmounted } from 'vue'

const props = defineProps<{
  show: boolean
  replying?: boolean // 父组件传来的发送中状态
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
const sending = ref(false) // 本地的 loading 状态

// 动态 placeholder
const placeholder = computed(() =>
  props.targetUser ? `回复 @${props.targetUser}...` : '说点什么吧...',
)

/* ------------------ 打开弹窗自动聚焦 ------------------ */
watch(
  () => props.show,
  (visible) => {
    if (visible) {
      sending.value = false
      localContent.value = props.targetUser ? `@${props.targetUser} ` : ''

      // 双重保障：nextTick + setTimeout
      nextTick(() => {
        textareaRef.value?.focus()
        setTimeout(() => textareaRef.value?.focus(), 60)
      })
    } else {
      localContent.value = ''
      sending.value = false
    }
  },
  { immediate: true },
)

/* ------------------ 切换 @的人时自动填充 ------------------ */
watch(
  () => props.targetUser,
  (name) => {
    if (!props.show) return
    if (!localContent.value.trim()) {
      localContent.value = name ? `@${name} ` : ''
      nextTick(() => textareaRef.value?.focus())
    }
  },
)

/* ------------------ Esc 关闭弹窗 ------------------ */
const handleKeyDown = (e: KeyboardEvent) => {
  if (e.key === 'Escape' && props.show && !sending.value) {
    close()
  }
}

onMounted(() => {
  window.addEventListener('keydown', handleKeyDown)
})

onUnmounted(() => {
  window.removeEventListener('keydown', handleKeyDown)
})

/* ------------------ Enter 发送 ------------------ */
// const handleEnterSubmit = (e: KeyboardEvent) => {
//   if (e.shiftKey) return // Shift + Enter 换行

//   e.preventDefault()

//   if (!localContent.value.trim()) return
//   if (sending.value || props.replying) return

//   submit()
// }

/* ------------------ 关闭 ------------------ */
const close = () => {
  if (props.replying || sending.value) return
  emit('update:show', false)
}

/* ------------------ 提交 ------------------ */
const submit = async () => {
  if (props.replying || sending.value || !localContent.value.trim()) return

  try {
    sending.value = true

    emit('submit', {
      content: localContent.value.trim(),
      parentId: props.parentId,
    })
  } finally {
    // 注意：真正关闭由父组件控制
    setTimeout(() => {
      sending.value = false
    }, 300)
  }
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
  transition:
    background-color var(--ease-fast) ease,
    color var(--ease-fast) ease;
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
  transition:
    border-color var(--ease-fast) ease,
    box-shadow var(--ease-fast) ease;
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
  transition:
    background-color var(--ease-fast) ease,
    color var(--ease-fast) ease,
    border-color var(--ease-fast) ease;
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
  transition:
    opacity var(--ease-normal) ease,
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

.modal-fade-enter-active,
.modal-fade-leave-active {
  transition: all 0.25s ease;
}
.modal-fade-enter-from,
.modal-fade-leave-to {
  opacity: 0;
  transform: translateY(6px) scale(0.97);
}

.reply-modal-mask {
  position: fixed;
  inset: 0;
  background: rgba(0, 0, 0, 0.45);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 9999;
}

.reply-modal {
  background: #fff;
  width: 520px;
  max-width: 96%;
  border-radius: 14px;
  padding: 20px;
}

.reply-textarea {
  width: 100%;
  min-height: 140px;
  resize: none;
  padding: 12px;
  border-radius: 8px;
  border: 1px solid #ddd;
}

.modal-actions {
  display: flex;
  justify-content: flex-end;
  gap: 12px;
  margin-top: 12px;
}

.loading-box {
  display: flex;
  align-items: center;
  gap: 6px;
}

.loader {
  width: 14px;
  height: 14px;
  border: 2px solid #ddd;
  border-top-color: #000;
  border-radius: 50%;
  animation: spin 0.7s linear infinite;
}

@keyframes spin {
  to {
    transform: rotate(360deg);
  }
}

.reply-modal {
  position: fixed;
  inset: 0;
  margin: auto;
  width: min(520px, 92%);
  height: fit-content;
  background: #ffffff;
  border-radius: 16px;
  padding: 24px 20px 20px;
  box-shadow: 0 20px 50px rgba(0,0,0,0.2);
  z-index: 2000;
  outline: none;

  /* 动效 */
  animation: modalIn 0.2s ease-out;
}

@keyframes modalIn {
  from {
    opacity: 0;
    transform: translateY(10px) scale(0.96);
  }
  to {
    opacity: 1;
    transform: translateY(0) scale(1);
  }
}

.modal-title {
  margin-bottom: 12px;
  font-size: 20px;
  font-weight: 600;
}

.reply-textarea {
  width: 100%;
  min-height: 110px;
  max-height: 260px;
  resize: vertical;
  padding: 12px;
  border-radius: 10px;
  border: 1px solid #e2e2e2;
  font-size: 14px;
  transition: .2s;
}

.reply-textarea:focus {
  outline: none;
  border-color: #2a88e7;
  box-shadow: 0 0 0 2px rgba(79,70,229,0.2);
}

.modal-actions {
  display: flex;
  justify-content: flex-end;
  margin-top: 18px;
}

.btn.submit {
  padding: 8px 18px;
  font-size: 14px;
  background: #2a88e7;
  border: none;
  border-radius: 999px;
  color: white;
  cursor: pointer;
  transition: .2s;
}

.btn.submit:hover {
  background: #4b98e5;
}

.btn.submit:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

.modal-close-btn {
  position: absolute;
  right: 16px;
  top: 12px;
  border: none;
  background: transparent;
  font-size: 22px;
  cursor: pointer;
  color: #888;
  transition: .2s;
}

.modal-close-btn:hover {
  color: #111;
  transform: scale(1.1);
}

</style>
