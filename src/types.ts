// src/types.ts
export interface Forum {
  id: number
  name: string
  description: string
  postCount: number
  avatar?: string
  ownerId?: number
  ownerName?: string
}

export interface Post {
  id: number
  forumId: number
  forumName: string
  title: string
  content: string
  replyCount: number
  author: string
  authorId: number
  authorAvatar?: string
  createdAt: number
  likeCount: number
  collectCount: number
  isLiked: boolean
  isCollected: boolean
  lastRepliedAt: number
  isFeatured: boolean
}

export interface CreatePost {
  forumId: number
  title: string
  content: string
  authorId: number
}

export interface Reply {
  id: number
  postId: number
  author: string
  authorId: number
  content: string
  floor?: number
  createdAt: number
  likeCount: number
  isLiked: boolean
  parentId?: number // 父回复 ID（null 表示一级回复）
  children?: Reply[]
  replyCount?: number
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
  id: number
  username: string
  email: string
  desc: string
  sex: number
  nickname: string
  avatar?: string
  createdAt: number
}
