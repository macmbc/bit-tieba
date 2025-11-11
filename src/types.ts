// src/types.ts
export interface Forum {
  id: number
  name: string
  description: string
  postCount: number
  avatar?: string
  ownerId?: number //吧主
  ownerName?: string
  isFollowed: true
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

// parentId为0时floor有意义，不为0时floor项不存在
export type Reply =
  | {
      id: number
      postId: number
      author: string
      authorId: number
      content: string
      parentId: 0 // 一级回复
      floor: number // 有意义
      createdAt: number
      likeCount: number
      isLiked: boolean
      replyCount?: number
      children?: Reply[] // 子回复
    }
  | {
      id: number
      postId: number
      author: string
      authorId: number
      content: string
      parentId: number // 二级回复
      floor?: never // 禁止使用 floor
      createdAt: number
      likeCount: number
      isLiked: boolean
    }

export interface CreateReply {
  postId: number
  authorId: number
  content: string
  parentId: number
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
  avatar?: string
  role: string | null //吧主与否
}
