// src/constants/forum.ts

/**
 * 论坛相关分页常量
 * 用途：控制「首次显示多少条子回复」和「加载更多时每页多少条」
 */
export const REPLY_CONSTANTS = {
  // 首次渲染时，每条一级回复默认显示的子回复数量
  INITIAL_SUB_REPLY_COUNT: 2,

  // 加载更多子回复时，每页请求的数量
  SUB_REPLY_PAGE_SIZE: 5,

  // 一级回复每页数量（列表页）
  ROOT_REPLY_PAGE_SIZE: 10,

  // 帖子列表每页数量（贴吧主页）
  POST_LIST_PAGE_SIZE: 20,
} as const
