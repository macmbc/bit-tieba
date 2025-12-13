# 后端需求规格说明书


## 1. 引言
- **目标**：明确后端交付的功能、接口、数据与非功能性要求，支撑前端对接与验收。
- **参考**：RequirementsAnalysis.md、server/README.md、server/API_DIFF.md、server/BACKEND_INTEGRATION.md、架构图。

## 2. 系统概述
- **总体架构**：单一 HTTP 入口（GateServer）承载论坛接口与账号/验证码/登录，内部调用：
  - VarifyServer（Node.js gRPC，邮件验证码、Redis 缓存 `code_<email>`）
  - StatusServer（C++ gRPC，分配 ChatServer、写入 Redis `utoken_<uid>`、路由信息）
  - ChatServer/ChatServer2 集群（TCP 长连 + gRPC 互通，好友与文本聊天）
  - MySQL（业务数据）与 Redis（token/缓存/分布式锁）
- **业务流示意**：![核心业务流](pic/Gemini_Generated_Image_v0f943v0f943v0f9.png)
- **运行环境**：Linux/macOS，CMake + gRPC + Boost + MySQL Connector/C++，Redis，Node.js ≥16（VarifyServer）。
- **默认端口**：GateServer 8080；VarifyServer gRPC 50051；StatusServer 50052；ChatServer TCP 8090/8091、gRPC 50055/50056。
- **角色**：游客（未登录）、登录用户、贴吧吧主/管理员（拥有设精/置顶与删帖权限）、聊天在线用户。

## 3. 功能需求
### 3.1 账号与安全
- 发送邮件验证码：校验 email → 调 VarifyServer 生成 4 位验证码，写 Redis `code_<email>`，返回 error。
- 用户注册：校验 JSON、两次密码一致、验证码匹配；MySQL 事务生成 uid；返回 `{uid,user,email}`。
- 重置密码：校验验证码，校验用户名与邮箱匹配，更新密码。
- 用户登录：校验邮箱+密码；调用 StatusServer 获取可用聊天节点与 token，写 Redis `utoken_<uid>`；返回 `{uid,token,host,port}`。
- Token 校验：写操作与“我的”查询需 `uid + token`，GateServer 在 Redis 校验。

### 3.2 贴吧管理
- 获取贴吧列表：分页必填 `page/limit`；返回贴吧摘要（关注数、帖子数）。
- 获取贴吧详情：`forum_id` 查询，若带 `uid/token` 返回 `is_followed`。
- 关注/取消关注：需 `uid/token/forum_id`；校验 token，更新 follow 表与计数。
- 我的关注贴吧列表：`uid/token + page/limit` 返回关注的贴吧。

### 3.3 帖子管理
- 获取帖子列表：按 `forum_id` + `page/limit` + `sort(time|hot|reply)`，过滤软删；返回帖子摘要（预览、置顶/精华等）。
- 获取帖子详情：`post_id`，可带 `uid/token` 返回 `is_liked/is_collected`。
- 发帖：`uid/token/forum_id/title/content`，事务写 post 与 forum 计数，返回 `post_id`。
- 更新/删除帖子：需作者或吧主权限；删除为软删（status=0）；更新标题/内容。
- 点赞/取消点赞、收藏/取消收藏：需 token，更新计数与记录。
- 置顶/加精：吧主管理动作；需 `uid/token/post_id` 与布尔状态。

### 3.4 回复管理
- 获取回复列表：`post_id` 必填，支持 `page/limit/sort(time|hot)`；返回楼层及 `sub_replies`，过滤软删。
- 发表回复：`uid/token/post_id/content` 可指定 `parent_reply_id`（楼中楼）；更新计数。
- 删除回复：作者、帖主或吧主可删（软删）。
- 点赞/取消点赞回复：需 token，更新计数。

