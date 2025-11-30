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

// ==================== 2.2 贴吧管理 ====================

export interface GetForumsParams {
  page: number
  limit: number
}

export const getForums = async (params: GetForumsParams): Promise<{ error: number; forums: Forum[] }> => {
  return api.get('/forums', { params })
}

export interface GetForumDetailParams {
  forum_id: number
  uid?: number
  token?: string
}

export const getForumDetail = async (params: GetForumDetailParams): Promise<{ error: number; forum: Forum }> => {
  return api.get('/forum/detail', { params })
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
  return api.get('/followed_forums', { params })
}

// ==================== 2.3 帖子管理 ====================

export interface GetPostsParams {
  forum_id?: number
  page: number
  limit: number
  sort?: 'time' | 'hot' | 'reply'
}

export const getPosts = async (params: GetPostsParams): Promise<{ error: number; posts: Post[] }> => {
  return api.get('/posts', { params })
}

export interface GetPostDetailParams {
  post_id: number
  uid?: number
  token?: string
}

export const getPostDetail = async (params: GetPostDetailParams): Promise<{ error: number; post: Post }> => {
  return api.get('/post/detail', { params })
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
  return api.get('/post/replies', { params })
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
  return api.get('/my/posts', { params })
}

export const getMyReplies = async (params: GetMyPostsParams): Promise<{ error: number; replies: Reply[] }> => {
  return api.get('/my/replies', { params })
}

export const getMyCollections = async (params: GetMyPostsParams): Promise<{ error: number; posts: Post[] }> => {
  return api.get('/my/collections', { params })
}

export const getMyFollowers = async (params: GetMyPostsParams): Promise<{ error: number; users: any[] }> => {
  return api.get('/my/followers', { params })
}

export const getMyFollowing = async (params: GetMyPostsParams): Promise<{ error: number; users: any[] }> => {
  return api.get('/my/following', { params })
}