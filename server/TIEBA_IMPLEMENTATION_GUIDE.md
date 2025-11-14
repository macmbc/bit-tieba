# 贴吧功能实现指南

本文档说明如何完成贴吧管理功能的剩余实现。

## 已完成的工作

1. ✅ 数据库表结构 (server/sql/tieba_tables.sql)
2. ✅ Protobuf消息定义 (所有服务的message.proto)
3. ✅ 消息ID和错误码定义 (const.h)
4. ✅ 数据结构定义 (data.h)
5. ✅ TiebaDao基础框架 (TiebaDao.h, TiebaDao.cpp部分实现)

## 待完成的工作

### 1. 完成TiebaDao.cpp的剩余实现

需要实现以下函数：
- `CreatePost` - 发帖
- `CreateReply` - 回复
- `GetPostList` - 获取帖子列表
- `GetReplyList` - 获取回复列表
- `GetPostInfo` - 获取帖子信息
- `GetReplyInfo` - 获取回复信息
- `DeletePost` - 删除帖子
- `DeleteReply` - 删除回复
- `Like` - 点赞/取消点赞
- `IsLiked` - 检查是否点赞
- `SetTop` - 置顶
- `SetEssence` - 加精
- `GetMyTiebaList` - 获取我的贴吧列表
- `GetTiebaMemberList` - 获取贴吧成员列表

### 2. 在MysqlMgr中添加贴吧相关方法

参考 `MysqlMgr.h` 和 `MysqlMgr.cpp`，添加贴吧相关的包装方法。

### 3. 在GateServer中添加HTTP接口

在 `LogicSystem.cpp` 中添加以下HTTP接口：
- `POST /tieba/create` - 创建贴吧
- `POST /tieba/search` - 搜索贴吧
- `GET /tieba/info` - 获取贴吧信息
- `GET /tieba/list` - 获取贴吧列表

### 4. 在ChatServer中添加TCP消息处理

在 `LogicSystem.cpp` 的 `RegisterCallBacks` 中注册贴吧相关消息处理函数，并实现：
- `CreateTiebaHandler` - 创建贴吧
- `SearchTiebaHandler` - 搜索贴吧
- `JoinTiebaHandler` - 加入贴吧
- `LeaveTiebaHandler` - 退出贴吧
- `CreatePostHandler` - 发帖
- `CreateReplyHandler` - 回复
- `GetPostListHandler` - 获取帖子列表
- `GetReplyListHandler` - 获取回复列表
- `DeletePostHandler` - 删除帖子
- `DeleteReplyHandler` - 删除回复
- `LikeHandler` - 点赞
- `SetTopHandler` - 置顶
- `SetEssenceHandler` - 加精
- `GetTiebaInfoHandler` - 获取贴吧信息
- `GetMyTiebaListHandler` - 获取我的贴吧列表
- `GetTiebaMemberListHandler` - 获取贴吧成员列表

### 5. 更新CMakeLists.txt

在 `ChatServer/CMakeLists.txt` 和 `ChatServer2/CMakeLists.txt` 中添加 `TiebaDao.cpp`。

### 6. 重新生成protobuf文件

运行以下命令重新生成protobuf文件：
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

### 7. 导入数据库表

执行SQL文件创建数据库表：
```bash
mysql -h127.0.0.1 -P3308 -uroot -p123456. llfc < server/sql/tieba_tables.sql
```

## 实现示例

### CreatePost实现示例

```cpp
int TiebaDao::CreatePost(int uid, int tieba_id, const std::string& title, const std::string& content) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return -1;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
	});

	try {
		con->_con->setAutoCommit(false);

		// 检查是否为贴吧成员
		std::unique_ptr<sql::PreparedStatement> pstmt_check(con->_con->prepareStatement(
			"SELECT 1 FROM tieba_member WHERE tieba_id = ? AND uid = ?"));
		pstmt_check->setInt(1, tieba_id);
		pstmt_check->setInt(2, uid);
		std::unique_ptr<sql::ResultSet> res_check(pstmt_check->executeQuery());
		if (!res_check->next()) {
			con->_con->rollback();
			return -1;  // 不是成员
		}

		// 获取新的帖子ID
		std::unique_ptr<sql::PreparedStatement> pstmt_id(con->_con->prepareStatement("UPDATE post_id SET id = id + 1"));
		pstmt_id->executeUpdate();

		std::unique_ptr<sql::PreparedStatement> pstmt_get_id(con->_con->prepareStatement("SELECT id FROM post_id"));
		std::unique_ptr<sql::ResultSet> res_id(pstmt_get_id->executeQuery());
		int post_id = 0;
		if (res_id->next()) {
			post_id = res_id->getInt("id");
		} else {
			con->_con->rollback();
			return -1;
		}

		// 插入帖子
		std::unique_ptr<sql::PreparedStatement> pstmt_insert(con->_con->prepareStatement(
			"INSERT INTO tieba_post (post_id, tieba_id, uid, title, content, reply_count, like_count, is_top, is_essence) "
			"VALUES (?, ?, ?, ?, ?, 0, 0, 0, 0)"));
		pstmt_insert->setInt(1, post_id);
		pstmt_insert->setInt(2, tieba_id);
		pstmt_insert->setInt(3, uid);
		pstmt_insert->setString(4, title);
		pstmt_insert->setString(5, content);
		pstmt_insert->executeUpdate();

		// 更新贴吧帖子数量
		std::unique_ptr<sql::PreparedStatement> pstmt_update(con->_con->prepareStatement(
			"UPDATE tieba SET post_count = post_count + 1 WHERE tieba_id = ?"));
		pstmt_update->setInt(1, tieba_id);
		pstmt_update->executeUpdate();

		con->_con->commit();
		return post_id;
	}
	catch (sql::SQLException& e) {
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException in CreatePost: " << e.what() << std::endl;
		return -1;
	}
}
```

