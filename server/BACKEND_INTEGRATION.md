# 后端集成与接口规范

本文面向需要复用本项目后端（`server` 目录）并与其它前端（如 `bit-tieba`）联动的开发者，覆盖服务架构、运行环境、数据库/缓存依赖、对外 HTTP 接口、主服务（Gate/Status/ChatServer）通信流程以及消息格式等内容。

---

## 1. 服务架构总览

```
┌───────────┐        ┌────────────────┐        ┌───────────────────────┐
│ bit-tieba │ <HTTP> │   GateServer   │ <gRPC> │   VarifyServer(Node)  │
└───────────┘        │   (8080)       │        │   (50051)             │
                      │   登录/注册    │        │   邮件验证码服务      │
                      │   密码重置     │        └───────────────────────┘
                      │   聊天入口     │
                      │                │ <gRPC> ┌───────────────────────┐
                      │                └──────► │   StatusServer        │
                      │                         │   (50052)             │
                      │                         │   聊天节点分配/Token  │
                      │                         └──────────────┬────────┘
                      │                                        gRPC
                      │                         ┌──────────────▼────────┐
                      │                         │   ChatServer 集群      │
                      └────────────────────────►│   TCP: 8090/8091 ...   │
                                                │   gRPC: 50055/50056... │
                                                │   实时聊天&好友业务     │
                                                └────────────────────────┘
```

- **GateServer（C++/Boost.Beast）**：对外唯一 HTTP 入口，负责注册、验证码、登录、密码重置。登录阶段会请求 StatusServer 分配聊天节点并回传临时 token。
- **VarifyServer（Node.js + gRPC）**：生成并发送 4 位邮件验证码，结果缓存于 Redis。
- **StatusServer（C++/gRPC）**：维护聊天节点列表，为用户分配 ChatServer 并生成登录 token，Token 存放于 Redis。
- **ChatServer（C++/TCP + gRPC）**：长连接聊天服务，处理登录、好友搜索/申请、消息发送、心跳等业务；节点间通过 gRPC 相互转发消息与踢人指令。
- **公共依赖**：
  - MySQL 8.0（默认 `127.0.0.1:3308`，数据库 `llfc`，脚本见 `sql备份/llfc.sql`）。
  - Redis（默认 `127.0.0.1:6380`，需设置密码 `123456`）。

---

## 2. 运行环境与配置

### 2.1 文件结构

```
server/
├─ GateServer/        # HTTP 网关
├─ StatusServer/      # 聊天节点分配
├─ ChatServer/        # 聊天主节点模板（chatserver1/2 等）
└─ VarifyServer/      # Node.js 验证码服务
```

### 2.2 关键配置文件

- `GateServer/config.ini`
- `StatusServer/config.ini`
- `ChatServer/config.ini`（每个实例需调整 `[SelfServer]` 和 `[PeerServer]`）
- `VarifyServer/config.json`

所有配置均以明文方式记录 MySQL、Redis、服务端口等，可按部署环境调整。

### 2.3 数据库初始化

```sql
mysql -h127.0.0.1 -P3308 -uroot -p123456. < sql备份/llfc.sql
```

主要表：
- `user`：用户基本资料
- `friend`：好友关系
- `friend_apply`：好友申请
- `user_id`：自增 uid 序列

### 2.4 Redis 键命名

| 键前缀                     | 说明                         | 典型写入者         |
|---------------------------|------------------------------|--------------------|
| `code_<email>`           | 邮件验证码（600s 过期）      | VarifyServer       |
| `utoken_<uid>`           | 登录 Token                    | StatusServer       |
| `uip_<uid>`              | ChatServer 实例名（路由）     | ChatServer 登录流程|
| `usession_<uid>`         | 会话 ID（踢人/清理）          | ChatServer         |
| `ubaseinfo_<uid>`        | 用户档案缓存（JSON）          | ChatServer         |
| `nameinfo_<name>`        | 用户名索引缓存（JSON）        | ChatServer         |
| `logincount`（Hash）      | 节点在线人数统计（可选）      | ChatServer         |
| `lock_<uid>` / `lockcount`| 分布式锁（互斥登录/清理）    | ChatServer         |

