# 贴吧功能测试指南

## 测试环境准备

### 1. 数据库准备
```bash
# 连接MySQL数据库
mysql -h127.0.0.1 -P3308 -uroot -p

# 创建数据库（如果不存在）
CREATE DATABASE IF NOT EXISTS llfc;
USE llfc;

# 导入贴吧表结构
source sql/tieba_tables.sql;

# 验证表是否创建成功
SHOW TABLES LIKE 'tieba%';
SHOW TABLES LIKE '%_id';
```

### 2. 启动服务
```bash
# 按顺序启动服务
cd server

# 1. 启动StatusServer
cd StatusServer
./statusserver &

# 2. 启动ChatServer
cd ../ChatServer
./chatserver1 &

# 3. 启动GateServer
cd ../GateServer
./gateserver &
```

### 3. 准备测试数据
```sql
-- 插入测试用户（如果不存在）
INSERT INTO user (uid, name, email, pwd, nick, icon) VALUES 
(1001, 'testuser1', 'test1@test.com', '123456', '测试用户1', 'icon1.png'),
(1002, 'testuser2', 'test2@test.com', '123456', '测试用户2', 'icon2.png'),
(1003, 'testuser3', 'test3@test.com', '123456', '测试用户3', 'icon3.png');
```

## 测试用例

### 测试1: 创建贴吧

**测试目的**: 验证用户可以成功创建贴吧

**消息ID**: `ID_CREATE_TIEBA_REQ` (1025)

**请求JSON**:
```json
{
  "uid": 1001,
  "tieba_name": "C++学习吧",
  "tieba_desc": "C++编程学习交流社区",
  "tieba_icon": "cpp_icon.png"
}
```

**期望响应**:
```json
{
  "error": 0,
  "tieba_id": 1,
  "tieba_name": "C++学习吧"
}
```

**验证点**:
- ✓ error为0表示成功
- ✓ 返回新创建的tieba_id
- ✓ 数据库中tieba表新增一条记录
- ✓ tieba_member表自动添加创建者为吧主(role=2)
- ✓ member_count为1

**数据库验证**:
```sql
SELECT * FROM tieba WHERE tieba_id = 1;
SELECT * FROM tieba_member WHERE tieba_id = 1;
SELECT * FROM tieba_id;
```

---

### 测试2: 创建重复贴吧（失败场景）

**测试目的**: 验证不能创建同名贴吧

**请求JSON**:
```json
{
  "uid": 1002,
  "tieba_name": "C++学习吧",
  "tieba_desc": "重复的贴吧",
  "tieba_icon": ""
}
```

**期望响应**:
```json
{
  "error": 1012
}
```

**验证点**:
- ✓ error为1012 (TiebaExist)
- ✓ 数据库中贴吧数量不变

---

### 测试3: 搜索贴吧

**测试目的**: 验证可以通过关键词搜索贴吧

**消息ID**: `ID_SEARCH_TIEBA_REQ` (1027)

**请求JSON**:
```json
{
  "keyword": "C++",
  "offset": 0,
  "limit": 10
}
```

**期望响应**:
```json
{
  "error": 0,
  "total": 1,
  "tieba_list": [
    {
      "tieba_id": 1,
      "tieba_name": "C++学习吧",
      "tieba_desc": "C++编程学习交流社区",
      "tieba_icon": "cpp_icon.png",
      "owner_uid": 1001,
      "member_count": 1,
      "post_count": 0,
      "create_time": "2025-11-13 10:00:00"
    }
  ]
}
```

**验证点**:
- ✓ 搜索结果包含匹配的贴吧
- ✓ total字段正确显示总数
- ✓ 支持分页（offset和limit）
- ✓ 按创建时间倒序排列

---

### 测试4: 加入贴吧

**测试目的**: 验证用户可以加入已存在的贴吧

**消息ID**: `ID_JOIN_TIEBA_REQ` (1029)

**请求JSON**:
```json
{
  "uid": 1002,
  "tieba_id": 1
}
```

**期望响应**:
```json
{
  "error": 0,
  "tieba_id": 1
}
```

**验证点**:
- ✓ tieba_member表新增记录
- ✓ 该用户role为0（普通成员）
- ✓ tieba表的member_count增加1

