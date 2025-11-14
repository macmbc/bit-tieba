# 贴吧功能测试操作指南

## 一、准备工作

### 1. 启动服务
```bash
# 1. 启动StatusServer
cd StatusServer
./statusserver &

# 2. 启动ChatServer（确认端口配置，默认8081）
cd ../ChatServer
./chatserver1 &
```

### 2. 准备数据库
```bash
# 连接MySQL
mysql -h127.0.0.1 -P3308 -uroot -p

# 导入表结构
USE llfc;
source sql/tieba_tables.sql;

# 创建测试用户
INSERT INTO user (uid, name, email, pwd, nick, icon) VALUES 
(1001, 'testuser1', 'test1@test.com', '123456', '测试用户1', 'icon1.png'),
(1002, 'testuser2', 'test2@test.com', '123456', '测试用户2', 'icon2.png');
```

## 二、Python测试脚本（推荐）

### 安装（无需额外依赖，Python 3自带）

### 运行所有测试
```bash
# 基本用法（连接到本地8081端口）
python tieba_test.py

# 指定服务器地址
python tieba_test.py 192.168.1.100 8081

# 在Windows PowerShell中
python.exe tieba_test.py
```

### 测试输出示例
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
  "tieba_desc": "C++编程学习交流社区",
  "tieba_icon": "cpp_icon.png"
}

<<< 接收响应 [ID:1026] <<<
{
  "error": 0,
  "tieba_id": 1,
  "tieba_name": "C++学习吧"
}
✓ 创建成功！贴吧ID: 1
```

### 修改测试用例
编辑 `tieba_test.py` 文件，可以：
- 修改测试数据
- 添加新的测试函数
- 调整测试顺序
- 修改用户ID

示例：
```python
# 在文件末尾添加自定义测试
def my_custom_test(client):
    data = {
        "uid": 1003,
        "tieba_name": "Python学习吧",
        "tieba_desc": "Python交流社区",
        "tieba_icon": ""
    }
    client.send_message(1025, data)
    response = client.receive_response()
    print(f"结果: {response}")

# 在 run_all_tests() 中调用
my_custom_test(client)
```

## 三、C++测试客户端

### 编译
```bash
# Linux/macOS
chmod +x compile_test_client.sh
./compile_test_client.sh

# 或手动编译
g++ -std=c++17 test_client.cpp -o test_client \
    -lboost_system -ljsoncpp -lpthread
```

### 运行
```bash
# 默认连接 127.0.0.1:8081
./test_client

# 指定服务器地址
./test_client 192.168.1.100 8081
```

## 四、使用 netcat 手动测试（高级）

### 1. 计算消息格式

消息结构：
```
[4字节总长度][2字节消息ID][2字节数据长度][JSON数据]
```

### 2. 使用Python辅助生成消息
```python
import struct
import json

def build_message(msg_id, data):
    json_data = json.dumps(data).encode('utf-8')
    data_len = len(json_data)
    total_len = 4 + 2 + 2 + data_len
    
    message = struct.pack('!I', total_len)
    message += struct.pack('!H', msg_id)
    message += struct.pack('!H', data_len)
    message += json_data
    
    return message

# 创建贴吧消息
data = {
    "uid": 1001,
    "tieba_name": "测试贴吧",
    "tieba_desc": "描述",
    "tieba_icon": ""
}

msg = build_message(1025, data)
with open('msg.bin', 'wb') as f:
    f.write(msg)

print("消息已保存到 msg.bin")
```

### 3. 发送消息
```bash
# 使用netcat发送
nc 127.0.0.1 8081 < msg.bin
```

## 五、使用数据库直接验证

### 查询贴吧信息
```sql
-- 查看所有贴吧
SELECT * FROM tieba;

-- 查看贴吧成员
SELECT tm.*, u.name 
FROM tieba_member tm 
LEFT JOIN user u ON tm.uid = u.uid 
WHERE tieba_id = 1;

-- 查看帖子列表
SELECT p.*, u.name 
FROM tieba_post p 
LEFT JOIN user u ON p.uid = u.uid 
WHERE tieba_id = 1;

-- 查看回复列表
SELECT r.*, u.name 
FROM tieba_reply r 
LEFT JOIN user u ON r.uid = u.uid 
WHERE post_id = 1;