---

## 3. GateServer HTTP 接口

**请求头**：`Content-Type: application/json`  
**响应格式**：JSON（服务端响应头 `text/json`），所有接口均在 `error` 字段返回状态码。

| 接口 | 方法 | 请求体 | 响应体 | 说明 |
|------|------|--------|--------|------|
| `/get_varifycode` | POST | `{"email":"user@example.com"}` | `{"error":0,"email":"user@example.com"}` | 创建 4 位验证码，写入 Redis `code_<email>`，并调用 VarifyServer 发送邮件 |
| `/user_register` | POST | `{"email":"","user":"","passwd":"","confirm":"","icon":"","varifycode":""}` | `{"error":0,"uid":1059,"user":"...","email":"..."}` | 校验验证码、用户名/邮箱唯一性，写 MySQL，并返回新 uid |
| `/reset_pwd` | POST | `{"email":"","user":"","passwd":"","varifycode":""}` | `{"error":0,"email":"","user":"","passwd":""}` | 校验验证码并验证邮箱/用户名匹配，更新密码 |
| `/user_login` | POST | `{"email":"","passwd":""}` | `{"error":0,"email":"","uid":1054,"token":"...","host":"127.0.0.1","port":"8090"}` | 校验密码→查询 StatusServer，返回 TCP 连接目标与 token |

常见 `error` 码：
```
0    Success
1001 Error_Json          1002 RPCFailed
1003 VarifyExpired       1004 VarifyCodeErr
1005 UserExist           1006 PasswdErr
1007 EmailNotMatch       1008 PasswdUpFailed
1009 PasswdInvalid       1010 TokenInvalid
1011 UidInvalid
```

### 3.1 后端处理流程与回调指引

下表将 GateServer 中每个接口的后端处理函数、关键步骤以及常见错误码整理出来，便于前端实现对应的回调策略。对应实现位于 `server/GateServer/LogicSystem.cpp`。

#### `/get_varifycode`
- **后端处理函数**：`LogicSystem::RegPost("/get_varifycode", handler)`
- **处理流程**：
  1. 解析请求体，提取 `email`。
  2. 调用 `VerifyGrpcClient::GetVarifyCode`，由 VarifyServer 生成验证码并写入 Redis `code_<email>`，同时发送邮件。
  3. 将 gRPC 返回的错误码透传给前端。
- **错误对照**：

| 错误码 | 触发条件（后端） | 后端动作 | 前端建议 |
|--------|------------------|----------|----------|
| 0 | 验证码生成成功 | 返回 email 与 error=0 | 开始倒计时、禁用获取按钮、提示“验证码已发送” |
| 1001 | JSON 解析失败 | 直接返回错误 JSON | 检查请求参数编码，提示“请求格式有误” |
| 1002 | gRPC/邮件服务异常 | `VerifyGrpcClient` 捕获异常后返回 `RPCFailed` | 提示“验证码服务异常”，允许用户稍后重试 |
| 1003 | Redis 未命中（验证码过期） | VarifyServer 返回 `VarifyExpired` | 引导重新获取验证码 |
| 1004 | 验证码不一致 | VarifyServer 返回 `VarifyCodeErr` | 提醒重新输入验证码 |

**前端回调模板（示例 TypeScript）**：
```ts
async function requestVerifyCode(email: string) {
  const resp = await gateApi.post('/get_varifycode', { email });
  switch (resp.error) {
    case 0:
      onVerifyCodeSent(email);
      break;
    case 1001:
      showToast('请求格式有误，请检查邮箱输入');
      break;
    case 1002:
      showToast('验证码服务异常，请稍后重试');
      break;
    case 1003:
      showToast('验证码已过期，请重新获取');
      break;
    case 1004:
      showToast('验证码错误，请重新输入');
      break;
    default:
      showToast(`获取验证码失败（错误码 ${resp.error}）`);
  }
}
```

