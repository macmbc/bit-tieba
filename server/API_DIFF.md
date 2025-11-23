# 后端接口与《RequirementsAnalysis.md》差异说明（2.2~2.6）

便于前端对接，以下列出当前 GateServer 已实现接口与需求文档中定义的差异点。未列出的字段/行为默认与需求一致。

## 2.2 贴吧管理
| 接口 | 需求文档 | 实际实现 | 备注 |
| --- | --- | --- | --- |
| GET `/forums` | 返回分页列表 | **同需求** | 分页参数 `page/limit`，必填 `forum_id` 无 |
| GET `/forum/:forum_id` | 路径参数 | 实际为 `GET /forum/detail?forum_id=...` | 动态路径未实现，用 query 参数代替；可带 `uid/token` 返回 `is_followed` |
| POST `/follow_forum` | 请求体 `{uid, forum_id}` | **增加** `token` | 需校验 Redis token |
| POST `/unfollow_forum` | `{uid, forum_id}` | **增加** `token` | 需校验 Redis token |
| GET `/followed_forums` | `?uid=&page=&limit=` | **增加** `token` | 需校验 Redis token |

## 2.3 帖子管理
| 接口 | 需求文档 | 实际实现 | 备注 |
| --- | --- | --- | --- |
| GET `/posts` | `?forum_id=&page=&limit=&sort=` | **同需求** | `sort` 支持 `time`/`hot`/`reply`（默认 time） |
| GET `/post/:post_id` | 路径参数 | 实际为 `GET /post/detail?post_id=...` | 动态路径未实现；可带 `uid/token` 返回 `is_liked/is_collected` |
| POST `/post` | `{uid, forum_id, title, content}` | 实际为 `POST /post/create`，需 `{uid, token, forum_id, title, content}` | 增加 token 校验；路径不同 |
| PUT `/post/:post_id` | `{uid, title, content}` | 实际为 `POST /post/update`，需 `{uid, token, post_id, title, content}` | 使用 POST，增加 token |
| DELETE `/post/:post_id` | `{uid}` | 实际为 `POST /post/delete`，需 `{uid, token, post_id}` | 使用 POST，增加 token；作者或吧主可删（软删除，status=0） |
| POST `/like_post` / `/unlike_post` | `{uid, post_id}` | **增加** `token` | 需 token |
| POST `/collect_post` / `/uncollect_post` | `{uid, post_id}` | **增加** `token` | 需 token |
| POST `/set_top_post` / `/set_essence_post` | `{uid, post_id, is_top/is_essence}` | **增加** `token`，路径使用 POST | 需 token；权限为贴吧 owner（需求描述“吧主”一致） |
| 响应 | 未指定 | 统一 `{ "error": code, ... }` | 成功 `error=0`，错误码详见 `const.h` |

## 2.4 回复管理
| 接口 | 需求文档 | 实际实现 | 备注 |
| --- | --- | --- | --- |
| GET `/post/:post_id/replies` | 路径参数，支持楼中楼 | 实际为 `GET /post/replies?post_id=...`，可选 `page/limit/sort` | `sort` 支持 `time`/`hot`（默认 time）；返回结构含 `replies`，每个含 `sub_replies` |
| POST `/reply` | `{uid, post_id, content, parent_reply_id, at_uids}` | 实际为 `{uid, token, post_id, content, parent_reply_id}` | 需 token；未实现 `at_uids` |
| DELETE `/reply/:reply_id` | `{uid}` | 实际为 `POST /reply/delete`，需 `{uid, token, reply_id}` | 使用 POST；删除权限：作者/帖子作者/贴吧 owner；软删（status=0） |
| POST `/like_reply` / `/unlike_reply` | `{uid, reply_id}` | **增加** `token` | 需 token |
| 响应 | 未指定 | 统一 `{ "error": code, ... }` | 成功 `error=0`；`ReplyNotFound=1022`、`AlreadyLikedReply=1023`、`NotLikedReply=1024` |

## 2.5 搜索功能
| 接口 | 需求文档 | 实际实现 | 备注 |
| --- | --- | --- | --- |
| GET `/search` | `?keyword=xxx&type=post|forum|user&forum_id=&page=&limit=` | **同需求** | `type` 默认 `post`；使用 `LIKE` 模糊匹配（可选加全文索引） |
| 响应 | `{"results":[...]}` | **同需求字段子集** | - post: `post_id, forum_id, uid, author, title, created_at, reply_count, like_count, is_top, is_essence, content_preview`<br>- forum: `forum_id, name, avatar, description, follower_count, post_count`<br>- user: `uid, name, nick, email, icon, sex` |

## 2.6 用户内容管理
| 接口 | 需求文档 | 实际实现 | 备注 |
| --- | --- | --- | --- |
| GET `/my/posts` | `?uid=&page=&limit=` | **增加** `token` | 需 token；返回 `posts` 列表 |
| GET `/my/replies` | `?uid=&page=&limit=` | **增加** `token` | 返回 `replies`，每条带 `post_title` |
| GET `/my/collections` | `?uid=&page=&limit=` | **增加** `token` | 返回收藏的帖子列表 |
| GET `/my/followers` | `?uid=&page=&limit=` | **增加** `token` | 返回粉丝用户列表 |
| GET `/my/following` | `?uid=&page=&limit=` | **增加** `token` | 返回关注用户列表 |
| 响应 | 未指定 | 统一 `{ "error": code, ... }` | 成功 `error=0`，错误码与其他接口一致 |

