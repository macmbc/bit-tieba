import { createRouter, createWebHistory } from 'vue-router'
import Home from '../views/HomeView.vue'
import Forums from '@/views/ForumsView.vue'
import Message from '@/views/MessagesView.vue'
import Me from '@/views/MeView.vue'
import SingleForum from '@/views/SingleForumView.vue'
import Post from '@/views/PostView.vue'
import Login from '@/views/LoginView.vue'
import Register from '@/views/RegisterView.vue'
import ResetPassword from '@/views/ResetPasswordView.vue'
import { useAuthStore } from '@/stores/authStore'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      redirect: '/login',
    },
    {
      path: '/login',
      name: 'login',
      component: Login,
      meta: { title: 'BIT-Tieba - 登录', requiresGuest: true },
    },
    {
      path: '/register',
      name: 'register',
      component: Register,
      meta: { title: 'BIT-Tieba - 注册', requiresGuest: true },
    },
    {
      path: '/reset-password',
      name: 'resetPassword',
      component: ResetPassword,
      meta: { title: 'BIT-Tieba - 重置密码', requiresGuest: true },
    },
    {
      path: '/home',
      name: 'home',
      component: Home,
      meta: { title: 'BIT-Tieba - 首页', requiresAuth: true },
    },
    {
      path: '/forums',
      name: 'forums',
      component: Forums,
      meta: { title: 'BIT-Tieba - 我的关注吧', requiresAuth: true },
    },
    {
      path: '/message',
      name: 'message',
      component: Message,
      meta: { title: 'BIT-Tieba - 我的消息', requiresAuth: true },
    },
    {
      path: '/me',
      name: 'me',
      component: Me,
      meta: { title: 'BIT-Tieba - 我的', requiresAuth: true },
    },
    {
     path: '/settings',
     name: 'settings',
     component: () => import('@/views/SettingsView.vue'),
     meta: { title: 'BIT-Tieba - 设置', requiresAuth: true },
   },
    {
      path: '/forum/:forumId',
      name: 'singleForum',
      component: SingleForum,
      meta: { title: 'BIT-Tieba - 贴吧详情', requiresAuth: true },
    },
    {
      path: '/forum/:forumId/post/:postId',
      name: 'post',
      component: Post,
      meta: { title: 'BIT-Tieba - 帖子详情', requiresAuth: true },
    },
  ],
})

router.beforeEach((to, from, next) => {
  document.title = to.meta.title || 'BIT-Tieba'

  const authStore = useAuthStore()
  authStore.initAuth()

  // 需要登录的页面
  if (to.meta.requiresAuth && !authStore.isLoggedIn) {
    next('/login')
    return
  }

  // 已登录用户访问登录/注册页面，重定向到首页
  if (to.meta.requiresGuest && authStore.isLoggedIn) {
    next('/home')
    return
  }

  next()
})

export default router
