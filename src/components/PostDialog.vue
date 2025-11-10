<template>
  <div v-if="show" class="dialog-overlay">
    <div class="dialog">
      <div class="dialog-header">
        <h2>发布新帖子</h2>
        <button class="close-btn" @click="closeDialog">×</button>
      </div>
      <form @submit.prevent="submitPost" class="post-form">
        <div class="form-group">
          <label for="title">标题</label>
          <input
            id="title"
            v-model="post.title"
            type="text"
            placeholder="请输入帖子标题"
            required
          />
        </div>
        <div class="form-group">
          <label for="content">内容</label>
          <textarea
            id="content"
            v-model="post.content"
            placeholder="请输入帖子内容"
            required
          ></textarea>
        </div>
        <div class="form-actions">
          <button type="submit" :disabled="submitting">发布</button>
          <button type="button" @click="closeDialog" :disabled="submitting">取消</button>
        </div>
        <div v-if="error" class="error">{{ error }}</div>
      </form>
    </div>
  </div>
</template>

<script lang="ts" setup>
import { ref } from 'vue'
import { createPostApi } from '@/api/forumApi' // 假设有此 API
import type { CreatePost } from '@/types'
import { useUserStore } from '@/stores/user';

const props = defineProps<{
  show: boolean
  forumId: number
}>()

const emit = defineEmits<{
  (e: 'close'): void
  (e: 'postCreated'): void
}>()

const post = ref<CreatePost>({
  forumId: props.forumId, // 初始化 forumId
  title: '',
  content: '',
  authorId: useUserStore.userId, // 初始化 authorId
})
const submitting = ref(false)
const error = ref('')

const closeDialog = () => {
  post.value = { title: '', content: '' } // 重置表单
  error.value = ''
  emit('close')
}

const submitPost = async () => {
  if (!post.value.title || !post.value.content) {
    error.value = '标题和内容不能为空'
    return
  }

  submitting.value = true
  try {
    await createPostApi(props.forumId, {
      title: post.value.title,
      content: post.value.content,
    })
    emit('postCreated') // 通知父组件帖子创建成功
    closeDialog()
  } catch (err) {
    error.value = `发布失败：${(err as Error).message}`
  } finally {
    submitting.value = false
  }
}
</script>

<style scoped>
.dialog-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.5);
  display: flex;
  justify-content: center;
  align-items: center;
  z-index: 2000;
}

.dialog {
  background: #fff;
  border-radius: 8px;
  width: 100%;
  max-width: 500px;
  padding: 20px;
  box-shadow: 0 4px 10px rgba(0, 0, 0, 0.2);
}

.dialog-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
}

.dialog-header h2 {
  margin: 0;
  font-size: 1.5rem;
  color: #333;
}

.close-btn {
  background: none;
  border: none;
  font-size: 1.5rem;
  cursor: pointer;
  color: #666;
}

.post-form {
  display: flex;
  flex-direction: column;
  gap: 15px;
}

.form-group {
  display: flex;
  flex-direction: column;
}

.form-group label {
  font-size: 0.9rem;
  color: #333;
  margin-bottom: 5px;
}

.form-group input,
.form-group textarea {
  padding: 8px;
  border: 1px solid #ddd;
  border-radius: 4px;
  font-size: 1rem;
}

.form-group textarea {
  min-height: 100px;
  resize: vertical;
}

.form-actions {
  display: flex;
  gap: 10px;
}

.form-actions button {
  padding: 8px 16px;
  border: none;
  border-radius: 4px;
  cursor: pointer;
}

.form-actions button[type="submit"] {
  background: #4c91d9;
  color: white;
}

.form-actions button[type="button"] {
  background: #ccc;
  color: #333;
}

.form-actions button:disabled {
  background: #eee;
  cursor: not-allowed;
}

.error {
  color: red;
  font-size: 0.9rem;
  text-align: center;
}

@media (prefers-color-scheme: dark) {
  .dialog {
    background: #222;
  }
  .dialog-header h2,
  .form-group label {
    color: #ddd;
  }
  .form-group input,
  .form-group textarea {
    background: #333;
    border-color: #444;
    color: #ddd;
  }
  .close-btn {
    color: #aaa;
  }
  .form-actions button[type="submit"] {
    background: #6ab0ff;
  }
  .form-actions button[type="button"] {
    background: #666;
    color: #ddd;
  }
  .error {
    color: #ff6347;
  }
}
</style>