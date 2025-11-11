// src/api/authApi.ts
import axios from 'axios'

// 后端 GateServer 地址
const BASE_URL = '/api' 

const authApi = axios.create({
  baseURL: BASE_URL,
  headers: {
    'Content-Type': 'application/json',
  },
  timeout: 10000,
})

// 响应拦截器 - 统一处理错误
authApi.interceptors.response.use(
  (response) => response.data,
  (error) => {
    console.error('API请求失败:', error)
    return Promise.reject(error)
  },
)

// ================== 类型定义 ==================

export interface ApiResponse<T = unknown> {
  error: number
  message?: string // 可选的提示信息
  data?: T // 具体数据内容
  [key: string]: unknown // 允许扩展字段
}

export interface GetVerifyCodeRequest {
  email: string
}

export interface GetVerifyCodeResponse extends ApiResponse {
  email: string
}

export interface RegisterRequest {
  email: string
  user: string
  passwd: string
  confirm: string
  icon?: string
  varifycode: string
}

export interface RegisterResponse extends ApiResponse {
  uid: number
  user: string
  email: string
}

export interface ResetPasswordRequest {
  email: string
  user: string
  passwd: string
  varifycode: string
}

export interface ResetPasswordResponse extends ApiResponse {
  email: string
  user: string
  passwd: string
}

export interface LoginRequest {
  email: string
  passwd: string
}

export interface LoginResponse extends ApiResponse {
  email: string
  uid: number
  token: string
  host: string
  port: string
}

// ================== 错误码常量 ==================

export const ERROR_CODES = {
  SUCCESS: 0,
  ERROR_JSON: 1001,
  RPC_FAILED: 1002,
  VARIFY_EXPIRED: 1003,
  VARIFY_CODE_ERR: 1004,
  USER_EXIST: 1005,
  PASSWD_ERR: 1006,
  EMAIL_NOT_MATCH: 1007,
  PASSWD_UP_FAILED: 1008,
  PASSWD_INVALID: 1009,
  TOKEN_INVALID: 1010,
  UID_INVALID: 1011,
} as const

export const ERROR_MESSAGES: Record<number, string> = {
  [ERROR_CODES.SUCCESS]: '操作成功',
  [ERROR_CODES.ERROR_JSON]: '请求格式有误',
  [ERROR_CODES.RPC_FAILED]: '服务暂不可用，请稍后重试',
  [ERROR_CODES.VARIFY_EXPIRED]: '验证码已过期，请重新获取',
  [ERROR_CODES.VARIFY_CODE_ERR]: '验证码错误',
  [ERROR_CODES.USER_EXIST]: '用户名或邮箱已存在',
  [ERROR_CODES.PASSWD_ERR]: '两次密码不一致',
  [ERROR_CODES.EMAIL_NOT_MATCH]: '用户名与邮箱不匹配',
  [ERROR_CODES.PASSWD_UP_FAILED]: '密码更新失败',
  [ERROR_CODES.PASSWD_INVALID]: '邮箱或密码错误',
  [ERROR_CODES.TOKEN_INVALID]: 'Token无效',
  [ERROR_CODES.UID_INVALID]: '用户ID无效',
}

// ================== API 接口 ==================

/**
 * 获取邮箱验证码
 */
export const getVerifyCode = async (
  data: GetVerifyCodeRequest,
): Promise<GetVerifyCodeResponse> => {
  return authApi.post('/get_varifycode', data)
}

/**
 * 用户注册
 */
export const userRegister = async (data: RegisterRequest): Promise<RegisterResponse> => {
  return authApi.post('/user_register', data)
}

/**
 * 重置密码
 */
export const resetPassword = async (data: ResetPasswordRequest): Promise<ResetPasswordResponse> => {
  return authApi.post('/reset_pwd', data)
}

/**
 * 用户登录
 */
export const userLogin = async (data: LoginRequest): Promise<LoginResponse> => {
  return authApi.post('/user_login', data)
}

/**
 * 获取错误信息
 */
export const getErrorMessage = (errorCode: number): string => {
  return ERROR_MESSAGES[errorCode] || `未知错误（错误码 ${errorCode}）`
}

export default authApi
