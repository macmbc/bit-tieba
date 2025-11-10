// src/api/forumApi.ts
import axios from 'axios'
import type { Forum, Post, Message, UserInfo, Reply } from '@/types'
import { REPLY_CONSTANTS } from '@/constants/forum'

const api = axios.create({
  baseURL: '/api',
})

const mockData = {
  hotForums: [
    { id: 1, name: 'Vue.js 吧', description: '探讨 Vue.js 开发经验', postCount: 150 },
    { id: 2, name: '前端开发吧', description: '前端技术分享', postCount: 300 },
    { id: 3, name: 'JavaScript 吧', description: 'JS 语言深度探讨', postCount: 200 },
  ] as Forum[],

  hotPosts: [
    {
      id: 1,
      forumId: 1,
      forumName: 'Vue.js 吧',
      title: 'Vue 3 新特性',
      content: 'Composition API 真好用！',
      replyCount: 10,
      author: '用户123',
      authorId: 123,
      createdAt: Date.now() - 3600 * 1000 * 10, // 10小时前
      lastRepliedAt: Date.now() - 3600 * 1000 * 2, // 2小时前
      likeCount: 25,
      collectCount: 8,
      isLiked: true,
      isCollected: false,
      isFeatured: true,
    },
    {
      id: 2,
      forumId: 2,
      forumName: '前端开发吧',
      title: '如何优化性能',
      content: '分享前端优化技巧...',
      replyCount: 5,
      author: '用户456',
      authorId: 456,
      createdAt: Date.now() - 7200 * 1000, // 2小时前
      lastRepliedAt: Date.now() - 3600 * 1000, // 1小时前
      likeCount: 15,
      collectCount: 4,
      isLiked: false,
      isCollected: true,
    },
  ] as Post[],

  followedPosts: [
    {
      id: 3,
      forumId: 1,
      forumName: 'Vue.js 吧',
      title: 'Vue 项目实践',
      content: '一个真实的 Vue 项目案例...',
      replyCount: 8,
      author: '用户123',
      authorId:123,
      createdAt: Date.now() - 86400 * 1000, // 1天前
      lastRepliedAt: Date.now() - 3600 * 1000 * 0.5, // 0.5小时前
      likeCount: 10,
      collectCount: 3,
      isLiked: false,
      isCollected: false,
      isFeatured: false,
    },
  ] as Post[],

  followedForums: [
    { id: 1, name: 'Vue.js 吧', description: '探讨 Vue.js 开发经验', postCount: 150 },
    { id: 2, name: '前端开发吧', description: '前端技术分享', postCount: 300 },
  ] as Forum[],

  messages: [
    {
      id: 1,
      type: 'post_reply' as const,
      sender: '用户A',
      content: '你的帖子很棒！支持一下！',
      timestamp: Date.now() - 3600 * 1000,
      source: 'Vue.js 吧',
      isRead: false,
      link: '/forum/1/post/1',
    },
    {
      id: 2,
      type: 'comment_reply' as const,
      sender: '用户B',
      content: '回复你的评论：确实，Vue 3 很强大！',
      timestamp: Date.now() - 7200 * 1000,
      source: '前端开发吧',
      isRead: false,
      link: '/forum/2/post/2#comment-123',
    },
    {
      id: 3,
      type: 'mention' as const,
      sender: '用户C',
      content: '@你 在讨论 Vue 项目实践，快来看！',
      timestamp: Date.now() - 86400 * 1000,
      source: 'Vue.js 吧',
      isRead: true,
      link: '/forum/1/post/3#mention-456',
    },
    {
      id: 4,
      type: 'system' as const,
      sender: '系统',
      content: '你被任命为Vue.js吧吧主',
      timestamp: Date.now() - 2 * 86400 * 1000,
      source: '系统',
      isRead: false,
      link: '/settings',
    },
    {
      id: 5,
      type: 'private_message' as const,
      sender: '用户D',
      content: '嗨，想聊聊Vue项目吗？',
      timestamp: Date.now() - 3 * 86400 * 1000,
      source: '私信',
      isRead: false,
      link: '/messages/private/789',
    },
    {
      id: 6,
      type: 'like_collect' as const,
      sender: '用户E',
      content: '收藏了你的帖子“Vue 3 新特性”',
      timestamp: Date.now() - 4 * 86400 * 1000,
      source: 'Vue.js 吧',
      isRead: false,
      link: '/forum/1/post/1',
    },
  ] as Message[],

  userInfo: {
    id:6,
    username: '用户123',
    email: "321123@qq.com",
    desc: '前端爱好者，喜欢Vue和TypeScript',
    nickname:'好姐妹小木曾雪菜',
    avatar: '/uploads/avatars/neuro.jpg',
    createdAt: Date.now() - 30 * 86400 * 1000,
  } as UserInfo,

  userPosts: [
    {
      id: 1,
      forumId: 1,
      forumName: 'Vue.js 吧',
      title: 'Vue 3 新特性',
      content: 'Composition API 真好用！',
      replyCount: 10,
      author: '用户123',
      authorId: 123,
      createdAt: Date.now() - 3600 * 1000,
      lastRepliedAt: Date.now() - 1800 * 1000,
      likeCount: 25,
      collectCount: 8,
      isLiked: true,
      isCollected: false,
    },
    {
      id: 3,
      forumId: 1,
      forumName: 'Vue.js 吧',
      title: 'Vue 项目实践',
      content: '一个真实的 Vue 项目案例...',
      replyCount: 8,
      author: '用户123',
      authorId: 'u123',
      createdAt: Date.now() - 86400 * 1000,
      lastRepliedAt: Date.now() - 7200 * 1000,
      likeCount: 10,
      collectCount: 3,
      isLiked: false,
      isCollected: false,
    },
  ] as Post[],
}