**数据库验证**:
```sql
SELECT * FROM tieba_member WHERE tieba_id = 1 AND uid = 1002;
SELECT member_count FROM tieba WHERE tieba_id = 1;
```

---

### 测试5: 获取贴吧信息

**测试目的**: 验证可以获取贴吧详细信息和用户角色

**消息ID**: `ID_GET_TIEBA_INFO_REQ` (1049)

**请求JSON**:
```json
{
  "uid": 1002,
  "tieba_id": 1
}
```

**期望响应**:
```json
{
  "error": 0,
  "tieba_info": {
    "tieba_id": 1,
    "tieba_name": "C++学习吧",
    "tieba_desc": "C++编程学习交流社区",
    "tieba_icon": "cpp_icon.png",
    "owner_uid": 1001,
    "member_count": 2,
    "post_count": 0,
    "create_time": "2025-11-13 10:00:00"
  },
  "is_member": true,
  "member_role": 0
}
```

**验证点**:
- ✓ 返回完整贴吧信息
- ✓ is_member正确反映成员状态
- ✓ member_role正确显示用户角色

---

### 测试6: 发帖

**测试目的**: 验证贴吧成员可以发帖

**消息ID**: `ID_CREATE_POST_REQ` (1033)

**请求JSON**:
```json
{
  "uid": 1002,
  "tieba_id": 1,
  "title": "如何学习C++模板编程？",
  "content": "最近在学习C++模板，感觉很难理解，有没有好的学习资料推荐？"
}
```

**期望响应**:
```json
{
  "error": 0,
  "post_id": 1,
  "tieba_id": 1
}
```

**验证点**:
- ✓ 返回新创建的post_id
- ✓ tieba_post表新增记录
- ✓ tieba表的post_count增加1
- ✓ post_id表的id自增

**数据库验证**:
```sql
SELECT * FROM tieba_post WHERE post_id = 1;
SELECT post_count FROM tieba WHERE tieba_id = 1;
SELECT * FROM post_id;
```

---

### 测试7: 非成员发帖（失败场景）

**测试目的**: 验证非成员不能发帖

**请求JSON**:
```json
{
  "uid": 1003,
  "tieba_id": 1,
  "title": "测试帖子",
  "content": "我不是成员"
}
```

**期望响应**:
```json
{
  "error": 1015
}
```

**验证点**:
- ✓ error为1015 (NotTiebaMember)
- ✓ 数据库中帖子数量不变

---

### 测试8: 获取帖子列表

**测试目的**: 验证可以获取贴吧的帖子列表

**消息ID**: `ID_GET_POST_LIST_REQ` (1041)

**请求JSON**:
```json
{
  "uid": 1002,
  "tieba_id": 1,
  "offset": 0,
  "limit": 20,
  "sort_type": 0
}
```

**期望响应**:
```json
{
  "error": 0,
  "total": 1,
  "post_list": [
    {
      "post_id": 1,
      "tieba_id": 1,
      "uid": 1002,
      "user_name": "testuser2",
      "user_icon": "icon2.png",
      "title": "如何学习C++模板编程？",
      "content": "最近在学习C++模板，感觉很难理解，有没有好的学习资料推荐？",
      "reply_count": 0,
      "like_count": 0,
      "is_top": false,
      "is_essence": false,
      "create_time": "2025-11-13 10:05:00",
      "is_liked": false
    }
  ]
}
```

**验证点**:
- ✓ 返回帖子列表
- ✓ 包含用户信息（JOIN查询）
- ✓ is_liked字段正确
- ✓ 支持分页
- ✓ sort_type控制排序方式

---

### 测试9: 回复帖子

**测试目的**: 验证可以回复帖子

**消息ID**: `ID_CREATE_REPLY_REQ` (1035)

**请求JSON**:
```json
{
  "uid": 1001,
  "post_id": 1,
  "content": "推荐《C++ Templates: The Complete Guide》这本书，讲解很详细。",
  "reply_to_uid": 0,
  "reply_to_reply_id": 0
}
```

**期望响应**:
```json
{
  "error": 0,
  "reply_id": 1,
  "post_id": 1
}
```

**验证点**:
- ✓ 返回新创建的reply_id
- ✓ tieba_reply表新增记录
- ✓ tieba_post表的reply_count增加1
- ✓ reply_id表的id自增