### Like实现示例

```cpp
bool TiebaDao::Like(int uid, int target_type, int target_id, bool is_like, int& like_count) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
	});

	try {
		con->_con->setAutoCommit(false);

		// 检查是否已点赞
		std::unique_ptr<sql::PreparedStatement> pstmt_check(con->_con->prepareStatement(
			"SELECT 1 FROM tieba_like WHERE uid = ? AND target_type = ? AND target_id = ?"));
		pstmt_check->setInt(1, uid);
		pstmt_check->setInt(2, target_type);
		pstmt_check->setInt(3, target_id);
		std::unique_ptr<sql::ResultSet> res_check(pstmt_check->executeQuery());
		bool already_liked = res_check->next();

		if (is_like && !already_liked) {
			// 添加点赞
			std::unique_ptr<sql::PreparedStatement> pstmt_insert(con->_con->prepareStatement(
				"INSERT INTO tieba_like (uid, target_type, target_id) VALUES (?, ?, ?)"));
			pstmt_insert->setInt(1, uid);
			pstmt_insert->setInt(2, target_type);
			pstmt_insert->setInt(3, target_id);
			pstmt_insert->executeUpdate();

			// 更新点赞数
			std::string table_name = (target_type == 1) ? "tieba_post" : "tieba_reply";
			std::string id_name = (target_type == 1) ? "post_id" : "reply_id";
			std::string update_sql = "UPDATE " + table_name + " SET like_count = like_count + 1 WHERE " + id_name + " = ?";
			std::unique_ptr<sql::PreparedStatement> pstmt_update(con->_con->prepareStatement(update_sql));
			pstmt_update->setInt(1, target_id);
			pstmt_update->executeUpdate();
		} else if (!is_like && already_liked) {
			// 取消点赞
			std::unique_ptr<sql::PreparedStatement> pstmt_delete(con->_con->prepareStatement(
				"DELETE FROM tieba_like WHERE uid = ? AND target_type = ? AND target_id = ?"));
			pstmt_delete->setInt(1, uid);
			pstmt_delete->setInt(2, target_type);
			pstmt_delete->setInt(3, target_id);
			pstmt_delete->executeUpdate();

			// 更新点赞数
			std::string table_name = (target_type == 1) ? "tieba_post" : "tieba_reply";
			std::string id_name = (target_type == 1) ? "post_id" : "reply_id";
			std::string update_sql = "UPDATE " + table_name + " SET like_count = like_count - 1 WHERE " + id_name + " = ?";
			std::unique_ptr<sql::PreparedStatement> pstmt_update(con->_con->prepareStatement(update_sql));
			pstmt_update->setInt(1, target_id);
			pstmt_update->executeUpdate();
		}

		// 获取当前点赞数
		std::string table_name = (target_type == 1) ? "tieba_post" : "tieba_reply";
		std::string id_name = (target_type == 1) ? "post_id" : "reply_id";
		std::string select_sql = "SELECT like_count FROM " + table_name + " WHERE " + id_name + " = ?";
		std::unique_ptr<sql::PreparedStatement> pstmt_get(con->_con->prepareStatement(select_sql));
		pstmt_get->setInt(1, target_id);
		std::unique_ptr<sql::ResultSet> res_get(pstmt_get->executeQuery());
		if (res_get->next()) {
			like_count = res_get->getInt("like_count");
		}

		con->_con->commit();
		return true;
	}
	catch (sql::SQLException& e) {
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException in Like: " << e.what() << std::endl;
		return false;
	}
}
```

## 注意事项

1. 所有数据库操作都需要使用事务，确保数据一致性
2. 权限检查：只有贴吧成员才能发帖，只有吧主/小吧主才能置顶/加精/删除帖子
3. 吧主不能退出贴吧
4. 删除帖子时需要同时删除所有回复和点赞记录
5. 时间戳格式：使用MySQL的TIMESTAMP类型，在查询时转换为字符串
6. 分页查询：使用LIMIT和OFFSET实现分页
7. 缓存：可以考虑使用Redis缓存热门贴吧和帖子列表

## 测试建议

1. 单元测试：测试每个数据库操作函数
2. 集成测试：测试完整的业务流程
3. 压力测试：测试并发情况下的性能
4. 边界测试：测试边界条件和异常情况

## 后续优化

1. 添加Redis缓存层
2. 实现消息推送（新帖子、新回复）
3. 添加图片上传功能
4. 实现帖子搜索功能
5. 添加帖子分类功能
6. 实现帖子推荐算法