-- 查看点赞记录
SELECT * FROM tieba_like WHERE target_type = 1 AND target_id = 1;
```

### 重置测试数据
```sql
-- 清空所有贴吧数据
TRUNCATE TABLE tieba_like;
TRUNCATE TABLE tieba_reply;
TRUNCATE TABLE tieba_post;
TRUNCATE TABLE tieba_member;
TRUNCATE TABLE tieba;

-- 重置自增ID
UPDATE tieba_id SET id = 0;
UPDATE post_id SET id = 0;
UPDATE reply_id SET id = 0;
```

## 六、常见测试场景

### 场景1：完整的创建-加入-发帖流程
```python
# 1. 用户A创建贴吧
client.send_message(1025, {"uid": 1001, "tieba_name": "测试吧", ...})

# 2. 用户B加入贴吧
client.send_message(1029, {"uid": 1002, "tieba_id": 1})

# 3. 用户B发帖
client.send_message(1033, {"uid": 1002, "tieba_id": 1, "title": "...", "content": "..."})

# 4. 用户A回复
client.send_message(1035, {"uid": 1001, "post_id": 1, "content": "..."})
```

### 场景2：测试权限控制
```python
# 1. 普通成员尝试置顶（应该失败）
client.send_message(1045, {"uid": 1002, "post_id": 1, "tieba_id": 1, "is_top": True})
# 期望返回: {"error": 1020}  # PermissionDenied

# 2. 吧主置顶（应该成功）
client.send_message(1045, {"uid": 1001, "post_id": 1, "tieba_id": 1, "is_top": True})
# 期望返回: {"error": 0, "post_id": 1, "is_top": true}
```

### 场景3：测试边界条件
```python
# 1. 超长贴吧名称
client.send_message(1025, {"uid": 1001, "tieba_name": "x"*100, ...})
# 期望返回: {"error": 1014}  # TiebaNameInvalid

# 2. 非成员发帖
client.send_message(1033, {"uid": 1003, "tieba_id": 1, ...})
# 期望返回: {"error": 1015}  # NotTiebaMember
```

## 七、查看服务器日志

### ChatServer日志
```bash
# 实时查看日志
tail -f ChatServer/chatserver.log

# 搜索错误
grep "error" ChatServer/chatserver.log
grep "TiebaDao" ChatServer/chatserver.log
```

### 监控消息处理
ChatServer会在控制台输出：
```
recv_msg id  is 1025
CreateTieba: uid=1001, name=C++学习吧
```

## 八、性能测试

### 并发测试脚本
```python
import threading
import time

def concurrent_test(user_id, tieba_id):
    client = TiebaTestClient()
    client.connect()
    
    # 同时发帖
    data = {
        "uid": user_id,
        "tieba_id": tieba_id,
        "title": f"并发测试帖子{user_id}",
        "content": "测试内容"
    }
    client.send_message(1033, data)
    response = client.receive_response()
    print(f"用户{user_id}发帖结果: {response}")
    
    client.close()

# 创建10个线程同时发帖
threads = []
for i in range(10):
    t = threading.Thread(target=concurrent_test, args=(1000+i, 1))
    threads.append(t)
    t.start()

for t in threads:
    t.join()

print("并发测试完成")
```

## 九、故障排查

### 问题1：连接失败
```bash
# 检查服务是否运行
ps aux | grep chatserver

# 检查端口是否监听
netstat -an | grep 8081
# 或
lsof -i :8081

# 检查防火墙
sudo iptables -L
```

### 问题2：消息无响应
- 检查消息ID是否正确
- 检查JSON格式是否正确
- 查看服务器日志
- 验证用户是否存在

### 问题3：权限错误
```sql
-- 检查用户在贴吧中的角色
SELECT * FROM tieba_member WHERE uid = 1001 AND tieba_id = 1;
```

## 十、测试检查清单

测试前确认：
- [ ] MySQL服务运行中
- [ ] Redis服务运行中
- [ ] StatusServer已启动
- [ ] ChatServer已启动
- [ ] 数据库表已创建
- [ ] 测试用户已创建
- [ ] 端口未被占用

测试后验证：
- [ ] 检查数据库数据是否正确
- [ ] 检查计数字段是否准确（member_count, post_count等）
- [ ] 检查日志无错误信息
- [ ] 清理测试数据（如需要）
