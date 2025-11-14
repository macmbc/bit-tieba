# 贴吧功能测试快速入门

## 最简单的测试方法（推荐）

### 1. 运行完整测试套件
```bash
# Windows PowerShell
python tieba_test.py

# 连接到远程服务器
python tieba_test.py 192.168.1.100 8081
```

输出示例：
```
==================================================
贴吧功能测试客户端
==================================================
✓ 已连接到服务器 127.0.0.1:8081

==================================================
测试1: 创建贴吧
==================================================
>>> 发送消息 [ID:1025] <<<
{
  "uid": 1001,
  "tieba_name": "C++学习吧",
  ...
}

<<< 接收响应 [ID:1026] <<<
{
  "error": 0,
  "tieba_id": 1,
  ...
}
✓ 创建成功！贴吧ID: 1
```

### 2. 快速测试单个功能
```bash
# 创建贴吧
python quick_test.py create_tieba

# 搜索贴吧（关键词"测试"）
python quick_test.py search_tieba 测试

# 加入贴吧（用户1002加入贴吧1）
python quick_test.py join_tieba 1002 1

# 发帖（用户1002在贴吧1发帖）
python quick_test.py create_post 1002 1

# 回复帖子（用户1001回复帖子1）
python quick_test.py create_reply 1001 1

# 点赞帖子（用户1001点赞帖子1）
python quick_test.py like 1001 1 1

# 获取帖子列表
python quick_test.py get_post_list 1002 1

# 获取我的贴吧列表
python quick_test.py get_my_tiebas 1002
```

## 测试前准备

### 1. 启动服务
```powershell
# StatusServer
cd StatusServer
.\statusserver.exe

# ChatServer（新建终端）
cd ChatServer
.\chatserver1.exe
```

### 2. 创建测试用户（MySQL）
```sql
USE llfc;

INSERT INTO user (uid, name, email, pwd, nick, icon) VALUES 
(1001, 'testuser1', 'test1@test.com', '123456', '测试用户1', 'icon1.png'),
(1002, 'testuser2', 'test2@test.com', '123456', '测试用户2', 'icon2.png');
```

## 常用测试命令速查

| 命令 | 说明 | 示例 |
|------|------|------|
| `create_tieba [uid]` | 创建贴吧 | `python quick_test.py create_tieba 1001` |
| `search_tieba [keyword]` | 搜索贴吧 | `python quick_test.py search_tieba C++` |
| `join_tieba [uid] [tieba_id]` | 加入贴吧 | `python quick_test.py join_tieba 1002 1` |
| `create_post [uid] [tieba_id]` | 发帖 | `python quick_test.py create_post 1002 1` |
| `create_reply [uid] [post_id]` | 回复 | `python quick_test.py create_reply 1001 1` |
| `like [uid] [type] [id]` | 点赞 | `python quick_test.py like 1001 1 1` |
| `set_top [uid] [post_id] [tieba_id]` | 置顶 | `python quick_test.py set_top 1001 1 1` |
| `get_post_list [uid] [tieba_id]` | 帖子列表 | `python quick_test.py get_post_list 1002 1` |
| `get_my_tiebas [uid]` | 我的贴吧 | `python quick_test.py get_my_tiebas 1002` |

## 验证测试结果

### 方法1: 查看终端输出
脚本会自动显示成功/失败状态和返回数据

### 方法2: 查询数据库
```sql
-- 查看贴吧
SELECT * FROM tieba;

-- 查看帖子
SELECT * FROM tieba_post WHERE tieba_id = 1;

-- 查看成员
SELECT * FROM tieba_member WHERE tieba_id = 1;

-- 查看点赞
SELECT * FROM tieba_like WHERE target_type = 1 AND target_id = 1;
```

### 方法3: 查看服务器日志
```powershell
# 实时查看日志
Get-Content ChatServer\chatserver.log -Wait -Tail 10

# 搜索错误
Select-String -Path ChatServer\chatserver.log -Pattern "error"
```

## 典型测试流程

### 完整业务流程测试
```powershell
# 1. 用户A创建贴吧
python quick_test.py create_tieba 1001

# 2. 搜索贴吧（验证创建成功）
python quick_test.py search_tieba 测试

# 3. 用户B加入贴吧
python quick_test.py join_tieba 1002 1

# 4. 用户B发帖
python quick_test.py create_post 1002 1

# 5. 用户A回复
python quick_test.py create_reply 1001 1

# 6. 用户A点赞
python quick_test.py like 1001 1 1

# 7. 用户A置顶帖子（作为吧主）
python quick_test.py set_top 1001 1 1

# 8. 查看帖子列表（验证置顶）
python quick_test.py get_post_list 1002 1

# 9. 查看我的贴吧
python quick_test.py get_my_tiebas 1002
```

## 清理测试数据

```sql
-- 清空所有贴吧数据
TRUNCATE TABLE tieba_like;
TRUNCATE TABLE tieba_reply;
TRUNCATE TABLE tieba_post;
TRUNCATE TABLE tieba_member;
TRUNCATE TABLE tieba;

-- 重置ID
UPDATE tieba_id SET id = 0;
UPDATE post_id SET id = 0;
UPDATE reply_id SET id = 0;
```

## 故障排查

### 连接失败
```powershell
# 检查服务是否运行
Get-Process | Where-Object {$_.ProcessName -like "*chatserver*"}

# 检查端口
netstat -an | Select-String "8081"
```

### 权限错误
检查用户角色：
```sql
SELECT * FROM tieba_member WHERE uid = 1001 AND tieba_id = 1;
```

### 数据不一致
查看计数字段：
```sql
SELECT tieba_id, tieba_name, member_count, post_count 
FROM tieba WHERE tieba_id = 1;
```

## 完整文档

- **详细测试指南**: `TIEBA_TEST_GUIDE.md` - 包含23个详细测试用例
- **操作说明**: `TEST_INSTRUCTIONS.md` - 详细的测试方法和工具说明
- **实现指南**: `TIEBA_IMPLEMENTATION_GUIDE.md` - 功能说明和接口文档

## 消息ID速查

| 功能 | 请求ID | 响应ID |
|------|--------|--------|
| 创建贴吧 | 1025 | 1026 |
| 搜索贴吧 | 1027 | 1028 |
| 加入贴吧 | 1029 | 1030 |
| 退出贴吧 | 1031 | 1032 |
| 发帖 | 1033 | 1034 |
| 回复 | 1035 | 1036 |
| 删除帖子 | 1037 | 1038 |
| 点赞 | 1039 | 1040 |
| 获取帖子列表 | 1041 | 1042 |
| 获取回复列表 | 1043 | 1044 |
| 置顶 | 1045 | 1046 |
| 加精 | 1047 | 1048 |
| 获取贴吧信息 | 1049 | 1050 |
| 获取我的贴吧 | 1051 | 1052 |
| 获取成员列表 | 1053 | 1054 |
| 删除回复 | 1055 | 1056 |