export const getHotForums = async (): Promise<Forum[]> => {
  return new Promise((resolve) => {
    setTimeout(() => resolve(mockData.hotForums), 500)
  })
}

export const getHotPosts = async (): Promise<Post[]> => {
  return new Promise((resolve) => {
    setTimeout(() => resolve(mockData.hotPosts), 500)
  })
}

export const getFollowedPosts = async (userId?: string): Promise<Post[]> => {
  return new Promise((resolve) => {
    setTimeout(() => resolve(userId ? mockData.followedPosts : []), 500)
  })
}

export const getFollowedForums = async (userId?: string): Promise<Forum[]> => {
  return new Promise((resolve) => {
    setTimeout(() => resolve(userId ? mockData.followedForums : []), 500)
  })
}

export const unfollowForumApi = async (userId: string, forumId: number): Promise<void> => {
  return new Promise((resolve) => {
    setTimeout(() => {
      mockData.followedForums = mockData.followedForums.filter((f) => f.id !== forumId)
      resolve()
    }, 500)
  })
}

// 新增：获取消息列表
export const getMessages = async (userId?: string): Promise<Message[]> => {
  // 真实 API 示例
  // const response = await api.get(`/messages?userId=${userId}`);
  // return response.data;
  return new Promise((resolve) => {
    setTimeout(() => resolve(userId ? mockData.messages : []), 500)
  })
}

// 新增：标记消息为已读
export const markMessageAsRead = async (userId: string, messageId: number): Promise<void> => {
  // 真实 API 示例
  // await api.post('/messages/read', { userId, messageId });
  return new Promise((resolve) => {
    setTimeout(() => {
      mockData.messages = mockData.messages.map((msg) =>
        msg.id === messageId ? { ...msg, isRead: true } : msg,
      )
      resolve()
    }, 500)
  })
}

// 新增：获取未读消息数
export const getUnreadCount = async (userId?: string): Promise<number> => {
  // 真实 API 示例
  // const response = await api.get(`/messages/unread?userId=${userId}`);
  // return response.data.count;
  return new Promise((resolve) => {
    setTimeout(() => {
      const count = userId ? mockData.messages.filter((msg) => !msg.isRead).length : 0
      resolve(count)
    }, 500)
  })
}

export const markAllMessagesAsRead = async (
  userId: string,
  type?: Message['type'],
): Promise<void> => {
  // 真实 API 示例
  // await api.post('/messages/read-all', { userId, type });
  return new Promise((resolve) => {
    setTimeout(() => {
      mockData.messages = mockData.messages.map((msg) =>
        (!type || msg.type === type) && !msg.isRead ? { ...msg, isRead: true } : msg,
      )
      resolve()
    }, 500)
  })
}

export const getUserInfo = async (userId: string): Promise<UserInfo> => {
  // 真实 API 示例
  // const response = await api.get(`/users/${userId}`);
  // return response.data;
  return new Promise((resolve) => {
    setTimeout(() => resolve(mockData.userInfo), 500)
  })
}

// 新增：获取用户帖子
export const getUserPosts = async (userId: string): Promise<Post[]> => {
  // 真实 API 示例
  // const response = await api.get(`/users/${userId}/posts`);
  // return response.data;
  return new Promise((resolve) => {
    setTimeout(() => resolve(mockData.userPosts), 500)
  })
}

