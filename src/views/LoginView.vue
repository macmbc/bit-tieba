<template>
  <div class="login-page">
    <div class="login-container">
      <div class="login-header">
        <h1>BIT-Tieba</h1>
        <p>登录你的账号</p>
      </div>

      <form class="login-form" @submit.prevent="handleLogin">
        <!-- 邮箱输入 -->
        <div class="form-group">
          <label for="email">邮箱</label>
          <input
            id="email"
            v-model="formData.email"
            type="email"
            placeholder="请输入邮箱"
            required
            :disabled="loading"
          />
        </div>

        <!-- 密码输入 -->
        <div class="form-group">
          <label for="password">密码</label>
          <input
            id="password"
            v-model="formData.passwd"
            type="password"
            placeholder="请输入密码"
            required
            :disabled="loading"
          />
        </div>

        <!-- 错误提示 -->
        <div v-if="errorMessage" class="error-message">
          {{ errorMessage }}
        </div>

        <!-- 登录按钮 -->
        <button type="submit" class="login-btn" :disabled="loading">
          {{ loading ? '登录中...' : '登录' }}
        </button>

        <!-- 底部链接 -->
        <div class="form-footer">
          <router-link to="/reset-password" class="link">忘记密码？</router-link>
          <span class="divider">|</span>
          <router-link to="/register" class="link">注册账号</router-link>
        </div>
      </form>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive } from 'vue'
import { useRouter } from 'vue-router'
import { userLogin, getErrorMessage, ERROR_CODES } from '@/api/authApi'
import { useAuthStore } from '@/stores/authStore'

const router = useRouter()
const authStore = useAuthStore()

const formData = reactive({
  email: '',
  passwd: '',
})

const loading = ref(false)
const errorMessage = ref('')

const handleLogin = async () => {
  if (loading.value) return

  errorMessage.value = ''
  loading.value = true

  try {
    const response = await userLogin({
      email: formData.email,
      passwd: formData.passwd,
    })

    if (response.error === ERROR_CODES.SUCCESS) {
      // 登录成功
      authStore.login(response)
      // 跳转到首页
      router.push('/home')
    } else {
      // 显示错误信息
      errorMessage.value = getErrorMessage(response.error)
    }
  } catch (error: any) {
    console.error('登录失败:', error)
    errorMessage.value = error.response?.data?.message || '登录失败，请检查网络连接'
  } finally {
    loading.value = false
  }
}
</script>

<style scoped>
.login-page {
  min-height: 100vh;
  display: flex;
  justify-content: center;
  align-items: center;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  padding: 20px;
}

.login-container {
  background: white;
  border-radius: 12px;
  box-shadow: 0 10px 40px rgba(0, 0, 0, 0.1);
  padding: 40px;
  width: 100%;
  max-width: 400px;
}

.login-header {
  text-align: center;
  margin-bottom: 30px;
}

.login-header h1 {
  font-size: 2rem;
  color: #333;
  margin-bottom: 10px;
}

.login-header p {
  font-size: 1rem;
  color: #666;
}

.login-form {
  display: flex;
  flex-direction: column;
  gap: 20px;
}

.form-group {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.form-group label {
  font-size: 0.9rem;
  color: #333;
  font-weight: 500;
}

.form-group input {
  padding: 12px;
  border: 1px solid #ddd;
  border-radius: 6px;
  font-size: 1rem;
  transition: border-color 0.3s;
}

.form-group input:focus {
  outline: none;
  border-color: #667eea;
}

.form-group input:disabled {
  background: #f5f5f5;
  cursor: not-allowed;
}

.error-message {
  padding: 10px;
  background: #fee;
  border: 1px solid #fcc;
  border-radius: 6px;
  color: #c33;
  font-size: 0.9rem;
  text-align: center;
}

.login-btn {
  padding: 14px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  border: none;
  border-radius: 6px;
  font-size: 1rem;
  font-weight: 600;
  cursor: pointer;
  transition: transform 0.2s, box-shadow 0.2s;
}

.login-btn:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4);
}

.login-btn:active:not(:disabled) {
  transform: translateY(0);
}

.login-btn:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

.form-footer {
  text-align: center;
  font-size: 0.9rem;
  color: #666;
  margin-top: 10px;
}

.form-footer .link {
  color: #667eea;
  text-decoration: none;
  font-weight: 500;
  transition: color 0.2s;
}

.form-footer .link:hover {
  color: #764ba2;
  text-decoration: underline;
}

.form-footer .divider {
  margin: 0 10px;
  color: #ccc;
}

/* 响应式 */
@media (max-width: 480px) {
  .login-container {
    padding: 30px 20px;
  }

  .login-header h1 {
    font-size: 1.5rem;
  }
}

/* 暗模式 */
@media (prefers-color-scheme: dark) {
  .login-page {
    background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
  }

  .login-container {
    background: #222;
    color: #ddd;
  }

  .login-header h1 {
    color: #ddd;
  }

  .login-header p {
    color: #aaa;
  }

  .form-group label {
    color: #ddd;
  }

  .form-group input {
    background: #333;
    border-color: #444;
    color: #ddd;
  }

  .form-group input:focus {
    border-color: #667eea;
  }

  .form-group input:disabled {
    background: #2a2a2a;
  }
}
</style>