#### `/user_register`
- **后端处理函数**：`LogicSystem::RegPost("/user_register", handler)`
- **处理流程**：
  1. 校验 `passwd == confirm`。
  2. 从 Redis 获取 `code_<email>`，验证验证码。
  3. 调用 `MysqlMgr::RegUser` 执行事务：校验用户名/邮箱唯一 → 生成新 uid → 插入数据。
  4. 成功返回 `uid` 及注册信息。
- **错误对照**：

| 错误码 | 触发条件 | 后端动作 | 前端建议 |
|--------|----------|----------|----------|
| 0 | 注册成功 | 返回新 uid、用户名、邮箱等 | 自动跳转登录页或直接复用登录流程 |
| 1001 | JSON 解析失败 | 返回错误 | 检查表单序列化 |
| 1006 | 两次密码不一致 | 终止流程，返回 `PasswdErr` | 高亮两个密码输入框并提示 |
| 1003 | 验证码过期 | 返回 `VarifyExpired` | 引导重新获取验证码 |
| 1004 | 验证码错误 | 返回 `VarifyCodeErr` | 提醒用户核对验证码 |
| 1005 | 用户名或邮箱已存在 | 事务回滚，返回 `UserExist` | 在表单上提示冲突字段 |
| -1 | MySQL 异常（例如连接断开） | 捕获后记录日志 | 统一提示“服务器异常，请稍后再试” |

#### `/reset_pwd`
- **后端处理函数**：`LogicSystem::RegPost("/reset_pwd", handler)`
- **处理流程**：
  1. 验证码校验同注册逻辑。
  2. `MysqlMgr::CheckEmail` 校验用户名与邮箱匹配。
  3. `MysqlMgr::UpdatePwd` 更新密码。
- **错误对照**：

| 错误码 | 触发条件 | 前端建议 |
|--------|----------|----------|
| 0 | 更新成功 | 提示“密码已重置”，跳转登录页 |
| 1001 | JSON 解析失败 | 检查请求体 |
| 1003 | 验证码过期 | 引导重新获取验证码 |
| 1004 | 验证码错误 | 提示重新输入 |
| 1007 | 用户名与邮箱不匹配 | 提示用户核对账号信息 |
| 1008 | 更新失败（数据库异常） | 提供重试按钮或反馈入口 |

#### `/user_login`
- **后端处理函数**：`LogicSystem::RegPost("/user_login", handler)`
- **处理流程**：
  1. `MysqlMgr::CheckPwd` 验证账号密码，并加载用户信息。
  2. 调用 `StatusGrpcClient::GetChatServer`，分配聊天节点并生成 token（写入 Redis `utoken_<uid>`）。
  3. 返回 `uid`、`token`、`host`、`port`。
- **错误对照**：

| 错误码 | 触发条件 | 后端动作 | 前端建议 |
|--------|----------|----------|----------|
| 0 | 登录成功 | 返回聊天节点信息 | 缓存 `{uid, token, host, port}` 并建立 TCP 连接 |
| 1001 | JSON 解析失败 | 返回错误 | 提示输入格式异常 |
| 1009 | 密码不匹配 | 返回 `PasswdInvalid` | 提示“邮箱或密码错误”，支持错误次数限制 |
| 1002 | StatusServer 不可用 | 返回 `RPCFailed` | 提示“服务暂不可用”，提供重试/反馈 |

**前端回调模板**：
```ts
async function login(payload: { email: string; passwd: string }) {
  const resp = await gateApi.post('/user_login', payload);
  if (resp.error === 0) {
    cacheSession(resp); // 记录 uid/token/host/port
    await connectTcp(resp);
    return;
  }
  const hint = {
    1009: '邮箱或密码错误',
    1002: '聊天服务暂不可用，请稍后重试',
    1001: '请求格式有误',
  }[resp.error] ?? `登录失败（错误码 ${resp.error}）`;
  showToast(hint);
}
```

---

## 4. 聊天主服务（ChatServer）通信规范

### 4.1 登录与会话流程

