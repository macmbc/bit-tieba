// src/api/forumApi.ts - 改造后对接真实后端
import type { Forum, Post, Message, UserInfo, Reply } from '@/types'
import * as backendApi from './backendForumApi'

// ==================== 辅助函数 ====================
function getToken(): string {
  const session = localStorage.getItem('user_session')
  if (!session) throw new Error('未登录')
  const { token } = JSON.parse(session)
  return token
}

function getUid(): number {
  const session = localStorage.getItem('user_session')
  if (!session) throw new Error('未登录')
  const { uid } = JSON.parse(session)
  return uid
}

// ==================== 2.2 贴吧管理 ====================

export const getHotForums = async (): Promise<Forum[]> => {
  const { forums } = await backendApi.getForums({ page: 1, limit: 20 })
  return forums
}

export const getFollowedForums = async (userId?: string): Promise<Forum[]> => {
  if (!userId) return []
  const { forums } = await backendApi.getFollowedForums({
    uid: getUid(),
    token: getToken(),
    page: 1,
    limit: 100,
  })
  return forums
}

export const followForumApi = async (userId: string, forumId: number): Promise<void> => {
  await backendApi.followForum({
    uid: getUid(),
    token: getToken(),
    forum_id: forumId,
  })
}

export const unfollowForumApi = async (userId: string, forumId: number): Promise<void> => {
  await backendApi.unfollowForum({
    uid: getUid(),
    token: getToken(),
    forum_id: forumId,
  })
}

export const getForum = async (forumId: number): Promise<Forum> => {
  const { forum } = await backendApi.getForumDetail({ forum_id: forumId })
  return forum
}

// ==================== 2.3 帖子管理 ====================

export const getHotPosts = async (): Promise<Post[]> => {
  const { posts } = await backendApi.getPosts({ page: 1, limit: 20, sort: 'hot' })
  return posts
}

export const getFollowedPosts = async (userId?: string): Promise<Post[]> => {
  // 需要后端支持获取关注的吧的帖子，暂时返回空
  return []
}

export const getForumPosts = async (forumId: number, page: number): Promise<Post[]> => {
  const { posts } = await backendApi.getPosts({ forum_id: forumId, page, limit: 20 })
  return posts
}

export const getPost = async (forumId: number, postId: number, userId?: string): Promise<Post> => {
  const { post } = await backendApi.getPostDetail({
    post_id: postId,
    uid: userId ? getUid() : undefined,
    token: userId ? getToken() : undefined,
  })
  return post
}

export const likePost = async (userId: string, postId: number): Promise<void> => {
  await backendApi.likePost({ uid: getUid(), token: getToken(), post_id: postId })
}

export const unlikePost = async (userId: string, postId: number): Promise<void> => {
  await backendApi.unlikePost({ uid: getUid(), token: getToken(), post_id: postId })
}

export const collectPost = async (userId: string, postId: number): Promise<void> => {
  await backendApi.collectPost({ uid: getUid(), token: getToken(), post_id: postId })
}

export const uncollectPost = async (userId: string, postId: number): Promise<void> => {
  await backendApi.uncollectPost({ uid: getUid(), token: getToken(), post_id: postId })
}

// ==================== 2.4 回复管理 ====================

export const getReplies = async (
  postId: number,
  page: number,
  limitNumber?: number,
  parentId?: number,
): Promise<{ replies: Reply[]; total: number }> => {
  const { replies } = await backendApi.getReplies({
    post_id: postId,
    page,
    limit: limitNumber,
  })
  // TODO: 需要后端支持parentId参数和嵌套结构
  return { replies, total: replies.length }
}

export const createReply = async (
  userId: string,
  postId: number,
  content: string,
  parentId?: number,
): Promise<Reply> => {
  const { reply_id } = await backendApi.createReply({
    uid: getUid(),
    token: getToken(),
    post_id: postId,
    content,
    parent_reply_id: parentId,
  })
  
  return {
    id: reply_id,
    postId,
    author: '当前用户',
    authorId: userId,
    content,
    floor: 1,
    createdAt: Date.now(),
    likeCount: 0,
    isLiked: false,
    parentId,
  }
}

export const likeReply = async (userId: string, replyId: number): Promise<void> => {
  await backendApi.likeReply({ uid: getUid(), token: getToken(), reply_id: replyId })
}

export const unlikeReply = async (userId: string, replyId: number): Promise<void> => {
  await backendApi.unlikeReply({ uid: getUid(), token: getToken(), reply_id: replyId })
}

// ==================== 2.6 用户内容管理 ====================

export const getUserPosts = async (userId: string): Promise<Post[]> => {
  try {
    const { posts } = await backendApi.getMyPosts({
      uid: getUid(),
      token: getToken(),
      page: 1,
      limit: 100,
    })
    return posts
  } catch (error) {
    console.error('获取用户帖子失败:', error)
    return []
  }
}

// ==================== 消息相关（暂不支持） ====================

export const getMessages = async (userId?: string): Promise<Message[]> => {
  return []
}

export const markMessageAsRead = async (userId: string, messageId: number): Promise<void> => {}

export const getUnreadCount = async (userId?: string): Promise<number> => {
  return 0
}

export const markAllMessagesAsRead = async (
  userId: string,
  type?: Message['type'],
): Promise<void> => {}

export const getUserInfo = async (userId: string): Promise<UserInfo> => {
  // 从 authStore 获取用户信息
  const session = localStorage.getItem('user_session')
  if (session) {
    const { email } = JSON.parse(session)
    return {
      username: email?.split('@')[0] || '用户',
      bio: '这个人很懒，什么都没写~',
      createdAt: Date.now() - 30 * 86400 * 1000,
    }
  }
  return {
    username: '用户',
    bio: '',
    createdAt: Date.now(),
  }
}