**数据库验证**:
```sql
SELECT * FROM tieba_reply WHERE reply_id = 1;
SELECT reply_count FROM tieba_post WHERE post_id = 1;
```

---

### 测试10: 回复其他回复

**测试目的**: 验证可以回复其他人的回复

**请求JSON**:
```json
{
  "uid": 1002,
  "post_id": 1,
  "content": "感谢推荐！我去找找这本书。",
  "reply_to_uid": 1001,
  "reply_to_reply_id": 1
}
```

**期望响应**:
```json
{
  "error": 0,
  "reply_id": 2,
  "post_id": 1
}
```

**验证点**:
- ✓ reply_to_uid和reply_to_reply_id正确记录

---

### 测试11: 获取回复列表

**测试目的**: 验证可以获取帖子的回复列表

**消息ID**: `ID_GET_REPLY_LIST_REQ` (1043)

**请求JSON**:
```json
{
  "uid": 1002,
  "post_id": 1,
  "offset": 0,
  "limit": 20
}
```

**期望响应**:
```json
{
  "error": 0,
  "total": 2,
  "post_info": {
    "post_id": 1,
    "title": "如何学习C++模板编程？",
    "content": "...",
    "reply_count": 2,
    "like_count": 0
  },
  "reply_list": [
    {
      "reply_id": 1,
      "post_id": 1,
      "uid": 1001,
      "user_name": "testuser1",
      "user_icon": "icon1.png",
      "content": "推荐《C++ Templates: The Complete Guide》这本书，讲解很详细。",
      "reply_to_uid": 0,
      "reply_to_user_name": "",
      "reply_to_reply_id": 0,
      "like_count": 0,
      "create_time": "2025-11-13 10:10:00",
      "is_liked": false
    },
    {
      "reply_id": 2,
      "post_id": 1,
      "uid": 1002,
      "user_name": "testuser2",
      "user_icon": "icon2.png",
      "content": "感谢推荐！我去找找这本书。",
      "reply_to_uid": 1001,
      "reply_to_user_name": "testuser1",
      "reply_to_reply_id": 1,
      "like_count": 0,
      "create_time": "2025-11-13 10:11:00",
      "is_liked": false
    }
  ]
}
```

**验证点**:
- ✓ 同时返回帖子信息和回复列表
- ✓ 回复按时间正序排列
- ✓ 包含回复目标用户信息

---

### 测试12: 点赞帖子

**测试目的**: 验证可以对帖子点赞

**消息ID**: `ID_LIKE_REQ` (1039)

**请求JSON**:
```json
{
  "uid": 1001,
  "target_type": 1,
  "target_id": 1,
  "is_like": true
}
```

**期望响应**:
```json
{
  "error": 0,
  "target_type": 1,
  "target_id": 1,
  "is_like": true,
  "like_count": 1
}
```

**验证点**:
- ✓ tieba_like表新增记录
- ✓ tieba_post表的like_count增加1
- ✓ 返回当前点赞总数

**数据库验证**:
```sql
SELECT * FROM tieba_like WHERE uid = 1001 AND target_type = 1 AND target_id = 1;
SELECT like_count FROM tieba_post WHERE post_id = 1;
```

---

### 测试13: 取消点赞

**测试目的**: 验证可以取消点赞

**请求JSON**:
```json
{
  "uid": 1001,
  "target_type": 1,
  "target_id": 1,
  "is_like": false
}
```

**期望响应**:
```json
{
  "error": 0,
  "target_type": 1,
  "target_id": 1,
  "is_like": false,
  "like_count": 0
}
```

**验证点**:
- ✓ tieba_like表删除记录
- ✓ tieba_post表的like_count减少1

---

### 测试14: 点赞回复

**测试目的**: 验证可以对回复点赞

**请求JSON**:
```json
{
  "uid": 1002,
  "target_type": 2,
  "target_id": 1,
  "is_like": true
}
```

**期望响应**:
```json
{
  "error": 0,
  "target_type": 2,
  "target_id": 1,
  "is_like": true,
  "like_count": 1
}
```

**验证点**:
- ✓ target_type为2表示回复
- ✓ tieba_reply表的like_count增加1

---