// 新增：获取贴吧详情
export const getForum = async (forumId: number): Promise<Forum> => {
  // 真实 API 示例
  // const response = await api.get(`/forums/${forumId}`);
  // return response.data;
  return new Promise((resolve, reject) => {
    setTimeout(() => {
      const forum = mockData.hotForums.find((f) => f.id === forumId)
      if (forum) {
        resolve(forum)
      } else {
        reject(new Error('贴吧不存在'))
      }
    }, 500)
  })
}

// 新增：获取贴吧帖子
export const getForumPosts = async (
  forumId: number,
  page: number,
  sort: 'newest' | 'reply' = 'reply',
): Promise<Post[]> => {
  // 真实 API 示例
  // const response = await api.get(`/forums/${forumId}/posts?page=${page}&limit=${limit}`);
  // return response.data;
  return new Promise((resolve) => {
    setTimeout(() => {
      const posts = [...mockData.hotPosts, ...mockData.followedPosts].filter(
        (p) => p.forumId === forumId,
      )
      if (sort === 'newest') {
        // 按发布时间倒序
        posts.sort((a, b) => b.createdAt - a.createdAt)
      } else if (sort === 'reply') {
        // 按最后回复时间倒序（如果没有则使用发布时间）
        posts.sort((a, b) => (b.lastRepliedAt || b.createdAt) - (a.lastRepliedAt || a.createdAt))
      }
      const start = (page - 1) * REPLY_CONSTANTS.POST_LIST_PAGE_SIZE
      const end = start + REPLY_CONSTANTS.POST_LIST_PAGE_SIZE
      resolve(posts.slice(start, end))
    }, 500)
  })
}

// 新增：关注贴吧
export const followForumApi = async (userId: string, forumId: number): Promise<void> => {
  // 真实 API 示例
  // await api.post(`/users/${userId}/follow-forum`, { forumId });
  return new Promise((resolve) => {
    setTimeout(() => {
      const forum = mockData.hotForums.find((f) => f.id === forumId)
      if (forum && !mockData.followedForums.some((f) => f.id === forumId)) {
        mockData.followedForums.push(forum)
      }
      resolve()
    }, 500)
  })
}

export const getPost = async (forumId: number, postId: number, userId?: string): Promise<Post> => {
  return new Promise((resolve) => {
    setTimeout(() => {
      const post = mockData.hotPosts.find((p) => p.id === postId && p.forumId === forumId)
      if (post) {
        resolve({
          ...post,
          authorId: 'user123',
          likeCount: 15,
          collectCount: 8,
          isLiked: userId ? Math.random() > 0.7 : false,
          isCollected: userId ? Math.random() > 0.8 : false,
        })
      } else {
        throw new Error('帖子不存在')
      }
    }, 500)
  })
}

