<!-- src/components/ReplyModal.vue -->
<template>
  <Teleport to="body">
    <transition name="modal">
      <div v-if="show" class="reply-modal-overlay" @click="close">
        <div class="reply-modal" @click.stop>
          <div class="modal-header">
            <h3>{{ targetUser ? `回复 @${targetUser}` : '发表回复' }}</h3>
            <button class="close-btn" @click="close">×</button>
          </div>

          <textarea
            v-model="localContent"
            :placeholder="placeholder"
            maxlength="1000"
            :disabled="props.replying"
            class="reply-textarea"
          ></textarea>

          <div class="modal-footer">
            <button class="cancel-btn" @click="close" :disabled="props.replying">取消</button>
            <button
              class="submit-btn"
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

<script lang="ts" setup>
import { ref, watch, computed } from 'vue'

const props = defineProps<{
  show: boolean
  replying?: boolean
  targetUser?: string
  parentId?: number   // 接收父回复 ID
  postId: number      // 新增：必须传 postId（createReply 需要）
}>()

const emit = defineEmits<{
  (e: 'update:show', val: boolean): void
  (e: 'submit', payload: { content: string; parentId?: number }): void
}>()

/* ---------- 内部状态 ---------- */
const localContent = ref('')

// 自动填充 @用户名
watch(
  () => props.show,
  (val) => {
    if (val) {
      localContent.value = props.targetUser ? `@${props.targetUser} ` : ''
    } else {
      localContent.value = ''
    }
  },
  { immediate: true }
)

const placeholder = computed(() =>
  props.targetUser ? `回复 @${props.targetUser}...` : '说点什么吧...',
)

/* ---------- 关闭 ---------- */
const close = () => {
  emit('update:show', false)
}

/* ---------- 提交 ---------- */
const submit = async () => {
  if (!localContent.value.trim()) return

  emit('submit', {
    content: localContent.value.trim(),
    parentId: props.parentId,   // 关键：传 parentId
  })

  // 可选：清空内容（防止重复提交）
  localContent.value = ''
}
</script>

<style scoped>
/* 遮罩 */
.reply-modal-overlay {
  position: fixed;
  inset: 0;
  background: rgba(0, 0, 0, 0.45);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 9999;
}

/* 模态框 */
.reply-modal {
  width: 90%;
  max-width: 520px;
  background: #fff;
  border-radius: 8px;
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.15);
  overflow: hidden;
}

/* 头部 */
.modal-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 16px;
  border-bottom: 1px solid #eee;
  background: #f8f9fa;
}
.modal-header h3 {
  margin: 0;
  font-size: 1.1rem;
  font-weight: 600;
}
.close-btn {
  background: none;
  border: none;
  font-size: 1.4rem;
  cursor: pointer;
  color: #999;
}

/* 文本域 */
.reply-textarea {
  width: 100%;
  min-height: 120px;
  padding: 12px;
  border: none;
  resize: vertical;
  font-size: 0.95rem;
  outline: none;
}

/* 底部按钮 */
.modal-footer {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
  padding: 12px 16px;
  border-top: 1px solid #eee;
  background: #f8f9fa;
}
.cancel-btn,
.submit-btn {
  padding: 6px 16px;
  border-radius: 4px;
  font-size: 0.9rem;
  cursor: pointer;
}
.cancel-btn {
  background: #fff;
  border: 1px solid #ddd;
  color: #333;
}
.submit-btn {
  background: #4c91d9;
  border: none;
  color: #fff;
}
.submit-btn:disabled,
.cancel-btn:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

/* 动画 */
.modal-enter-active,
.modal-leave-active {
  transition: all 0.2s ease;
}
.modal-enter-from,
.modal-leave-to {
  opacity: 0;
  transform: scale(0.95);
}
.modal-enter-to,
.modal-leave-from {
  opacity: 1;
  transform: scale(1);
}

/* 暗黑模式 */
@media (prefers-color-scheme: dark) {
  .reply-modal-overlay {
    background: rgba(0, 0, 0, 0.7);
  }
  .reply-modal {
    background: #222;
  }
  .modal-header,
  .modal-footer {
    background: #2a2a2a;
    border-color: #444;
  }
  .reply-textarea {
    background: #222;
    color: #ddd;
  }
  .cancel-btn {
    background: #222;
    border-color: #444;
    color: #ddd;
  }
}
</style>