### 测试15: 置顶帖子（吧主操作）

**测试目的**: 验证吧主可以置顶帖子

**消息ID**: `ID_SET_TOP_REQ` (1045)

**请求JSON**:
```json
{
  "uid": 1001,
  "post_id": 1,
  "tieba_id": 1,
  "is_top": true
}
```

**期望响应**:
```json
{
  "error": 0,
  "post_id": 1,
  "is_top": true
}
```

**验证点**:
- ✓ tieba_post表的is_top字段更新为1
- ✓ 只有吧主或小吧主可以操作

**数据库验证**:
```sql
SELECT is_top FROM tieba_post WHERE post_id = 1;
```

---

### 测试16: 普通成员置顶（失败场景）

**测试目的**: 验证普通成员不能置顶

**请求JSON**:
```json
{
  "uid": 1002,
  "post_id": 1,
  "tieba_id": 1,
  "is_top": true
}
```

**期望响应**:
```json
{
  "error": 1020
}
```

**验证点**:
- ✓ error为1020 (PermissionDenied)
- ✓ is_top状态不变

---

### 测试17: 加精帖子（吧主操作）

**测试目的**: 验证吧主可以加精帖子

**消息ID**: `ID_SET_ESSENCE_REQ` (1047)

**请求JSON**:
```json
{
  "uid": 1001,
  "post_id": 1,
  "tieba_id": 1,
  "is_essence": true
}
```

**期望响应**:
```json
{
  "error": 0,
  "post_id": 1,
  "is_essence": true
}
```

**验证点**:
- ✓ tieba_post表的is_essence字段更新为1

---

### 测试18: 获取我的贴吧列表

**测试目的**: 验证可以获取用户加入的所有贴吧

**消息ID**: `ID_GET_MY_TIEBA_LIST_REQ` (1051)

**请求JSON**:
```json
{
  "uid": 1002
}
```

**期望响应**:
```json
{
  "error": 0,
  "tieba_list": [
    {
      "tieba_id": 1,
      "tieba_name": "C++学习吧",
      "tieba_desc": "C++编程学习交流社区",
      "tieba_icon": "cpp_icon.png",
      "owner_uid": 1001,
      "member_count": 2,
      "post_count": 1,
      "create_time": "2025-11-13 10:00:00"
    }
  ]
}
```

**验证点**:
- ✓ 返回用户作为成员的所有贴吧
- ✓ 按创建时间倒序排列

---

### 测试19: 获取贴吧成员列表

**测试目的**: 验证可以获取贴吧的成员列表

**消息ID**: `ID_GET_TIEBA_MEMBER_LIST_REQ` (1053)

**请求JSON**:
```json
{
  "uid": 1001,
  "tieba_id": 1,
  "offset": 0,
  "limit": 20
}
```

**期望响应**:
```json
{
  "error": 0,
  "total": 2,
  "member_list": [
    {
      "uid": 1001,
      "user_name": "testuser1",
      "user_icon": "icon1.png",
      "role": 2,
      "join_time": "2025-11-13 10:00:00"
    },
    {
      "uid": 1002,
      "user_name": "testuser2",
      "user_icon": "icon2.png",
      "role": 0,
      "join_time": "2025-11-13 10:03:00"
    }
  ]
}
```

**验证点**:
- ✓ 按角色倒序、加入时间正序排列
- ✓ 吧主排在最前面
- ✓ 包含用户详细信息

---

### 测试20: 删除回复（作者删除）

**测试目的**: 验证回复作者可以删除自己的回复

**消息ID**: `ID_DELETE_REPLY_REQ` (1055)

**请求JSON**:
```json
{
  "uid": 1002,
  "reply_id": 2,
  "post_id": 1
}
```

**期望响应**:
```json
{
  "error": 0,
  "reply_id": 2
}
```

**验证点**:
- ✓ tieba_reply表删除记录
- ✓ tieba_post表的reply_count减少1
- ✓ tieba_like表删除相关点赞记录

**数据库验证**:
```sql
SELECT * FROM tieba_reply WHERE reply_id = 2;
SELECT reply_count FROM tieba_post WHERE post_id = 1;
SELECT * FROM tieba_like WHERE target_type = 2 AND target_id = 2;
```

---

### 测试21: 删除帖子（吧主删除）