// 新增：获取回复列表
export const getReplies = async (
  postId: number,
  page: number,
  limitNumber: number = REPLY_CONSTANTS.INITIAL_SUB_REPLY_COUNT,
  parentId?: number, // 若指定 parentId，则表示获取楼中楼详情
): Promise<{ replies: Reply[]; total: number }> => {
  return new Promise((resolve) => {
    setTimeout(() => {
      // 模拟所有回复
      const mockReplies: Reply[] = [
        {
          id: 1,
          postId,
          author: '用户A',
          authorId: 1,
          content: '楼主说得对',
          floor: 1,
          createdAt: Date.now() - 3600000,
          likeCount: 5,
          isLiked: false,
        },
        {
          id: 4,
          postId,
          author: '用户D',
          authorId: 4,
          content: '支持',
          floor: 2,
          createdAt: Date.now() - 7200000,
          likeCount: 1,
          isLiked: false,
        },
        // 子回复（parentId: 1）
        {
          id: 2,
          postId,
          author: '用户B',
          authorId: 2,
          content: '@用户A 确实',
          createdAt: Date.now() - 1800000,
          likeCount: 2,
          isLiked: false,
          parentId: 1,
        },
        {
          id: 3,
          postId,
          author: '用户C',
          authorId: 3,
          content: '@用户B 同意',
          createdAt: Date.now() - 900000,
          likeCount: 1,
          isLiked: false,
          parentId: 1,
        },
        {
          id: 5,
          postId,
          author: '用户A',
          authorId: 1,
          content: '@用户B 谢谢',
          createdAt: Date.now() - 600000,
          likeCount: 0,
          isLiked: false,
          parentId: 1,
        },
        {
          id: 6,
          postId,
          author: '用户E',
          authorId: 5,
          content: '再补充一点',
          createdAt: Date.now() - 300000,
          likeCount: 3,
          isLiked: false,
          parentId: 1,
        },
        // 另一个楼层（parentId: 4）
        {
          id: 7,
          postId,
          author: '用户F',
          authorId: 6,
          content: '我也觉得',
          createdAt: Date.now() - 500000,
          likeCount: 1,
          isLiked: false,
          parentId: 4,
        },
      ]

      let total = 0
      let resultReplies: Reply[] = []

      if (parentId !== undefined) {
        // ==================== 情况 1：加载指定 parentId 的子回复（支持分页）================
        // console.log('with parentId')
        const children = mockReplies
          .filter((r) => r.parentId === parentId)
          .sort((a, b) => a.createdAt - b.createdAt)

        total = children.length

        // 分页
        const start = (page - 1) * REPLY_CONSTANTS.SUB_REPLY_PAGE_SIZE
        const end = start + REPLY_CONSTANTS.SUB_REPLY_PAGE_SIZE
        const paginated = children.slice(start, end)

        resultReplies = paginated.map((r) => ({
          ...r,
          children: [], // 子回复不再嵌套（避免无限递归）
        }))
      } else {
        console.log('no parentId')
        const roots = mockReplies
          .filter((r) => !r.parentId)
          .sort((a, b) => a.createdAt - b.createdAt)

        total = roots.length

        // 分页一级回复
        const start = (page - 1) * REPLY_CONSTANTS.ROOT_REPLY_PAGE_SIZE
        const end = start + REPLY_CONSTANTS.ROOT_REPLY_PAGE_SIZE
        const paginatedRoots = roots.slice(start, end)

        resultReplies = paginatedRoots.map((root) => {
          const children = mockReplies
            .filter((r) => r.parentId === root.id)
            .sort((a, b) => a.createdAt - b.createdAt)

          const displayedChildren = children.slice(0, limitNumber).map((c) => ({
            ...c,
            children: [], // 二级以下不嵌套
          }))

          return {
            ...root,
            children: displayedChildren,
            replyCount: children.length, // 关键：用于“查看全部 X 条”
          }
        })
      }
      resolve({ replies: resultReplies, total })
    }, 300)
  })
}

// 新增：发表回复
export const createReply = async (
  userId: string,
  postId: number,
  content: string,
  parentId?: number,
): Promise<Reply> => {
  return new Promise((resolve) => {
    setTimeout(() => {
      const newReply: Reply = {
        id: Date.now() + Math.random(),
        postId,
        author: '当前用户',
        authorId: userId,
        content,
        //如果是楼中楼则不必设置楼层数
        floor: 1, // 实际应从后端获取
        createdAt: Date.now(),
        likeCount: 0,
        isLiked: false,
        parentId,
      }
      resolve(newReply)
    }, 500)
  })
}

export const createPostApi = async (forumId: number, post: { title: string; content: string }) => {
  // 模拟 API 调用
  // const response = await fetch(`/api/forums/${forumId}/posts`, {
  //   method: 'POST',
  //   headers: { 'Content-Type': 'application/json' },
  //   body: JSON.stringify(post),
  // })
  // if (!response.ok) throw new Error('创建帖子失败')
  // return response.json()

  // 模拟返回数据
  return new Promise<Post>((resolve) => {
    setTimeout(() => {
      resolve({
        id: Math.random(), // 模拟帖子 ID
        title: post.title,
        forumId: forumId,
        forumName:
        content: post.content,
        author: '当前用户', // 应从用户状态获取
        authorAvatar: '', // 应从用户状态获取
        replyCount: 0,
        likeCount: 0,
        isLiked: false,
        isFeatured: false,
        lastRepliedAt: Date.now(),
      })
    }, 500)
  })
}

// 点赞/取消点赞
export const likePost = async (userId: string, postId: number): Promise<void> => {
  return new Promise((resolve) => setTimeout(resolve, 300))
}
export const unlikePost = async (userId: string, postId: number): Promise<void> => {
  return new Promise((resolve) => setTimeout(resolve, 300))
}
export const collectPost = async (userId: string, postId: number): Promise<void> => {
  return new Promise((resolve) => setTimeout(resolve, 300))
}
export const uncollectPost = async (userId: string, postId: number): Promise<void> => {
  return new Promise((resolve) => setTimeout(resolve, 300))
}
export const likeReply = async (userId: string, replyId: number): Promise<void> => {
  return new Promise((resolve) => setTimeout(resolve, 300))
}
export const unlikeReply = async (userId: string, replyId: number): Promise<void> => {
  return new Promise((resolve) => setTimeout(resolve, 300))
}