### 3.5 搜索
- 关键词搜索：`keyword` + `type=post|forum|user` + `page/limit`，按 LIKE 匹配（可加全文索引）；post 可选 `forum_id` 过滤。
- 结果字段：post（id、标题、作者、计数、置顶/精华、内容预览）、forum（id、名称、描述、关注/帖子数）、user（uid、name/nick/email/icon/sex）。

### 3.6 用户内容管理
- 我的帖子/回复/收藏：`uid/token + page/limit`；回复结果携带 `post_title`。
- 我的粉丝/关注：`uid/token + page/limit` 返回用户列表。

### 3.7 实时聊天（TCP）
- 登录流程：持有 GateServer 返回的 `{uid,token,host,port}`，建立 TCP，发送 `MSG_CHAT_LOGIN`；成功返回用户档案、申请列表、好友列表并写 Redis 路由。
- 好友功能：搜索用户（uid 或用户名）、发起申请、审批（同意/拒绝），实时推送通知。
- 文本聊天：发送/ACK/推送均携带 `text_array`；跨节点通过 gRPC 转发。
- 心跳与踢下线：客户端 10s 心跳（ID 1023），超时 20s 清理；异地登录推送 `ID_NOTIFY_OFF_LINE_REQ`。

## 4. 接口规格
### 4.1 HTTP 约定（GateServer）
- 协议：HTTP/1.1，`Content-Type: application/json`；响应头 `text/json`。
- 成功/失败：统一返回 JSON，`error`=0 代表成功，其他为错误码（见 §9）。
- 认证：写操作及“我的”查询需 `uid` 与 `token`；校验失败返回 `TokenInvalid=1010` 或 `UidInvalid=1011`。
- 软删：帖子、回复删除不物理删除，列表/详情默认过滤 `status=1`。

### 4.2 HTTP 接口清单
| 分组 | 方法&路径 | 认证 | 请求关键字段 | 成功响应要点 |
|---|---|---|---|---|
| 验证码/账号 | POST `/get_varifycode` | 否 | email | `{error,email}` |
|  | POST `/user_register` | 否 | email,user,passwd,confirm,icon,varifycode | `{error,uid,user,email}` |
|  | POST `/reset_pwd` | 否 | email,user,passwd,varifycode | `{error}` |
|  | POST `/user_login` | 否 | email,passwd | `{error,uid,token,host,port,email}` |
| 贴吧 | GET `/forums` | 否 | page,limit | `{error,forums:[{forum_id,name,avatar,description,follower_count,post_count}]}` |
|  | GET `/forum/detail` | 可选 | forum_id,(uid,token) | `{error,forum:{...,is_followed}}` |
|  | POST `/follow_forum` `/unfollow_forum` | 是 | uid,token,forum_id | `{error}` |
|  | GET `/followed_forums` | 是 | uid,token,page,limit | `{error,forums:[...]}` |
| 帖子 | GET `/posts` | 否 | forum_id,page,limit,sort | `{error,posts:[...content_preview...]}` |
|  | GET `/post/detail` | 可选 | post_id,(uid,token) | `{error,post:{...,is_liked,is_collected}}` |
|  | POST `/post/create` | 是 | uid,token,forum_id,title,content | `{error,post_id}` |
|  | POST `/post/update` | 是 | uid,token,post_id,title,content | `{error}` |
|  | POST `/post/delete` | 是 | uid,token,post_id | `{error}` |
|  | POST `/like_post` `/unlike_post` `/collect_post` `/uncollect_post` | 是 | uid,token,post_id | `{error}` |
|  | POST `/set_top_post` `/set_essence_post` | 是 | uid,token,post_id,is_top/is_essence | `{error}` |
| 回复 | GET `/post/replies` | 可选 | post_id,page,limit,sort,(uid,token) | `{error,replies:[{...,sub_replies:[...] }]}` |
|  | POST `/reply` | 是 | uid,token,post_id,content,parent_reply_id | `{error,reply_id}` |
|  | POST `/reply/delete` | 是 | uid,token,reply_id | `{error}` |
|  | POST `/like_reply` `/unlike_reply` | 是 | uid,token,reply_id | `{error}` |
| 搜索 | GET `/search` | 否 | keyword,type,page,limit,(forum_id) | `{error,results:[...]}` |
| 我的 | GET `/my/posts` `/my/replies` `/my/collections` | 是 | uid,token,page,limit | `{error,posts/replies:[...]}` |
|  | GET `/my/followers` `/my/following` | 是 | uid,token,page,limit | `{error,users:[...]}` |