**测试目的**: 验证吧主可以删除任何帖子

**消息ID**: `ID_DELETE_POST_REQ` (1037)

**请求JSON**:
```json
{
  "uid": 1001,
  "post_id": 1,
  "tieba_id": 1
}
```

**期望响应**:
```json
{
  "error": 0,
  "post_id": 1
}
```

**验证点**:
- ✓ tieba_post表删除记录
- ✓ tieba_reply表删除所有相关回复
- ✓ tieba_like表删除所有相关点赞
- ✓ tieba表的post_count减少1

**数据库验证**:
```sql
SELECT * FROM tieba_post WHERE post_id = 1;
SELECT * FROM tieba_reply WHERE post_id = 1;
SELECT * FROM tieba_like WHERE target_type = 1 AND target_id = 1;
SELECT post_count FROM tieba WHERE tieba_id = 1;
```

---

### 测试22: 退出贴吧

**测试目的**: 验证普通成员可以退出贴吧

**消息ID**: `ID_LEAVE_TIEBA_REQ` (1031)

**请求JSON**:
```json
{
  "uid": 1002,
  "tieba_id": 1
}
```

**期望响应**:
```json
{
  "error": 0,
  "tieba_id": 1
}
```

**验证点**:
- ✓ tieba_member表删除记录
- ✓ tieba表的member_count减少1

**数据库验证**:
```sql
SELECT * FROM tieba_member WHERE tieba_id = 1 AND uid = 1002;
SELECT member_count FROM tieba WHERE tieba_id = 1;
```

---

### 测试23: 吧主退出贴吧（失败场景）

**测试目的**: 验证吧主不能退出自己的贴吧

**请求JSON**:
```json
{
  "uid": 1001,
  "tieba_id": 1
}
```

**期望响应**:
```json
{
  "error": 1020
}
```

**验证点**:
- ✓ error为1020 (PermissionDenied)
- ✓ 吧主成员记录不变

---

## 性能测试

### 并发发帖测试

**测试目的**: 验证系统在高并发下的稳定性

**测试方法**:
1. 创建100个用户
2. 所有用户同时加入同一个贴吧
3. 所有用户同时发帖
4. 验证post_count和post_id的一致性

**期望结果**:
- ✓ 所有帖子都成功创建
- ✓ post_id连续无重复
- ✓ post_count正确
- ✓ 无死锁发生

---

### 并发点赞测试

**测试目的**: 验证点赞计数的准确性

**测试方法**:
1. 100个用户同时对同一个帖子点赞
2. 验证like_count

**期望结果**:
- ✓ like_count为100
- ✓ tieba_like表有100条记录
- ✓ 无重复点赞

---

## 边界测试

### 测试1: 超长贴吧名称

**请求JSON**:
```json
{
  "uid": 1001,
  "tieba_name": "这是一个非常非常非常非常非常非常非常非常非常非常长的贴吧名称超过50个字符",
  "tieba_desc": "测试",
  "tieba_icon": ""
}
```

**期望响应**:
```json
{
  "error": 1014
}
```

---

### 测试2: 空标题发帖

**请求JSON**:
```json
{
  "uid": 1002,
  "tieba_id": 1,
  "title": "",
  "content": "内容"
}
```

**期望响应**:
```json
{
  "error": 1001
}
```

---

### 测试3: 超大分页请求

**请求JSON**:
```json
{
  "uid": 1002,
  "tieba_id": 1,
  "offset": 0,
  "limit": 10000
}
```

**期望行为**:
- ✓ 自动限制limit为100
- ✓ 正常返回结果

---

## 集成测试流程

### 完整业务流程测试

1. **用户A创建贴吧**
2. **用户B、C搜索并加入贴吧**
3. **用户B发帖**
4. **用户C回复帖子**
5. **用户A点赞帖子和回复**
6. **用户A将帖子置顶并加精**
7. **获取帖子列表，验证置顶帖在最前面**
8. **用户B删除自己的帖子**
9. **验证回复也被删除**
10. **用户C退出贴吧**
11. **验证成员数减少**

---

## 测试工具推荐

### 1. 使用Postman测试（如果有HTTP接口）
创建Postman Collection，包含所有测试用例

