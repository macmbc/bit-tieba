/* eslint-disable @typescript-eslint/no-explicit-any */
// src/api/backendForumApi.ts
import axios from 'axios'
import type { Forum, Post, Reply } from '@/types'

const api = axios.create({
  baseURL: '/api',
  headers: {
    'Content-Type': 'application/json',
  },
  timeout: 10000,
})

// 响应拦截器
api.interceptors.response.use(
  (response) => response.data,
  (error) => {
    console.error('API请求失败:', error)
    return Promise.reject(error)
  }
)

// ==================== 数据转换函数 ====================

function transformForum(data: any): Forum {
  return {
    id: data.forum_id,
    name: data.name,
    description: data.description || '',
    postCount: data.post_count || 0,
    followerCount: data.follower_count || 0,
    avatar:data.avatar
  }
}

function transformPost(data: any): Post {
  console.log("forum:",data)
  return {
    id: data.post_id,
    forumId: data.forum_id,
    forumName: data.forum_name || '',
    title: data.title,
    content: data.content || data.content_preview || '',
    replyCount: data.reply_count || 0,
    author: data.author || '',
    authorId: data.uid?.toString() || '',
    createdAt: new Date(data.created_at).getTime(),
    likeCount: data.like_count || 0,
    collectCount: data.collect_count || 0,
    isLiked: data.is_liked || false,
    isCollected: data.is_collected || false,
  }
}

function transformReply(data: any): Reply {
  console.log("reply_data:",data)
  return {
    id: data.reply_id,
    postId: data.post_id,
    author: data.author || '',
    authorId: data.uid?.toString() || '',
    content: data.content,
    floor: data.floor || 0,
    createdAt: new Date(data.created_at).getTime(),
    likeCount: data.like_count || 0,
    isLiked: data.is_liked || false,
    parentId: data.parent_reply_id || undefined,
    children: data.sub_replies?.map(transformReply) || [],
    replyCount: data.sub_replies?.length || 0,
  }
}

// ==================== 2.2 贴吧管理 ====================

export interface GetForumsParams {
  page: number
  limit: number
}

export const getForums = async (params: GetForumsParams): Promise<{ error: number; forums: Forum[] }> => {
  const response: any = await api.get('/forums', { params })
  console.log("forums",response)
  return {
    error: response.error,
    forums: (response.forums || []).map(transformForum),
  }
}

export interface GetForumDetailParams {
  forum_id: number
  uid?: number
  token?: string
}

export const getForumDetail = async (params: GetForumDetailParams): Promise<{ error: number; forum: Forum }> => {
  const response: any = await api.get('/forum/detail', { params })
  return {
    error: response.error,
    forum: transformForum(response.forum),
  }
}

export interface FollowForumParams {
  uid: number
  token: string
  forum_id: number
}

export const followForum = async (data: FollowForumParams): Promise<{ error: number }> => {
  return api.post('/follow_forum', data)
}

export const unfollowForum = async (data: FollowForumParams): Promise<{ error: number }> => {
  return api.post('/unfollow_forum', data)
}

export interface GetFollowedForumsParams {
  uid: number
  token: string
  page: number
  limit: number
}

export const getFollowedForums = async (params: GetFollowedForumsParams): Promise<{ error: number; forums: Forum[] }> => {
  const response: any = await api.get('/followed_forums', { params })
  return {
    error: response.error,
    forums: (response.forums || []).map(transformForum),
  }
}

// ==================== 2.3 帖子管理 ====================

export interface GetPostsParams {
  forum_id?: number
  page: number
  limit: number
  sort?: 'time' | 'hot' | 'reply'
}

export const getPosts = async (params: GetPostsParams): Promise<{ error: number; posts: Post[] }> => {
  console.log(params)
  const response: any = await api.get('/posts', { params })
  console.log("posts:",response)
  return {
    error: response.error,
    posts: (response.posts || []).map(transformPost),
  }
}

export interface GetPostDetailParams {
  post_id: number
  uid?: number
  token?: string
}

export const getPostDetail = async (params: GetPostDetailParams): Promise<{ error: number; post: Post }> => {
  const response: any = await api.get('/post/detail', { params })
  console.log('posts:', response)

  return {
    error: response.error,
    post: transformPost(response.post),
  }
}

export interface CreatePostParams {
  uid: number
  token: string
  forum_id: number
  title: string
  content: string
}

export const createPost = async (data: CreatePostParams): Promise<{ error: number; post_id: number }> => {
  return api.post('/post/create', data)
}