1. 前端调用 `/user_login` 获取 `uid`、`token`、`host`、`port`。
2. 建立到 `host:port` 的 TCP 长连接，遵循自定义帧格式（见 4.2）。
3. 首次发送 `MSG_CHAT_LOGIN`（ID: 1005），正文携带 `uid`、`token`。
4. 若 `MSG_CHAT_LOGIN_RSP`（ID: 1006）返回 `error:0`，则会附带：
   - 用户基础资料（name/nick/icon/desc/sex 等）
   - `apply_list`：当前待处理好友申请
   - `friend_list`：好友列表（含备注 `back`）
5. 进入业务阶段：心跳、好友管理、聊天等。
6. 若其他终端登录同一账号，本连接会收到 `ID_NOTIFY_OFF_LINE_REQ` 并被服务器关闭；前端需提示用户重新登录。

### 4.2 TCP 帧封包

每条消息通过固定的 4 字节头部 + JSON 正文传输：

```
0-1 字节：消息 ID (short, big-endian)
2-3 字节：正文长度 (short, big-endian)
4-... 字节：UTF-8 JSON 正文
```

建议在客户端封装 `encodeFrame(msgId, json)` / `decodeFrame(buffer)`，并建立消息处理器映射。

### 4.3 消息 ID 对照

| ID | 宏名 | 描述 | 方向 |
|----|------|------|------|
| 1005 | `MSG_CHAT_LOGIN` | 登录请求 | Client → Server |
| 1006 | `MSG_CHAT_LOGIN_RSP` | 登录响应 | Server → Client |
| 1007 | `ID_SEARCH_USER_REQ` | 搜索用户（uid 或 name） | C → S |
| 1008 | `ID_SEARCH_USER_RSP` | 搜索结果 | S → C |
| 1009 | `ID_ADD_FRIEND_REQ` | 发起好友申请 | C → S |
| 1010 | `ID_ADD_FRIEND_RSP` | 申请提交结果 | S → C |
| 1011 | `ID_NOTIFY_ADD_FRIEND_REQ` | 被申请方实时通知 | S → C |
| 1013 | `ID_AUTH_FRIEND_REQ` | 同意/拒绝好友 | C → S |
| 1014 | `ID_AUTH_FRIEND_RSP` | 同意结果（含好友资料） | S → C |
| 1015 | `ID_NOTIFY_AUTH_FRIEND_REQ` | 申请方获批通知 | S → C |
| 1017 | `ID_TEXT_CHAT_MSG_REQ` | 发送文本消息 | C → S |
| 1018 | `ID_TEXT_CHAT_MSG_RSP` | 文本消息 ACK | S → C |
| 1019 | `ID_NOTIFY_TEXT_CHAT_MSG_REQ` | 文本消息推送 | S → C |
| 1021 | `ID_NOTIFY_OFF_LINE_REQ` | 异地登录踢下线 | S → C |
| 1023 | `ID_HEART_BEAT_REQ` | 心跳 | C → S |
| 1024 | `ID_HEARTBEAT_RSP` | 心跳响应 | S → C |

**心跳建议**：前端每 ~10s 发送一次 `ID_HEART_BEAT_REQ`，超过 20s 未心跳会被服务器清理。

### 4.4 消息体示例

#### 登录
```json
// MSG_CHAT_LOGIN
{"uid":1054,"token":"5a3c..."}

// MSG_CHAT_LOGIN_RSP
{
  "error":0,
  "uid":1054,
  "name":"sqy",
  "email":"3175614975@qq.com",
  "nick":"sqy",
  "desc":"",
  "sex":0,
  "icon":":/res/head_2.jpg",
  "pwd":"745230",
  "apply_list":[
    {"name":"userA","uid":1032,"icon":":/res/head_1.jpg","nick":"userA","sex":0,"desc":"","status":0}
  ],
  "friend_list":[
    {"name":"ocz","uid":1055,"icon":":/res/head_3.jpg","nick":"ocz","sex":0,"desc":"","back":"sqy"}
  ]
}
```

