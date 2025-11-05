// src/types.ts
export interface Forum {
  id: number
  name: string
  description: string
  postCount: number
}

export interface Post {
  id: number
  forumId: number
  forumName: string
  title: string
  content: string
  replyCount: number
  author: string
  authorId: string
  createdAt: number
  likeCount: number
  collectCount: number
  isLiked: boolean
  isCollected: boolean
}

export interface Reply {
  id: number
  postId: number
  author: string
  authorId: string
  content: string
  floor?: number
  createdAt: number
  likeCount: number
  isLiked: boolean
  parentId?: number // 父回复 ID（null 表示一级回复）
  children?: Reply[]
  replyCount?: number
}

export interface User {
  id: string // 用户唯一ID（一般为UUID或数据库主键）
  username: string // 用户名（唯一）
  avatar?: string // 头像URL（可选）
  bio?: string // 简介或签名（可选）
  createdAt: number
}

export interface Message {
  id: number
  type: 'post_reply' | 'comment_reply' | 'mention' | 'system' | 'private_message' | 'like_collect'
  sender: string
  content: string
  timestamp: number
  source: string
  isRead: boolean
  link: string
}

export interface UserInfo {
  username: string
  bio: string
  avatar?: string
  createdAt: number
}