export interface UpdatePostParams {
  uid: number
  token: string
  post_id: number
  title: string
  content: string
}

export const updatePost = async (data: UpdatePostParams): Promise<{ error: number }> => {
  return api.post('/post/update', data)
}

export interface DeletePostParams {
  uid: number
  token: string
  post_id: number
}

export const deletePost = async (data: DeletePostParams): Promise<{ error: number }> => {
  return api.post('/post/delete', data)
}

export interface LikePostParams {
  uid: number
  token: string
  post_id: number
}

export const likePost = async (data: LikePostParams): Promise<{ error: number }> => {
  return api.post('/like_post', data)
}

export const unlikePost = async (data: LikePostParams): Promise<{ error: number }> => {
  return api.post('/unlike_post', data)
}

export const collectPost = async (data: LikePostParams): Promise<{ error: number }> => {
  return api.post('/collect_post', data)
}

export const uncollectPost = async (data: LikePostParams): Promise<{ error: number }> => {
  return api.post('/uncollect_post', data)
}

export interface SetTopPostParams {
  uid: number
  token: string
  post_id: number
  is_top: boolean
}

export const setTopPost = async (data: SetTopPostParams): Promise<{ error: number }> => {
  return api.post('/set_top_post', data)
}

export interface SetEssencePostParams {
  uid: number
  token: string
  post_id: number
  is_essence: boolean
}

export const setEssencePost = async (data: SetEssencePostParams): Promise<{ error: number }> => {
  return api.post('/set_essence_post', data)
}

// ==================== 2.4 回复管理 ====================

export interface GetRepliesParams {
  post_id: number
  page?: number
  limit?: number
  sort?: 'time' | 'hot'
  uid?: number
  token?: string
}

export const getReplies = async (params: GetRepliesParams): Promise<{ error: number; replies: Reply[] }> => {
  const response: any = await api.get('/post/replies', { params })
  console.log(response.replies)
  return {
    error: response.error,
    replies: (response.replies || []).map(transformReply),
  }
}

export interface CreateReplyParams {
  uid: number
  token: string
  post_id: number
  content: string
  parent_reply_id?: number
}

export const createReply = async (data: CreateReplyParams): Promise<{ error: number; reply_id: number }> => {
  return api.post('/reply', data)
}

export interface DeleteReplyParams {
  uid: number
  token: string
  reply_id: number
}

export const deleteReply = async (data: DeleteReplyParams): Promise<{ error: number }> => {
  return api.post('/reply/delete', data)
}

export interface LikeReplyParams {
  uid: number
  token: string
  reply_id: number
}

export const likeReply = async (data: LikeReplyParams): Promise<{ error: number }> => {
  return api.post('/like_reply', data)
}

export const unlikeReply = async (data: LikeReplyParams): Promise<{ error: number }> => {
  return api.post('/unlike_reply', data)
}

// ==================== 2.5 搜索功能 ====================

export interface SearchParams {
  keyword: string
  type?: 'post' | 'forum' | 'user'
  forum_id?: number
  page?: number
  limit?: number
}

export const search = async (params: SearchParams): Promise<{ error: number; results: any[] }> => {
  return api.get('/search', { params })
}

// ==================== 2.6 用户内容管理 ====================

export interface GetMyPostsParams {
  uid: number
  token: string
  page: number
  limit: number
}

export const getMyPosts = async (params: GetMyPostsParams): Promise<{ error: number; posts: Post[] }> => {
  const response: any = await api.get('/my/posts', { params })
  return {
    error: response.error,
    posts: (response.posts || []).map(transformPost),
  }
}

export const getMyReplies = async (params: GetMyPostsParams): Promise<{ error: number; replies: Reply[] }> => {
  const response: any = await api.get('/my/replies', { params })
  return {
    error: response.error,
    replies: (response.replies || []).map(transformReply),
  }
}

export const getMyCollections = async (params: GetMyPostsParams): Promise<{ error: number; posts: Post[] }> => {
  const response: any = await api.get('/my/collections', { params })
  return {
    error: response.error,
    posts: (response.posts || []).map(transformPost),
  }
}

export const getMyFollowers = async (params: GetMyPostsParams): Promise<{ error: number; users: any[] }> => {
  return api.get('/my/followers', { params })
}

export const getMyFollowing = async (params: GetMyPostsParams): Promise<{ error: number; users: any[] }> => {
  return api.get('/my/following', { params })
}