#### 搜索用户
```json
// ID_SEARCH_USER_REQ
{"uid":"1055"}   // 纯数字视为 uid，否则按用户名搜索

// ID_SEARCH_USER_RSP
{"error":0,"uid":1055,"name":"ocz","nick":"ocz","email":"...","sex":0,"icon":":/res/head_3.jpg"}
```

#### 好友申请与审批
```json
// ID_ADD_FRIEND_REQ
{"uid":1054,"applyname":"sqy","bakname":"sqy","touid":1055}

// ID_ADD_FRIEND_RSP
{"error":0}

// ID_NOTIFY_ADD_FRIEND_REQ （被申请方实时收到）
{"error":0,"applyuid":1054,"name":"sqy","desc":"","icon":":/res/head_2.jpg","sex":0,"nick":"sqy"}

// ID_AUTH_FRIEND_REQ
{"fromuid":1055,"touid":1054,"back":"兄弟"}

// ID_AUTH_FRIEND_RSP
{"error":0,"uid":1054,"name":"sqy","nick":"sqy","icon":":/res/head_2.jpg","sex":0}

// ID_NOTIFY_AUTH_FRIEND_REQ （申请方收到）
{"error":0,"fromuid":1055,"touid":1054,"name":"ocz","nick":"ocz","icon":":/res/head_3.jpg","sex":0}
```

#### 文本聊天
```json
// ID_TEXT_CHAT_MSG_REQ
{
  "fromuid":1054,
  "touid":1055,
  "text_array":[
    {"msgid":"local-uuid-1","content":"你好"},
    {"msgid":"local-uuid-2","content":"在吗？"}
  ]
}

// ID_TEXT_CHAT_MSG_RSP
{
  "error":0,
  "fromuid":1054,
  "touid":1055,
  "text_array":[
    {"msgid":"local-uuid-1","content":"你好"},
    {"msgid":"local-uuid-2","content":"在吗？"}
  ]
}

// ID_NOTIFY_TEXT_CHAT_MSG_REQ （接收方收到推送）
{
  "error":0,
  "fromuid":1054,
  "touid":1055,
  "text_array":[
    {"msgid":"local-uuid-1","content":"你好"},
    {"msgid":"local-uuid-2","content":"在吗？"}
  ]
}
```

#### 心跳与离线
```json
// ID_HEART_BEAT_REQ
{"fromuid":1054}

// ID_HEARTBEAT_RSP
{"error":0}

// ID_NOTIFY_OFF_LINE_REQ
{"error":0,"uid":1054}
```

### 4.5 后端消息处理回调速查

| 消息 ID | 后端处理函数 | 关键逻辑 | 典型错误码 | 备注 |
|---------|--------------|----------|------------|------|
| 1005 `MSG_CHAT_LOGIN` | `LogicSystem::LoginHandler` | 校验 Redis token、装载用户与好友数据、处理异地登录 | 1011（token 未找到）、1010（token 不匹配） | 若检测到旧连接会调用 `CSession::NotifyOffline` 踢出旧会话 |
| 1007 `ID_SEARCH_USER_REQ` | `LogicSystem::SearchInfo` | 根据是否纯数字判断 uid/name，优先读取 Redis 缓存 | 1011 | 找不到用户时返回 `UidInvalid` |
| 1009 `ID_ADD_FRIEND_REQ` | `LogicSystem::AddFriendApply` | 写 `friend_apply`，本节点直推或通过 gRPC 转发通知对方 | - | 通知失败不影响响应，前端需关注实时推送 |
| 1013 `ID_AUTH_FRIEND_REQ` | `LogicSystem::AuthFriendApply` | 更新申请状态、插入双向好友、通知申请方 | 1011 | 获取双方资料失败时返回错误 |
| 1017 `ID_TEXT_CHAT_MSG_REQ` | `LogicSystem::DealChatTextMsg` | 回 ACK、根据 `uip_<uid>` 路由消息（本节点或 gRPC） | - | 对方离线时 ACK 仍为 0，前端需结合在线状态 |
| 1023 `ID_HEART_BEAT_REQ` | `LogicSystem::HeartBeatHandler` | 更新 session 心跳，返回成功 | - | 超过 20s 无心跳，定时器会清理会话 |
| 1011 `ID_NOTIFY_ADD_FRIEND_REQ` | `ChatServiceImpl::NotifyAddFriend` | 被申请方所在节点通过本地 session 推送 | - | 前端需刷新申请列表 |
| 1015 `ID_NOTIFY_AUTH_FRIEND_REQ` | `ChatServiceImpl::NotifyAuthFriend` | 申请成功后推送给发起方 | 1011 | 若拉取对方资料失败返回错误 |
| 1019 `ID_NOTIFY_TEXT_CHAT_MSG_REQ` | `ChatServiceImpl::NotifyTextChatMsg` | 文本消息实时推送 | - | `textmsgs` 数组与请求中的 `text_array` 对应 |
| 1021 `ID_NOTIFY_OFF_LINE_REQ` | `CSession::NotifyOffline` | 异地登录或服务端清理触发 | - | 前端需立即跳转登录并提示 |