### 4.3 内部接口与协议
- **VarifyServer gRPC**：`GetVarifyCode(email)` → 写 Redis 验证码并发邮件；错误码透传。
- **StatusServer gRPC**：`GetChatServer(uid,name)` → 选择 ChatServer，生成 token 写 Redis，返回 host/port/name。
- **ChatServer gRPC**：节点间转发好友通知、文本消息、踢人等；对客户端暴露 TCP 协议。
- **TCP 消息格式**：2 字节消息 ID + 2 字节正文长度（大端）+ UTF-8 JSON。常用 ID：登录 1005/1006，搜索用户 1007/1008，申请好友 1009/1010，审批 1013/1014，文本消息 1017/1018/1019，心跳 1023/1024，踢下线 1021（详见 BACKEND_INTEGRATION.md）。

## 5. 数据与存储需求
### 5.1 MySQL 表
- **BBS/论坛表**（字段以 `databaseD/*_20251213.txt` 为准，示例数据已导出方便填充）：
  - `user`：`id` 自增 PK，`uid` 唯一，`name` 索引，`email` 唯一，`pwd`、`nick`、`desc`、`sex`、`icon`。
  - `forum`：`forum_id` PK，`name`、`avatar`、`description`、`owner_uid`、`follower_cnt`、`post_cnt`、`created_at`、`updated_at`。
  - `forum_admin`：`id` PK，`forum_id`，`uid`，`role`（1=owner/吧主）。
  - `forum_follow`：`id` PK，`forum_id`，`uid`，`created_at`。
  - `post`：`post_id` PK，`forum_id`，`uid`，`title`，`content`，`is_top`，`is_essence`，`like_cnt`，`collect_cnt`，`reply_cnt`，`status`（软删），`created_at`，`updated_at`，`last_reply_at`。
  - `post_like`、`post_collect`：`id` PK，`post_id`，`uid`，`created_at`。
  - `reply`：`reply_id` PK，`post_id`，`uid`，`content`，`parent_reply_id`，`root_reply_id`，`floor`，`like_cnt`，`status`，`created_at`。
  - `reply_like`：`id` PK，`reply_id`，`uid`，`created_at`。
  - `user_follow`：`id` PK，`uid`，`target_uid`，`created_at`（粉丝/关注关系，用于“我的粉丝/关注”）。
- **聊天/账户表（来源 `databaseD/llfc.sql`）**：
  - `user`：`id` PK，自增；`uid` 唯一；`email` 唯一；`name` 索引；字段含 `pwd/nick/desc/sex/icon`，字符集 utf8mb4。
  - `user_id`：单行序列表，`id` PK；`reg_user` 存储过程通过自增该表生成新的 `uid`。
  - `friend`：`id` PK，自增；`self_id`+`friend_id` 唯一索引；字段 `back` 记录好友备注。
  - `friend_apply`：`id` PK，自增；`from_uid`+`to_uid` 唯一；`status` tinyint（0 待处理，1 同意）。
  - `reg_user` 存储过程：检查用户名/邮箱唯一 → 更新 `user_id` → 插入 `user`，出错回滚并返回 `result=-1`，已存在返回 `0`，成功返回新 `uid`。
  - 默认字符集/排序：utf8mb4/utf8mb4_unicode_ci。
