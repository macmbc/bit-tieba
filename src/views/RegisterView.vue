<template>
  <div class="register-page">
    <div class="register-container">
      <div class="register-header">
        <h1>BIT-Tieba</h1>
        <p>创建新账号</p>
      </div>

      <form class="register-form" @submit.prevent="handleRegister">
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

        <!-- 密码输入 -->
        <div class="form-group">
          <label for="password">密码 *</label>
          <input
            id="password"
            v-model="formData.passwd"
            type="password"
            placeholder="请输入密码"
            required
            :disabled="loading"
          />
        </div>

        <!-- 确认密码输入 -->
        <div class="form-group">
          <label for="confirm-password">确认密码 *</label>
          <input
            id="confirm-password"
            v-model="formData.confirm"
            type="password"
            placeholder="请再次输入密码"
            required
            :disabled="loading"
          />
        </div>

        <!-- 头像选择 (可选) -->
        <div class="form-group">
          <label for="icon">头像 (可选)</label>
          <input
            id="icon"
            v-model="formData.icon"
            type="text"
            placeholder="头像URL或路径"
            :disabled="loading"
          />
        </div>

        <!-- 错误/成功提示 -->
        <div v-if="message.text" :class="['message', message.type]">
          {{ message.text }}
        </div>

        <!-- 注册按钮 -->
        <button type="submit" class="register-btn" :disabled="loading">
          {{ loading ? '注册中...' : '注册' }}
        </button>

        <!-- 底部链接 -->
        <div class="form-footer">
          已有账号？
          <router-link to="/login" class="link">立即登录</router-link>
        </div>
      </form>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive } from 'vue'
import { useRouter } from 'vue-router'
import { getVerifyCode, userRegister, getErrorMessage, ERROR_CODES } from '@/api/authApi'

const router = useRouter()

const formData = reactive({
  email: '',
  user: '',
  passwd: '',
  confirm: '',
  icon: '',
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

// 注册
const handleRegister = async () => {
  if (loading.value) return

  // 验证密码一致性
  if (formData.passwd !== formData.confirm) {
    showMessage('两次输入的密码不一致', 'error')
    return
  }

  loading.value = true
  message.text = ''

  try {
    const response = await userRegister({
      email: formData.email,
      user: formData.user,
      passwd: formData.passwd,
      confirm: formData.confirm,
      icon: formData.icon || undefined,
      varifycode: formData.varifycode,
    })

    if (response.error === ERROR_CODES.SUCCESS) {
      showMessage('注册成功！即将跳转到登录页面...', 'success')
      setTimeout(() => {
        router.push('/login')
      }, 2000)
    } else {
      showMessage(getErrorMessage(response.error), 'error')
    }
  } catch (error: any) {
    console.error('注册失败:', error)
    showMessage(error.response?.data?.message || '注册失败，请稍后重试', 'error')
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
.register-page {
  min-height: 100vh;
  display: flex;
  justify-content: center;
  align-items: center;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  padding: 20px;
}

.register-container {
  background: white;
  border-radius: 12px;
  box-shadow: 0 10px 40px rgba(0, 0, 0, 0.1);
  padding: 40px;
  width: 100%;
  max-width: 450px;
  max-height: 90vh;
  overflow-y: auto;
}

.register-header {
  text-align: center;
  margin-bottom: 30px;
}

.register-header h1 {
  font-size: 2rem;
  color: #333;
  margin-bottom: 10px;
}

.register-header p {
  font-size: 1rem;
  color: #666;
}

.register-form {
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

.register-btn {
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

.register-btn:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4);
}

.register-btn:active:not(:disabled) {
  transform: translateY(0);
}

.register-btn:disabled {
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

/* 响应式 */
@media (max-width: 480px) {
  .register-container {
    padding: 30px 20px;
  }

  .register-header h1 {
    font-size: 1.5rem;
  }

  .verify-code-group {
    flex-direction: column;
  }
}

/* 暗模式 */
@media (prefers-color-scheme: dark) {
  .register-page {
    background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
  }

  .register-container {
    background: #222;
    color: #ddd;
  }

  .register-header h1 {
    color: #ddd;
  }

  .register-header p {
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