### 4.6 前端消息回调模板

推荐实现统一的分发器，根据消息 ID 调用对应的前端回调，并集中处理错误码：

```ts
type MsgHandler = (body: any) => void;

const handlers: Record<number, MsgHandler> = {
  1006: handleLoginRsp,
  1008: handleSearchRsp,
  1010: handleAddFriendRsp,
  1011: onIncomingFriendApply,
  1014: handleAuthRsp,
  1015: onFriendApproved,
  1018: handleTextAck,
  1019: onIncomingText,
  1021: onForceLogout,
  1024: onHeartbeatRsp,
};

function dispatchFrame(id: number, payload: string) {
  const body = JSON.parse(payload);
  if (body.error && body.error !== 0) {
    handleBusinessError(id, body);
    return;
  }
  handlers[id]?.(body);
}
```

常用回调建议：

| 回调名 | 消息 ID | 建议动作 |
|--------|---------|----------|
| `handleLoginRsp` | 1006 | 初始化会话、同步好友和申请列表、标记在线状态 |
| `handleSearchRsp` | 1008 | 更新搜索结果；error≠0 时提示“未找到用户” |
| `handleAddFriendRsp` | 1010 | 成功后禁用按钮、显示“等待对方处理”；失败恢复按钮 |
| `onIncomingFriendApply` | 1011 | 插入一条新申请，触发红点/声音提示 |
| `handleAuthRsp` | 1014 | 成功时将好友加入列表，错误时展示原因 |
| `onFriendApproved` | 1015 | 提示“对方已通过”，刷新好友列表 |
| `handleTextAck` | 1018 | 将对应 `msgid` 标记为“已送达” |
| `onIncomingText` | 1019 | 将消息追加到窗口、更新未读计数与提醒 |
| `onHeartbeatRsp` | 1024 | 更新时间戳用于心跳监控 |
| `onForceLogout` | 1021 | 清理缓存、断开连接、跳转登录页并展示提示 |

### 4.7 服务端/前端错误处理矩阵

| 场景 | 服务端行为 | 前端建议 |
|------|------------|----------|
| `MSG_CHAT_LOGIN_RSP` 返回 `error=1011` | Redis 未命中 token，直接返回 `UidInvalid` | 关闭连接，重新走 `/user_login`，提示“登录过期，请重新登录” |
| `MSG_CHAT_LOGIN_RSP` 返回 `error=1010` | Token 不匹配，可能异地登录 | 清理本地 token，提示“账号在其他设备登录”，跳转登录页 |
| `ID_TEXT_CHAT_MSG_RSP` 返回非 0（保留） | 消息落库/转发失败 | 将消息标记为发送失败，允许用户重发 |
| 收到 `ID_NOTIFY_OFF_LINE_REQ` | 服务端即将关闭连接 | 立即断开 socket，展示异地登录提示 |
| `ID_NOTIFY_AUTH_FRIEND_REQ` error=1011 | 服务端获取对方资料失败 | 提示“对方资料暂不可用”，提供手动刷新入口 |
| HTTP 接口返回 `RPCFailed` | 上游 gRPC 服务不可用 | 提供重试/反馈按钮，避免频繁请求 |