### 2. 使用TCP客户端测试
编写简单的C++测试客户端：
```cpp
// 示例测试代码
void testCreateTieba() {
    // 连接到ChatServer
    // 发送创建贴吧消息
    // 验证响应
}
```

### 3. 数据库监控
```sql
-- 监控脚本
SELECT 
    (SELECT COUNT(*) FROM tieba) as tieba_count,
    (SELECT COUNT(*) FROM tieba_member) as member_count,
    (SELECT COUNT(*) FROM tieba_post) as post_count,
    (SELECT COUNT(*) FROM tieba_reply) as reply_count,
    (SELECT COUNT(*) FROM tieba_like) as like_count;
```

---

## 常见问题排查

### 问题1: 创建贴吧失败
**检查项**:
- ✓ tieba_id表是否初始化
- ✓ 用户是否存在
- ✓ 数据库连接是否正常

### 问题2: 点赞计数不正确
**检查项**:
- ✓ 事务是否正确提交
- ✓ 是否有并发冲突
- ✓ 数据库隔离级别

### 问题3: 权限检查失败
**检查项**:
- ✓ tieba_member表的role字段
- ✓ IsTiebaMember函数返回值
- ✓ 权限判断逻辑

---

## 测试报告模板

```
测试时间: YYYY-MM-DD HH:MM:SS
测试人员: XXX
测试环境: 开发/测试/生产

测试结果统计:
- 总测试用例数: 23
- 通过: XX
- 失败: XX
- 跳过: XX

失败用例详情:
1. 测试用例名称
   - 失败原因: XXX
   - 错误日志: XXX
   - 修复建议: XXX

性能指标:
- 平均响应时间: XXX ms
- 最大并发: XXX
- 内存使用: XXX MB

建议:
1. XXX
2. XXX
```

---

## 附录: 错误码对照表

| 错误码 | 错误名称 | 说明 |
|-------|---------|------|
| 0 | Success | 成功 |
| 1001 | Error_Json | JSON解析失败 |
| 1002 | RPCFailed | RPC调用失败 |
| 1012 | TiebaExist | 贴吧已存在 |
| 1013 | TiebaNotExist | 贴吧不存在 |
| 1014 | TiebaNameInvalid | 贴吧名称无效 |
| 1015 | NotTiebaMember | 不是贴吧成员 |
| 1016 | NotTiebaOwner | 不是吧主 |
| 1017 | NotTiebaAdmin | 不是管理员 |
| 1018 | PostNotExist | 帖子不存在 |
| 1019 | ReplyNotExist | 回复不存在 |
| 1020 | PermissionDenied | 权限不足 |

---

## 附录: 消息ID对照表

| 消息ID | 功能 | 请求/响应 |
|-------|------|----------|
| 1025 | 创建贴吧 | REQ |
| 1026 | 创建贴吧 | RSP |
| 1027 | 搜索贴吧 | REQ |
| 1028 | 搜索贴吧 | RSP |
| 1029 | 加入贴吧 | REQ |
| 1030 | 加入贴吧 | RSP |
| 1031 | 退出贴吧 | REQ |
| 1032 | 退出贴吧 | RSP |
| 1033 | 发帖 | REQ |
| 1034 | 发帖 | RSP |
| 1035 | 回复 | REQ |
| 1036 | 回复 | RSP |
| 1037 | 删除帖子 | REQ |
| 1038 | 删除帖子 | RSP |
| 1039 | 点赞 | REQ |
| 1040 | 点赞 | RSP |
| 1041 | 获取帖子列表 | REQ |
| 1042 | 获取帖子列表 | RSP |
| 1043 | 获取回复列表 | REQ |
| 1044 | 获取回复列表 | RSP |
| 1045 | 置顶 | REQ |
| 1046 | 置顶 | RSP |
| 1047 | 加精 | REQ |
| 1048 | 加精 | RSP |
| 1049 | 获取贴吧信息 | REQ |
| 1050 | 获取贴吧信息 | RSP |
| 1051 | 获取我的贴吧列表 | REQ |
| 1052 | 获取我的贴吧列表 | RSP |
| 1053 | 获取成员列表 | REQ |
| 1054 | 获取成员列表 | RSP |
| 1055 | 删除回复 | REQ |
| 1056 | 删除回复 | RSP |
