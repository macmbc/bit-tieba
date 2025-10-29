// src/stores/post.ts
import { defineStore } from 'pinia'

// ---------------------------
// 类型定义部分
// ---------------------------

// 评论类型
export interface Comment {
  id: number
  author: string
  content: string
  time: string
}

// 帖子类型
export interface Post {
  id: number
  title: string
  content: string
  author: string
  time: string
  comments: Comment[]
}

// 新增帖子时的输入类型（不包含自动生成的字段）
export type NewPost = Omit<Post, 'id' | 'time' | 'comments'>

// 新增评论时的输入类型
export type NewComment = Omit<Comment, 'id' | 'time'>

// ---------------------------
// Store 定义
// ---------------------------

export const usePostStore = defineStore('post', {
  state: () => ({
    posts: [
      {
        id: 1,
        title: 'Vue 3 有哪些新特性？',
        content: '大家觉得 Vue 3 相比 Vue 2 有什么优势？',
        author: '前端学习者',
        time: '2023-10-01 10:00',
        comments: [{ id: 101, author: '张三', content: 'Composition API 很好用！', time: '10:05' }],
      },
    ] as Post[], // 指定 posts 的类型
  }),

  actions: {
    // 发布新帖子
    addPost(post: NewPost) {
      const newPost: Post = {
        id: Date.now(),
        ...post,
        time: new Date().toLocaleString(),
        comments: [],
      }
      this.posts.unshift(newPost)
    },

    // 添加评论
    addComment(postId: number, comment: NewComment) {
      const post = this.posts.find((p) => p.id === postId)
      if (post) {
        const newComment: Comment = {
          id: Date.now(),
          ...comment,
          time: new Date().toLocaleString(),
        }
        post.comments.push(newComment)
      }
    },
  },

  getters: {
    getPostById: (state) => {
      return (id: number) => state.posts.find((post) => post.id === id)
    },
  },
})
