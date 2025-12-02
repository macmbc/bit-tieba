<template>
  <div class="reset-password-page">
    <div class="reset-password-container">
      <div class="reset-password-header">
        <h1>BIT-Tieba</h1>
        <p>重置密码</p>
      </div>

      <form class="reset-password-form" @submit.prevent="handleResetPassword">
        <!-- 邮箱输入 -->
        <div class="form-group">
          <label for="email">邮箱 *</label>
          <input
            id="email"
            v-model="formData.email"
            type="email"
            placeholder="请输入邮箱"
            required
            :disabled="loading"
          />
        </div>

        <!-- 用户名输入 -->
        <div class="form-group">
          <label for="username">用户名 *</label>
          <input
            id="username"
            v-model="formData.user"
            type="text"
            placeholder="请输入用户名"
            required
            :disabled="loading"
          />
        </div>

        <!-- 验证码输入 -->
        <div class="form-group">
          <label for="verifycode">验证码 *</label>
          <div class="verify-code-group">
            <input
              id="verifycode"
              v-model="formData.varifycode"
              type="text"
              placeholder="请输入验证码"
              required
              maxlength="4"
              :disabled="loading"
            />
            <button
              type="button"
              class="get-code-btn"
              :disabled="loading || countdown > 0"
              @click="handleGetVerifyCode"
            >
              {{ countdown > 0 ? `${countdown}秒后重试` : '获取验证码' }}
            </button>
          </div>
        </div>

        <!-- 新密码输入 -->
        <div class="form-group">
          <label for="new-password">新密码 *</label>
          <input
            id="new-password"
            v-model="formData.passwd"
            type="password"
            placeholder="请输入新密码"
            required
            :disabled="loading"
          />
        </div>

        <!-- 错误/成功提示 -->
        <div v-if="message.text" :class="['message', message.type]">
          {{ message.text }}
        </div>

        <!-- 重置按钮 -->
        <button type="submit" class="reset-btn" :disabled="loading">
          {{ loading ? '重置中...' : '重置密码' }}
        </button>

        <!-- 底部链接 -->
        <div class="form-footer">
          <router-link to="/login" class="link">返回登录</router-link>
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
import { getVerifyCode, resetPassword, getErrorMessage, ERROR_CODES } from '@/api/authApi'

const router = useRouter()

const formData = reactive({
  email: '',
  user: '',
  passwd: '',
  varifycode: '',
})

const loading = ref(false)
const countdown = ref(0)
const message = reactive({
  text: '',
  type: '' as 'error' | 'success' | '',
})

let countdownTimer: number | null = null

// 获取验证码
const handleGetVerifyCode = async () => {
  if (!formData.email) {
    showMessage('请先输入邮箱', 'error')
    return
  }

  const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/
  if (!emailRegex.test(formData.email)) {
    showMessage('请输入有效的邮箱地址', 'error')
    return
  }

  loading.value = true
  message.text = ''

  try {
    const response = await getVerifyCode({ email: formData.email })

    if (response.error === ERROR_CODES.SUCCESS) {
      showMessage('验证码已发送至您的邮箱', 'success')
      startCountdown()
    } else {
      showMessage(getErrorMessage(response.error), 'error')
    }
  } catch (error: any) {
    console.error('获取验证码失败:', error)
    showMessage('获取验证码失败，请检查网络连接', 'error')
  } finally {
    loading.value = false
  }
}

// 开始倒计时
const startCountdown = () => {
  countdown.value = 60
  if (countdownTimer) clearInterval(countdownTimer)

  countdownTimer = window.setInterval(() => {
    countdown.value--
    if (countdown.value <= 0) {
      if (countdownTimer) clearInterval(countdownTimer)
    }
  }, 1000)
}

// 重置密码
const handleResetPassword = async () => {
  if (loading.value) return

  loading.value = true
  message.text = ''

  try {
    const response = await resetPassword({
      email: formData.email,
      user: formData.user,
      passwd: formData.passwd,
      varifycode: formData.varifycode,
    })

    if (response.error === ERROR_CODES.SUCCESS) {
      showMessage('密码重置成功！即将跳转到登录页面...', 'success')
      setTimeout(() => {
        router.push('/login')
      }, 2000)
    } else {
      showMessage(getErrorMessage(response.error), 'error')
    }
  } catch (error: any) {
    console.error('重置密码失败:', error)
    showMessage(error.response?.data?.message || '重置密码失败，请稍后重试', 'error')
  } finally {
    loading.value = false
  }
}

// 显示消息
const showMessage = (text: string, type: 'error' | 'success') => {
  message.text = text
  message.type = type
}
</script>

<style scoped>
.reset-password-page {
  min-height: 100vh;
  display: flex;
  justify-content: center;
  align-items: center;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  padding: 20px;
}

.reset-password-container {
  background: white;
  border-radius: 12px;
  box-shadow: 0 10px 40px rgba(0, 0, 0, 0.1);
  padding: 40px;
  width: 100%;
  max-width: 450px;
}

.reset-password-header {
  text-align: center;
  margin-bottom: 30px;
}

.reset-password-header h1 {
  font-size: 2rem;
  color: #333;
  margin-bottom: 10px;
}

.reset-password-header p {
  font-size: 1rem;
  color: #666;
}

.reset-password-form {
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

.verify-code-group {
  display: flex;
  gap: 10px;
}

.verify-code-group input {
  flex: 1;
}

.get-code-btn {
  padding: 12px 20px;
  background: #667eea;
  color: white;
  border: none;
  border-radius: 6px;
  font-size: 0.9rem;
  cursor: pointer;
  white-space: nowrap;
  transition: background 0.3s;
}

.get-code-btn:hover:not(:disabled) {
  background: #5568d3;
}

.get-code-btn:disabled {
  background: #ccc;
  cursor: not-allowed;
}

.message {
  padding: 10px;
  border-radius: 6px;
  font-size: 0.9rem;
  text-align: center;
}

.message.error {
  background: #fee;
  border: 1px solid #fcc;
  color: #c33;
}

.message.success {
  background: #efe;
  border: 1px solid #cfc;
  color: #3c3;
}

.reset-btn {
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

.reset-btn:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4);
}

.reset-btn:active:not(:disabled) {
  transform: translateY(0);
}

.reset-btn:disabled {
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
  .reset-password-container {
    padding: 30px 20px;
  }

  .reset-password-header h1 {
    font-size: 1.5rem;
  }

  .verify-code-group {
    flex-direction: column;
  }
}

/* 暗模式 */
@media (prefers-color-scheme: dark) {
  .reset-password-page {
    background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
  }

  .reset-password-container {
    background: #222;
    color: #ddd;
  }

  .reset-password-header h1 {
    color: #ddd;
  }

  .reset-password-header p {
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

  .get-code-btn {
    background: #5568d3;
  }

  .get-code-btn:hover:not(:disabled) {
    background: #667eea;
  }
}
</style>