### 5.2 Redis 键前缀
- `code_<email>`（验证码，600s）
- `utoken_<uid>`（登录 token）
- `uip_<uid>`（聊天路由 ChatServer 名）
- `usession_<uid>`（会话 ID，踢人用）
- `ubaseinfo_<uid>`、`nameinfo_<name>`（聊天用户缓存）
- `logincount`（Hash，在线人数统计，可选）
- `lock_<uid>`、`lockcount`（分布式锁，互斥登录/清理）
### 5.3 数据一致性
- 发帖/删帖等涉及计数使用事务；软删通过 `status` 字段过滤。

## 6. 安全与权限
- 认证：Token 必须与 uid 一致，Redis 校验；token 由 StatusServer 颁发，前端需在失效时重新登录。
- 访问控制：
  - 发帖/回帖/点赞/收藏/关注/我的*：需登录。
  - 删帖：作者或对应贴吧吧主；删回复：作者、帖主或吧主。
  - 置顶/加精：贴吧 owner 权限。
- 通信安全：默认 HTTP/TCP；如需公网部署，建议通过反向代理启用 TLS。
- 输入校验：JSON 解析失败返回 `Error_Json`；SQL 访问均使用预处理语句防注入。

## 7. 非功能性要求
- **性能**：列表接口分页必填；排序支持 time/hot/reply；Redis 缓存 token/验证码/用户资料以降低数据库压力。
- **可用性**：依赖 MySQL、Redis、VarifyServer、StatusServer、ChatServer 正常；失败时返回错误码，前端需有重试/提示。
- **可靠性**：关键写操作事务或显式回滚（发帖、删帖、计数更新）；聊天心跳超时自动清理。
- **可维护性**：配置集中于各服务 `config.ini/json`；日志输出标准输出，便于收集。
- **兼容性/扩展**：支持多 ChatServer 节点，StatusServer 负责分配；BBS 接口偏向 POST + query 风格（无路径参数）。

## 8. 配置与部署要求
- 配置文件：`GateServer/config.ini`、`StatusServer/config.ini`、`ChatServer/config.ini`、`ChatServer2/config.ini`、`VarifyServer/config.json`。
- 需按环境修改：MySQL/Redis host、端口、账号、密码；ChatServer 名称/端口；SMTP 设置（VarifyServer）。
- 构建：CMake ≥3.18，gRPC/protobuf，MySQL Connector/C++，Boost；VarifyServer 需 `npm install`。
- 运行顺序：MySQL/Redis → VarifyServer → StatusServer → ChatServer* → GateServer。

## 9. 错误码（GateServer/公共）
| 码值 | 含义 |
|---|---|
| 0 | Success |
| 1001 | Error_Json（JSON 解析失败） |
| 1002 | RPCFailed（内部 gRPC 或服务异常） |
| 1003 | VarifyExpired（验证码过期/不存在） |
| 1004 | VarifyCodeErr（验证码错误） |
| 1005 | UserExist（用户名或邮箱已存在） |
| 1006 | PasswdErr（密码不一致） |
| 1007 | EmailNotMatch（用户名与邮箱不匹配） |
| 1008 | PasswdUpFailed（重置密码失败） |
| 1009 | PasswdInvalid（密码错误） |
| 1010 | TokenInvalid（token 失效） |
| 1011 | UidInvalid（uid 无效） |
| 1012 | ForumNotFound |
| 1013 | AlreadyFollowed |
| 1014 | NotFollowed |
| 1015 | DbError |
| 1016 | PostNotFound |
| 1017 | NoPermission |
| 1018 | AlreadyLiked |
| 1019 | NotLiked |
| 1020 | AlreadyCollected |
| 1021 | NotCollected |
| 1022 | ReplyNotFound |
| 1023 | AlreadyLikedReply |
| 1024 | NotLikedReply |

## 10. 验收要点
- 所有接口按表格参数校验并返回统一 `error` 字段。
- 写操作未携带或校验失败的 token 必须拒绝。
- 列表接口均支持分页且过滤软删数据。
- 聊天登录后收到好友列表/申请列表，心跳超时会被服务端清理，异地登录会推送下线。
- 配置可通过修改 `config.ini/json` 适配不同环境，默认端口/依赖可启动。