## 通用差异与约定
- **鉴权**：所有写操作以及“我的”类查询均需 `uid + token`，token 取自 `/user_login`，Redis 校验键名 `utoken_<uid>`。
- **路径风格**：未实现路径参数的接口改用 query（如 `/forum/detail`、`/post/detail`、`/post/replies`）；更新/删除多用 POST 而非 PUT/DELETE。
- **错误码**：统一使用 `error` 字段；常见值见 `server/GateServer/const.h`（0 成功，1001 JSON 解析失败，1010 Token 失效，1012 贴吧不存在，1016 帖子不存在，1017 无权限等）。
- **软删除**：帖子、回复删除为软删（status=0），列表/详情查询带 `status=1` 过滤。

## 当前请求体 / 响应体结构（实际实现）

### 2.2 贴吧管理
- `GET /forums?{page,limit}`  
  - 响应：`{"error":0,"forums":[{"forum_id", "name", "avatar", "description", "follower_count", "post_count"}]}`
- `GET /forum/detail?forum_id=&uid=&token=`  
  - 响应：`{"error":0,"forum":{"forum_id","name","avatar","description","follower_count","post_count","is_followed":bool}}`
- `POST /follow_forum` / `POST /unfollow_forum`  
  - 请求：`{"uid":int,"token":string,"forum_id":int}`  
  - 响应：`{"error":0}`
- `GET /followed_forums?uid=&token=&page=&limit=`  
  - 响应：`{"error":0,"forums":[{同上}]}`

### 2.3 帖子管理
- `GET /posts?forum_id=&page=&limit=&sort=`  
  - 响应：`{"error":0,"posts":[{"post_id","forum_id","uid","author","title","created_at","reply_count","like_count","is_top", "is_essence","content_preview"}]}`
- `GET /post/detail?post_id=&uid=&token=`  
  - 响应：`{"error":0,"post":{"post_id","forum_id","uid","author","title","content","created_at","reply_count","like_count","collect_count","is_top","is_essence","is_liked":bool,"is_collected":bool}}`
- `POST /post/create`  
  - 请求：`{"uid":int,"token":string,"forum_id":int,"title":string,"content":string}`  
  - 响应：`{"error":0,"post_id":int}`
- `POST /post/update`  
  - 请求：`{"uid":int,"token":string,"post_id":int,"title":string,"content":string}`  
  - 响应：`{"error":0}`
- `POST /post/delete`  
  - 请求：`{"uid":int,"token":string,"post_id":int}`  
  - 响应：`{"error":0}` （软删）
- `POST /like_post` / `POST /unlike_post` / `POST /collect_post` / `POST /uncollect_post`  
  - 请求：`{"uid":int,"token":string,"post_id":int}`  
  - 响应：`{"error":0}`
- `POST /set_top_post` / `POST /set_essence_post`  
  - 请求：`{"uid":int,"token":string,"post_id":int,"is_top":bool}` / `{"uid":int,"token":string,"post_id":int,"is_essence":bool}`  
  - 响应：`{"error":0}`

### 2.4 回复管理
- `GET /post/replies?post_id=&page=&limit=&sort=&uid=&token=`  
  - 响应示例：  
    ```json
    {
      "error": 0,
      "replies": [
        {
          "reply_id": 1, "post_id": 4, "uid": 1054, "content": "...",
          "parent_reply_id": 0, "floor": 1, "like_count": 0, "created_at": "...",
          "is_liked": false,
          "sub_replies": [
            {
              "reply_id": 2, "post_id": 4, "uid": 1054, "content": "...",
              "parent_reply_id": 1, "root_reply_id": 1, "floor": 0,
              "like_count": 0, "created_at": "...", "is_liked": false
            }
          ]
        }
      ]
    }
    ```
- `POST /reply`  
  - 请求：`{"uid":int,"token":string,"post_id":int,"content":string,"parent_reply_id":long}` (parent 可为 0)  
  - 响应：`{"error":0,"reply_id":long}`
- `POST /reply/delete`  
  - 请求：`{"uid":int,"token":string,"reply_id":long}`  
  - 响应：`{"error":0}` （软删）
- `POST /like_reply` / `POST /unlike_reply`  
  - 请求：`{"uid":int,"token":string,"reply_id":long}`  
  - 响应：`{"error":0}`

### 2.5 搜索功能
- `GET /search?keyword=&type=post|forum|user&forum_id=&page=&limit=`  
  - 响应：`{"error":0,"results":[ ... ]}`  
    - type=post: `{"post_id","forum_id","uid","author","title","created_at","reply_count","like_count","is_top","is_essence","content_preview"}`  
    - type=forum: `{"forum_id","name","avatar","description","follower_count","post_count"}`  
    - type=user: `{"uid","name","nick","email","icon","sex"}`

### 2.6 用户内容管理
- `GET /my/posts?uid=&token=&page=&limit=`  
  - 响应：`{"error":0,"posts":[{"post_id","forum_id","uid","author","title","created_at","reply_count","like_count","is_top","is_essence","content_preview"}]}`
- `GET /my/replies?uid=&token=&page=&limit=`  
  - 响应：`{"error":0,"replies":[{"reply_id","post_id","uid","content","parent_reply_id","root_reply_id","floor","like_count","created_at","post_title"}]}`
- `GET /my/collections?uid=&token=&page=&limit=`  
  - 响应：`{"error":0,"posts":[{同上帖子列表字段}]}`
- `GET /my/followers?uid=&token=&page=&limit=` / `GET /my/following?uid=&token=&page=&limit=`  
  - 响应：`{"error":0,"users":[{"uid","name","nick","email","icon","sex"}]}`