### 4.8 前端接入建议

1. **封装 HTTP SDK**：统一 `error` 码与文案（如 1003 → “验证码已过期”）。
2. **TCP 客户端单例**：为消息 ID 注册处理器，并支持断线重连（需重新登录获取 token）。
3. **消息发送确认**：以 `msgid` 关联请求与 ACK，更新消息状态（发送中/成功/失败）。
4. **缓存 Session 信息**：持久化 `{uid, token, host, port}`，重启后可快速重连。
5. **异常处理**：处理 `TokenInvalid`、`UidInvalid`、`ID_NOTIFY_OFF_LINE_REQ` 等，及时回到登录态。
6. **心跳策略**：建议 10s 左右发送一次；若多次心跳失败则断线重连。

---

## 5. StatusServer 接口说明（gRPC）

虽然前端无需直接调用，但理解流程有助于排查问题：

| RPC | 请求 | 响应 | 说明 |
|-----|------|------|------|
| `GetChatServer(GetChatServerReq)` | `{ uid }` | `{ error, host, port, token }` | GateServer 登录时调用；`token` 写入 Redis `utoken_<uid>` |
| `Login(LoginReq)` | `{ uid, token }` | `{ error, uid, token }` | ChatServer 登录校验时调用；若 `token` 无效返回 `UidInvalid/TokenInvalid` |

---

## 6. 部署与测试流程示例

1. **准备依赖**：启动 MySQL、Redis，导入 `llfc.sql`，安装 Node.js 依赖（`cd server/VarifyServer && npm install`）。
2. **启动后端**：
   - `VarifyServer`: `node server.js`
   - `StatusServer`: `./statusserver`
   - `ChatServer`: 为每个节点构建并运行（如 `./chatserver1`）
   - `GateServer`: `./gateserver`
3. **验证 HTTP 接口**：
   - `/get_varifycode` → 邮件和 Redis 验证
   - `/user_register` → 新增用户
   - `/user_login` → 获取聊天节点地址与 token
4. **验证主服务**：
   - 建立 TCP 连接 → 发送登录帧 → 接收 `MSG_CHAT_LOGIN_RSP`
   - 发送心跳、好友申请、文本消息，确认对应推送到达
- **前端联调**：
   - 在 `bit-tieba` 中实现上述流程
   - 对接 UI（验证码、注册、登录、消息列表、好友申请等）

---

## 7. 常见问题 & 排查思路

| 问题表现 | 排查建议 |
|----------|----------|
| `/user_login` 返回 `RPCFailed` | 检查 StatusServer 是否启动、gRPC 端口是否可达 |
| 登录成功但 TCP 登录返回 `TokenInvalid` | 确认 Redis `utoken_<uid>` 是否存在；若手动清理 Redis，需重新登录获取新 token |
| 发送好友申请无推送 | 查看 `uip_<uid>` 是否指向正确 ChatServer 节点；若在其他节点，确认 gRPC 端口互通 |
| 心跳超时被踢 | 确保前端按约定发送心跳；排查客户端读写逻辑是否阻塞 |

---

## 8. 后续扩展建议

- **消息持久化/历史记录**：当前仅实时转发，可在 ChatServer 中引入消息写库或 Kafka。
- **Token 生命周期**：默认无过期策略，可在 StatusServer 生成带时效的 token 并周期刷新。
- **分布式部署**：ChatServer 可横向扩容；注意更新 StatusServer `config.ini` 的聊天节点列表并保持 gRPC 互通。
- **HTTP 接口鉴权**：视需求可在 GateServer 增加签名、限流等保护措施。

---

如需调整或扩展接口，请保持与现有 Redis/MySQL 数据模型兼容，并同步更新本文档。
