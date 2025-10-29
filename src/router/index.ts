import { createRouter, createWebHistory } from 'vue-router'
import Home from '../views/HomeView.vue'
import Forums from '@/views/ForumsView.vue'
import Message from '@/views/MessagesView.vue'
import Me from '@/views/MeView.vue'
import SingleForum from '@/views/SingleForumView.vue'
import Post from '@/views/PostView.vue'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      redirect: '/home',
    },
    {
      path: '/home',
      name: 'home',
      component: Home,
      meta: { title: 'BIT-Tieba - 首页' },
    },
    {
      path: '/forums',
      name: 'forums',
      component: Forums,
      meta: { title: 'BIT-Tieba - 我的关注吧' },
    },
    {
      path: '/message',
      name: 'message',
      component: Message,
      meta: { title: 'BIT-Tieba - 我的消息' },
    },
    {
      path: '/me',
      name: 'me',
      component: Me,
      meta: { title: 'BIT-Tieba - 我的' },
    },
    {
      path: '/forum/:forumId',
      name: 'singleForum',
      component: SingleForum,
      meta: { title: 'BIT-Tieba - 贴吧详情' },
    },
    {
      path: '/forum/:forumId/post/:postId',
      name: 'post',
      component: Post,
      meta: { title: 'BIT-Tieba - 帖子详情' },
    },
  ],
})

router.beforeEach((to, from, next) => {
  document.title = to.meta.title || 'BIT-Tieba'
  next()
})

export default router
