# 贴吧功能测试指南

本文档提供完整的贴吧功能测试指南，包括如何运行服务和测试所有贴吧相关功能。

## 目录

1. [环境准备](#环境准备)
2. [数据库初始化](#数据库初始化)
3. [编译项目](#编译项目)
4. [运行服务](#运行服务)
5. [测试贴吧功能](#测试贴吧功能)
6. [API接口文档](#api接口文档)
7. [TCP消息测试](#tcp消息测试)
8. [常见问题](#常见问题)

---

## 环境准备

### 1. 确保数据库和Redis已启动

```bash
# 检查MySQL是否运行
sudo systemctl status mysql
# 或
mysql -h127.0.0.1 -P3308 -uroot -p123456. -e "SELECT 1"

# 检查Redis是否运行
redis-cli -h 127.0.0.1 -p 6380 -a 123456 PING
```

### 2. 导入数据库表结构

```bash
# 导入贴吧相关表结构
mysql -h127.0.0.1 -P3308 -uroot -p123456. llfc < server/sql/tieba_tables.sql
```

### 3. 验证表结构

```bash
mysql -h127.0.0.1 -P3308 -uroot -p123456. llfc -e "SHOW TABLES LIKE 'tieba%'"
mysql -h127.0.0.1 -P3308 -uroot -p123456. llfc -e "SHOW TABLES LIKE 'post%'"
mysql -h127.0.0.1 -P3308 -uroot -p123456. llfc -e "SHOW TABLES LIKE 'reply%'"
```

---

## 数据库初始化

### 1. 创建数据库表

执行以下SQL文件创建贴吧相关表：

```bash
mysql -h127.0.0.1 -P3308 -uroot -p123456. llfc < server/sql/tieba_tables.sql
```

### 2. 验证表结构

```sql
-- 查看贴吧表结构
DESC tieba;

-- 查看帖子表结构
DESC tieba_post;

-- 查看回复表结构
DESC tieba_reply;

-- 查看点赞表结构
DESC tieba_like;

-- 查看成员表结构
DESC tieba_member;
```

---

## 编译项目

### 1. 重新生成protobuf文件

```bash
cd server
for dir in GateServer StatusServer ChatServer ChatServer2; do
  cd "$dir"
  protoc --proto_path=. \
    --cpp_out=. \
    --grpc_out=. \
    --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
    message.proto
  cd ..
done
```

### 2. 编译项目

```bash
cd server/build
cmake --build . --target gateserver
cmake --build . --target statusserver
cmake --build . --target chatserver1
cmake --build . --target chatserver2
```

### 3. 验证编译结果

```bash
ls -la GateServer/gateserver
ls -la StatusServer/statusserver
ls -la ChatServer/chatserver1
ls -la ChatServer2/chatserver2
```

---

## 运行服务

### 1. 启动顺序

按照以下顺序启动服务：

1. **MySQL** (如果未启动)
2. **Redis** (如果未启动)
3. **VarifyServer** (Node.js服务)
4. **StatusServer** (gRPC服务)
5. **ChatServer** (TCP服务)
6. **ChatServer2** (TCP服务，可选)
7. **GateServer** (HTTP网关)

### 2. 启动命令

```bash
# 1. 启动VarifyServer
cd server/VarifyServer
npm install
npm run serve

# 2. 启动StatusServer
cd server/StatusServer
./statusserver

# 3. 启动ChatServer
cd server/ChatServer
./chatserver1

# 4. 启动ChatServer2（可选）
cd server/ChatServer2
./chatserver2

# 5. 启动GateServer
cd server/GateServer
./gateserver
```

### 3. 验证服务启动

```bash
# 检查GateServer
curl http://127.0.0.1:8080

# 检查gRPC服务
grpcurl -plaintext 127.0.0.1:50051 list
grpcurl -plaintext 127.0.0.1:50052 list
```

---

## 测试贴吧功能

### 1. 用户注册和登录

首先需要注册用户并登录，获取uid和token：

```bash
# 1. 获取验证码
curl -X POST http://127.0.0.1:8080/get_varifycode \
  -H "Content-Type: application/json" \
  -d '{"email":"test@example.com"}'

# 2. 用户注册
curl -X POST http://127.0.0.1:8080/user_register \
  -H "Content-Type: application/json" \
  -d '{
    "email":"test@example.com",
    "user":"testuser",
    "passwd":"password123",
    "confirm":"password123",
    "icon":":/res/head_1.jpg",
    "varifycode":"1234"
  }'

# 3. 用户登录
curl -X POST http://127.0.0.1:8080/user_login \
  -H "Content-Type: application/json" \
  -d '{
    "email":"test@example.com",
    "passwd":"password123"
  }'
```

预期响应（登录）：
```json
{
  "error": 0,
  "email": "test@example.com",
  "uid": 1054,
  "token": "...",
  "host": "127.0.0.1",
  "port": "8090"
}
```

### 2. 创建贴吧

**注意**: 创建贴吧功能需要在ChatServer中通过TCP消息实现，GateServer中暂未实现HTTP接口。

### 3. 搜索贴吧

**注意**: 搜索贴吧功能需要在ChatServer中通过TCP消息实现，GateServer中暂未实现HTTP接口。

### 4. 加入贴吧

**注意**: 加入贴吧功能需要在ChatServer中通过TCP消息实现。

### 5. 发帖

**注意**: 发帖功能需要在ChatServer中通过TCP消息实现。

### 6. 回复

**注意**: 回复功能需要在ChatServer中通过TCP消息实现。

### 7. 点赞

**注意**: 点赞功能需要在ChatServer中通过TCP消息实现。

### 8. 置顶/加精

**注意**: 置顶/加精功能需要在ChatServer中通过TCP消息实现，只有吧主或小吧主可以操作。

---

## API接口文档

### GateServer HTTP接口

#### 1. 创建贴吧 (待实现)

```http
POST /tieba/create
Content-Type: application/json

{
  "uid": 1054,
  "tieba_name": "测试贴吧",
  "tieba_desc": "这是一个测试贴吧",
  "tieba_icon": ":/res/tieba_icon.jpg"
}
```

#### 2. 搜索贴吧 (待实现)

```http
POST /tieba/search
Content-Type: application/json

{
  "keyword": "测试",
  "offset": 0,
  "limit": 20
}
```

#### 3. 获取贴吧信息 (待实现)

```http
GET /tieba/info?tieba_id=1
```

或

```http
GET /tieba/info?tieba_name=测试贴吧
```

#### 4. 获取我的贴吧列表 (待实现)

```http
GET /tieba/my?uid=1054
```

---

## TCP消息测试

### 1. 连接ChatServer

使用telnet或nc连接到ChatServer：

```bash
telnet 127.0.0.1 8090
# 或
nc 127.0.0.1 8090
```

### 2. 发送消息格式

TCP消息格式：
- 总长度 (4字节)
- 消息ID (2字节)
- 数据长度 (2字节)
- 数据内容 (JSON格式)

### 3. 测试消息

#### 创建贴吧

```json
{
  "uid": 1054,
  "tieba_name": "测试贴吧",
  "tieba_desc": "这是一个测试贴吧",
  "tieba_icon": ":/res/tieba_icon.jpg"
}
```

消息ID: `1025` (ID_CREATE_TIEBA_REQ)

#### 搜索贴吧

```json
{
  "keyword": "测试",
  "offset": 0,
  "limit": 20
}
```

消息ID: `1027` (ID_SEARCH_TIEBA_REQ)

#### 加入贴吧

```json
{
  "uid": 1054,
  "tieba_id": 1
}
```

消息ID: `1029` (ID_JOIN_TIEBA_REQ)

#### 发帖

```json
{
  "uid": 1054,
  "tieba_id": 1,
  "title": "测试帖子",
  "content": "这是测试帖子内容"
}
```

消息ID: `1033` (ID_CREATE_POST_REQ)

#### 回复

```json
{
  "uid": 1054,
  "post_id": 1,
  "content": "这是回复内容",
  "reply_to_uid": 0,
  "reply_to_reply_id": 0
}
```

消息ID: `1035` (ID_CREATE_REPLY_REQ)

#### 点赞

```json
{
  "uid": 1054,
  "target_type": 1,
  "target_id": 1,
  "is_like": true
}
```

消息ID: `1039` (ID_LIKE_REQ)

- `target_type`: 1-帖子, 2-回复
- `target_id`: 帖子ID或回复ID
- `is_like`: true-点赞, false-取消点赞

#### 获取帖子列表

```json
{
  "uid": 1054,
  "tieba_id": 1,
  "offset": 0,
  "limit": 20,
  "sort_type": 0
}
```

消息ID: `1041` (ID_GET_POST_LIST_REQ)

- `sort_type`: 0-按时间倒序, 1-按回复数倒序, 2-按点赞数倒序

#### 获取回复列表

```json
{
  "uid": 1054,
  "post_id": 1,
  "offset": 0,
  "limit": 20
}
```

消息ID: `1043` (ID_GET_REPLY_LIST_REQ)

#### 置顶

```json
{
  "uid": 1054,
  "post_id": 1,
  "tieba_id": 1,
  "is_top": true
}
```

消息ID: `1045` (ID_SET_TOP_REQ)

#### 加精

```json
{
  "uid": 1054,
  "post_id": 1,
  "tieba_id": 1,
  "is_essence": true
}
```

消息ID: `1047` (ID_SET_ESSENCE_REQ)

#### 删除帖子

```json
{
  "uid": 1054,
  "post_id": 1,
  "tieba_id": 1
}
```

消息ID: `1037` (ID_DELETE_POST_REQ)

#### 删除回复

```json
{
  "uid": 1054,
  "reply_id": 1,
  "post_id": 1
}
```

消息ID: `1055` (ID_DELETE_REPLY_REQ)

---

## 常见问题

### 1. 数据库连接失败

**问题**: 无法连接到MySQL数据库

**解决方案**:
- 检查MySQL是否运行
- 检查配置文件中的数据库连接信息
- 检查防火墙设置

### 2. 表不存在

**问题**: 提示表不存在

**解决方案**:
- 执行SQL文件创建表结构
- 检查数据库名称是否正确

### 3. 权限不足

**问题**: 提示权限不足

**解决方案**:
- 检查用户是否为贴吧成员
- 检查用户角色（吧主/小吧主/普通成员）
- 检查操作权限（只有吧主/小吧主可以置顶/加精/删除帖子）

### 4. 贴吧已存在

**问题**: 创建贴吧时提示贴吧已存在

**解决方案**:
- 检查贴吧名称是否已存在
- 使用其他名称创建贴吧

### 5. 不是贴吧成员

**问题**: 提示不是贴吧成员

**解决方案**:
- 先加入贴吧
- 检查用户是否已加入贴吧

### 6. 帖子不存在

**问题**: 提示帖子不存在

**解决方案**:
- 检查帖子ID是否正确
- 检查帖子是否已被删除

---

## 测试流程示例

### 完整测试流程

1. **用户注册和登录**
   ```bash
   # 获取验证码
   curl -X POST http://127.0.0.1:8080/get_varifycode \
     -H "Content-Type: application/json" \
     -d '{"email":"test@example.com"}'
   
   # 注册用户
   curl -X POST http://127.0.0.1:8080/user_register \
     -H "Content-Type: application/json" \
     -d '{
       "email":"test@example.com",
       "user":"testuser",
       "passwd":"password123",
       "confirm":"password123",
       "icon":":/res/head_1.jpg",
       "varifycode":"1234"
     }'
   
   # 登录
   curl -X POST http://127.0.0.1:8080/user_login \
     -H "Content-Type: application/json" \
     -d '{
       "email":"test@example.com",
       "passwd":"password123"
     }'
   ```

2. **连接ChatServer**
   ```bash
   telnet 127.0.0.1 8090
   ```

3. **创建贴吧** (通过TCP消息)
   - 发送创建贴吧消息
   - 接收响应，获取贴吧ID

4. **加入贴吧** (通过TCP消息)
   - 发送加入贴吧消息
   - 接收响应

5. **发帖** (通过TCP消息)
   - 发送发帖消息
   - 接收响应，获取帖子ID

6. **回复** (通过TCP消息)
   - 发送回复消息
   - 接收响应，获取回复ID

7. **点赞** (通过TCP消息)
   - 发送点赞消息
   - 接收响应

8. **获取帖子列表** (通过TCP消息)
   - 发送获取帖子列表消息
   - 接收响应，查看帖子列表

9. **获取回复列表** (通过TCP消息)
   - 发送获取回复列表消息
   - 接收响应，查看回复列表

10. **置顶/加精** (通过TCP消息，需要吧主权限)
    - 发送置顶/加精消息
    - 接收响应

---

## 注意事项

1. **权限检查**: 确保测试用户有足够的权限执行操作
2. **数据清理**: 测试完成后清理测试数据
3. **错误处理**: 测试各种错误情况（权限不足、数据不存在等）
4. **并发测试**: 测试并发情况下的性能
5. **边界测试**: 测试边界条件（空字符串、最大值等）

---

## 下一步

1. **实现GateServer HTTP接口**: 在GateServer中添加贴吧相关的HTTP接口
2. **实现ChatServer TCP消息处理**: 在ChatServer中实现所有贴吧相关的TCP消息处理
3. **添加Redis缓存**: 使用Redis缓存热门贴吧和帖子列表
4. **性能优化**: 优化数据库查询性能
5. **安全加固**: 添加接口限流、防止SQL注入等安全措施

---

**祝测试顺利！**

如有任何问题或建议，欢迎反馈。

