#include "TiebaDao.h"
#include "ConfigMgr.h"
#include "MysqlDao.h"
#include <sstream>
#include <iomanip>

TiebaDao::TiebaDao() {
	auto & cfg = ConfigMgr::Inst();
	const auto& host = cfg["Mysql"]["Host"];
	const auto& port = cfg["Mysql"]["Port"];
	const auto& pwd = cfg["Mysql"]["Passwd"];
	const auto& schema = cfg["Mysql"]["Schema"];
	const auto& user = cfg["Mysql"]["User"];
	pool_.reset(new MySqlPool(host+":"+port, user, pwd, schema, 5));
}

TiebaDao::~TiebaDao() {
	if (pool_) {
		pool_->Close();
	}
}

// 创建贴吧
int TiebaDao::CreateTieba(int uid, const std::string& tieba_name, const std::string& tieba_desc, const std::string& tieba_icon) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return -1;
	}

	Defer defer([this, &con]() {
		if (con) {
			pool_->returnConnection(std::move(con));
		}
	});

	try {
		con->_con->setAutoCommit(false);

		// 检查贴吧名称是否已存在
		std::unique_ptr<sql::PreparedStatement> pstmt_check(con->_con->prepareStatement("SELECT tieba_id FROM tieba WHERE tieba_name = ?"));
		pstmt_check->setString(1, tieba_name);
		std::unique_ptr<sql::ResultSet> res_check(pstmt_check->executeQuery());
		if (res_check->next()) {
			con->_con->rollback();
			return 0;  // 贴吧已存在
		}

		// 获取新的贴吧ID
		std::unique_ptr<sql::PreparedStatement> pstmt_id(con->_con->prepareStatement("UPDATE tieba_id SET id = id + 1"));
		pstmt_id->executeUpdate();

		std::unique_ptr<sql::PreparedStatement> pstmt_get_id(con->_con->prepareStatement("SELECT id FROM tieba_id"));
		std::unique_ptr<sql::ResultSet> res_id(pstmt_get_id->executeQuery());
		int tieba_id = 0;
		if (res_id->next()) {
			tieba_id = res_id->getInt("id");
		} else {
			con->_con->rollback();
			return -1;
		}

		// 插入贴吧信息
		std::unique_ptr<sql::PreparedStatement> pstmt_insert(con->_con->prepareStatement(
			"INSERT INTO tieba (tieba_id, tieba_name, tieba_desc, tieba_icon, owner_uid, member_count, post_count) "
			"VALUES (?, ?, ?, ?, ?, 1, 0)"));
		pstmt_insert->setInt(1, tieba_id);
		pstmt_insert->setString(2, tieba_name);
		pstmt_insert->setString(3, tieba_desc);
		pstmt_insert->setString(4, tieba_icon);
		pstmt_insert->setInt(5, uid);
		pstmt_insert->executeUpdate();

		// 插入吧主成员记录
		std::unique_ptr<sql::PreparedStatement> pstmt_member(con->_con->prepareStatement(
			"INSERT INTO tieba_member (tieba_id, uid, role) VALUES (?, ?, 2)"));
		pstmt_member->setInt(1, tieba_id);
		pstmt_member->setInt(2, uid);
		pstmt_member->executeUpdate();

		con->_con->commit();
		return tieba_id;
	}
	catch (sql::SQLException& e) {
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException in CreateTieba: " << e.what() << std::endl;
		return -1;
	}
}

// 搜索贴吧
bool TiebaDao::SearchTieba(const std::string& keyword, int offset, int limit, std::vector<std::shared_ptr<TiebaInfo>>& tieba_list, int& total) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		if (con) {
			pool_->returnConnection(std::move(con));
		}
	});

	try {
		// 获取总数
		std::unique_ptr<sql::PreparedStatement> pstmt_count(con->_con->prepareStatement(
			"SELECT COUNT(*) as total FROM tieba WHERE tieba_name LIKE ?"));
		std::string search_key = "%" + keyword + "%";
		pstmt_count->setString(1, search_key);
		std::unique_ptr<sql::ResultSet> res_count(pstmt_count->executeQuery());
		if (res_count->next()) {
			total = res_count->getInt("total");
		}

		// 获取贴吧列表
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT tieba_id, tieba_name, tieba_desc, tieba_icon, owner_uid, member_count, post_count, create_time "
			"FROM tieba WHERE tieba_name LIKE ? ORDER BY create_time DESC LIMIT ? OFFSET ?"));
		pstmt->setString(1, search_key);
		pstmt->setInt(2, limit);
		pstmt->setInt(3, offset);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		while (res->next()) {
			auto info = std::make_shared<TiebaInfo>();
			info->tieba_id = res->getInt("tieba_id");
			info->tieba_name = res->getString("tieba_name");
			info->tieba_desc = res->getString("tieba_desc");
			info->tieba_icon = res->getString("tieba_icon");
			info->owner_uid = res->getInt("owner_uid");
			info->member_count = res->getInt("member_count");
			info->post_count = res->getInt("post_count");
			
			// 转换时间戳
			// std::unique_ptr<sql::SQLString> time_str = res->getString("create_time");
			// if (time_str) {
			// 	info->create_time = time_str->asStdString();
			// }
			info->create_time = res->getString("create_time").asStdString();
			tieba_list.push_back(info);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException in SearchTieba: " << e.what() << std::endl;
		return false;
	}
}

// 获取贴吧信息
bool TiebaDao::GetTiebaInfo(int tieba_id, std::shared_ptr<TiebaInfo>& tieba_info) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		if (con) {
			pool_->returnConnection(std::move(con));
		}
	});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT tieba_id, tieba_name, tieba_desc, tieba_icon, owner_uid, member_count, post_count, create_time "
			"FROM tieba WHERE tieba_id = ?"));
		pstmt->setInt(1, tieba_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		if (res->next()) {
			tieba_info = std::make_shared<TiebaInfo>();
			tieba_info->tieba_id = res->getInt("tieba_id");
			tieba_info->tieba_name = res->getString("tieba_name");
			tieba_info->tieba_desc = res->getString("tieba_desc");
			tieba_info->tieba_icon = res->getString("tieba_icon");
			tieba_info->owner_uid = res->getInt("owner_uid");
			tieba_info->member_count = res->getInt("member_count");
			tieba_info->post_count = res->getInt("post_count");
			// std::unique_ptr<sql::SQLString> time_str = res->getString("create_time");
			// if (time_str) {
			// 	tieba_info->create_time = time_str->asStdString();
			// }
			tieba_info->create_time = res->getString("create_time").asStdString();
			return true;
		}
		return false;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException in GetTiebaInfo: " << e.what() << std::endl;
		return false;
	}
}

bool TiebaDao::GetTiebaInfo(const std::string& tieba_name, std::shared_ptr<TiebaInfo>& tieba_info) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		if (con) {
			pool_->returnConnection(std::move(con));
		}
	});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT tieba_id, tieba_name, tieba_desc, tieba_icon, owner_uid, member_count, post_count, create_time "
			"FROM tieba WHERE tieba_name = ?"));
		pstmt->setString(1, tieba_name);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		if (res->next()) {
			tieba_info = std::make_shared<TiebaInfo>();
			tieba_info->tieba_id = res->getInt("tieba_id");
			tieba_info->tieba_name = res->getString("tieba_name");
			tieba_info->tieba_desc = res->getString("tieba_desc");
			tieba_info->tieba_icon = res->getString("tieba_icon");
			tieba_info->owner_uid = res->getInt("owner_uid");
			tieba_info->member_count = res->getInt("member_count");
			tieba_info->post_count = res->getInt("post_count");
			// std::unique_ptr<sql::SQLString> time_str = res->getString("create_time");
			// if (time_str) {
			// 	tieba_info->create_time = time_str->asStdString();
			// }
			tieba_info->create_time = res->getString("create_time").asStdString();
			return true;
		}
		return false;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException in GetTiebaInfo: " << e.what() << std::endl;
		return false;
	}
}

// 检查贴吧是否存在
bool TiebaDao::TiebaExists(const std::string& tieba_name) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		if (con) {
			pool_->returnConnection(std::move(con));
		}
	});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT 1 FROM tieba WHERE tieba_name = ?"));
		pstmt->setString(1, tieba_name);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		return res->next();
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException in TiebaExists: " << e.what() << std::endl;
		return false;
	}
}

bool TiebaDao::TiebaExists(int tieba_id) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		if (con) {
			pool_->returnConnection(std::move(con));
		}
	});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT 1 FROM tieba WHERE tieba_id = ?"));
		pstmt->setInt(1, tieba_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		return res->next();
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException in TiebaExists: " << e.what() << std::endl;
		return false;
	}
}

// 加入贴吧
bool TiebaDao::JoinTieba(int uid, int tieba_id) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		if (con) {
			pool_->returnConnection(std::move(con));
		}
	});

	try {
		con->_con->setAutoCommit(false);

		// 检查是否已经是成员
		std::unique_ptr<sql::PreparedStatement> pstmt_check(con->_con->prepareStatement(
			"SELECT 1 FROM tieba_member WHERE tieba_id = ? AND uid = ?"));
		pstmt_check->setInt(1, tieba_id);
		pstmt_check->setInt(2, uid);
		std::unique_ptr<sql::ResultSet> res_check(pstmt_check->executeQuery());
		if (res_check->next()) {
			con->_con->rollback();
			return false;  // 已经是成员
		}

		// 插入成员记录
		std::unique_ptr<sql::PreparedStatement> pstmt_insert(con->_con->prepareStatement(
			"INSERT INTO tieba_member (tieba_id, uid, role) VALUES (?, ?, 0)"));
		pstmt_insert->setInt(1, tieba_id);
		pstmt_insert->setInt(2, uid);
		pstmt_insert->executeUpdate();

		// 更新成员数量
		std::unique_ptr<sql::PreparedStatement> pstmt_update(con->_con->prepareStatement(
			"UPDATE tieba SET member_count = member_count + 1 WHERE tieba_id = ?"));
		pstmt_update->setInt(1, tieba_id);
		pstmt_update->executeUpdate();

		con->_con->commit();
		return true;
	}
	catch (sql::SQLException& e) {
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException in JoinTieba: " << e.what() << std::endl;
		return false;
	}
}

// 退出贴吧
bool TiebaDao::LeaveTieba(int uid, int tieba_id) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		if (con) {
			pool_->returnConnection(std::move(con));
		}
	});

	try {
		con->_con->setAutoCommit(false);

		// 检查是否为吧主
		std::unique_ptr<sql::PreparedStatement> pstmt_check(con->_con->prepareStatement(
			"SELECT role FROM tieba_member WHERE tieba_id = ? AND uid = ?"));
		pstmt_check->setInt(1, tieba_id);
		pstmt_check->setInt(2, uid);
		std::unique_ptr<sql::ResultSet> res_check(pstmt_check->executeQuery());
		if (res_check->next()) {
			int role = res_check->getInt("role");
			if (role == 2) {  // 吧主不能退出
				con->_con->rollback();
				return false;
			}
		} else {
			con->_con->rollback();
			return false;  // 不是成员
		}

		// 删除成员记录
		std::unique_ptr<sql::PreparedStatement> pstmt_delete(con->_con->prepareStatement(
			"DELETE FROM tieba_member WHERE tieba_id = ? AND uid = ?"));
		pstmt_delete->setInt(1, tieba_id);
		pstmt_delete->setInt(2, uid);
		pstmt_delete->executeUpdate();

		// 更新成员数量
		std::unique_ptr<sql::PreparedStatement> pstmt_update(con->_con->prepareStatement(
			"UPDATE tieba SET member_count = member_count - 1 WHERE tieba_id = ?"));
		pstmt_update->setInt(1, tieba_id);
		pstmt_update->executeUpdate();

		con->_con->commit();
		return true;
	}
	catch (sql::SQLException& e) {
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException in LeaveTieba: " << e.what() << std::endl;
		return false;
	}
}

// 检查是否为贴吧成员
bool TiebaDao::IsTiebaMember(int uid, int tieba_id, int& role) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		if (con) {
			pool_->returnConnection(std::move(con));
		}
	});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT role FROM tieba_member WHERE tieba_id = ? AND uid = ?"));
		pstmt->setInt(1, tieba_id);
		pstmt->setInt(2, uid);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		if (res->next()) {
			role = res->getInt("role");
			return true;
		}
		role = 0;  // 非成员
		return false;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException in IsTiebaMember: " << e.what() << std::endl;
		return false;
	}
}

// 发帖
int TiebaDao::CreatePost(int uid, int tieba_id, const std::string& title, const std::string& content) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return -1;
	}

	Defer defer([this, &con]() {
		if (con) {
			pool_->returnConnection(std::move(con));
		}
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

// 回复
int TiebaDao::CreateReply(int uid, int post_id, const std::string& content, int reply_to_uid, int reply_to_reply_id) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return -1;
	}

	Defer defer([this, &con]() {
		if (con) {
			pool_->returnConnection(std::move(con));
		}
	});

	try {
		con->_con->setAutoCommit(false);

		// 检查帖子是否存在
		std::unique_ptr<sql::PreparedStatement> pstmt_check(con->_con->prepareStatement(
			"SELECT tieba_id FROM tieba_post WHERE post_id = ?"));
		pstmt_check->setInt(1, post_id);
		std::unique_ptr<sql::ResultSet> res_check(pstmt_check->executeQuery());
		if (!res_check->next()) {
			con->_con->rollback();
			return -1;  // 帖子不存在
		}

		// 获取新的回复ID
		std::unique_ptr<sql::PreparedStatement> pstmt_id(con->_con->prepareStatement("UPDATE reply_id SET id = id + 1"));
		pstmt_id->executeUpdate();

		std::unique_ptr<sql::PreparedStatement> pstmt_get_id(con->_con->prepareStatement("SELECT id FROM reply_id"));
		std::unique_ptr<sql::ResultSet> res_id(pstmt_get_id->executeQuery());
		int reply_id = 0;
		if (res_id->next()) {
			reply_id = res_id->getInt("id");
		} else {
			con->_con->rollback();
			return -1;
		}

		// 插入回复
		std::unique_ptr<sql::PreparedStatement> pstmt_insert(con->_con->prepareStatement(
			"INSERT INTO tieba_reply (reply_id, post_id, uid, content, reply_to_uid, reply_to_reply_id, like_count) "
			"VALUES (?, ?, ?, ?, ?, ?, 0)"));
		pstmt_insert->setInt(1, reply_id);
		pstmt_insert->setInt(2, post_id);
		pstmt_insert->setInt(3, uid);
		pstmt_insert->setString(4, content);
		pstmt_insert->setInt(5, reply_to_uid);
		pstmt_insert->setInt(6, reply_to_reply_id);
		pstmt_insert->executeUpdate();

		// 更新帖子回复数量
		std::unique_ptr<sql::PreparedStatement> pstmt_update(con->_con->prepareStatement(
			"UPDATE tieba_post SET reply_count = reply_count + 1 WHERE post_id = ?"));
		pstmt_update->setInt(1, post_id);
		pstmt_update->executeUpdate();

		con->_con->commit();
		return reply_id;
	}
	catch (sql::SQLException& e) {
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException in CreateReply: " << e.what() << std::endl;
		return -1;
	}
}

// 获取帖子列表
bool TiebaDao::GetPostList(int uid, int tieba_id, int offset, int limit, int sort_type, std::vector<std::shared_ptr<PostInfo>>& post_list, int& total) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		if (con) {
			pool_->returnConnection(std::move(con));
		}
	});

	try {
		// 获取总数
		std::unique_ptr<sql::PreparedStatement> pstmt_count(con->_con->prepareStatement(
			"SELECT COUNT(*) as total FROM tieba_post WHERE tieba_id = ?"));
		pstmt_count->setInt(1, tieba_id);
		std::unique_ptr<sql::ResultSet> res_count(pstmt_count->executeQuery());
		if (res_count->next()) {
			total = res_count->getInt("total");
		}

		// 构建排序SQL
		std::string order_by;
		if (sort_type == 0) {
			// 按时间倒序
			order_by = "ORDER BY is_top DESC, create_time DESC";
		} else if (sort_type == 1) {
			// 按回复数倒序
			order_by = "ORDER BY is_top DESC, reply_count DESC, create_time DESC";
		} else {
			// 按点赞数倒序
			order_by = "ORDER BY is_top DESC, like_count DESC, create_time DESC";
		}

		// 获取帖子列表（JOIN user表获取用户名和头像）
		std::string sql = "SELECT p.post_id, p.tieba_id, p.uid, u.name as user_name, u.icon as user_icon, "
			"p.title, p.content, p.reply_count, p.like_count, p.is_top, p.is_essence, p.create_time "
			"FROM tieba_post p LEFT JOIN user u ON p.uid = u.uid "
			"WHERE p.tieba_id = ? " + order_by + " LIMIT ? OFFSET ?";
		
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(sql));
		pstmt->setInt(1, tieba_id);
		pstmt->setInt(2, limit);
		pstmt->setInt(3, offset);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		// 检查是否已点赞
		std::unique_ptr<sql::PreparedStatement> pstmt_like(con->_con->prepareStatement(
			"SELECT 1 FROM tieba_like WHERE uid = ? AND target_type = 1 AND target_id = ?"));

		while (res->next()) {
			auto info = std::make_shared<PostInfo>();
			info->post_id = res->getInt("post_id");
			info->tieba_id = res->getInt("tieba_id");
			info->uid = res->getInt("uid");
			info->user_name = res->getString("user_name");
			info->user_icon = res->getString("user_icon");
			info->title = res->getString("title");
			info->content = res->getString("content");
			info->reply_count = res->getInt("reply_count");
			info->like_count = res->getInt("like_count");
			info->is_top = res->getInt("is_top") == 1;
			info->is_essence = res->getInt("is_essence") == 1;
			
			// std::unique_ptr<sql::SQLString> time_str = res->getString("create_time");
			// if (time_str) {
			// 	info->create_time = time_str->asStdString();
			// }
			info->create_time = res->getString("create_time").asStdString();

			// 检查是否已点赞
			pstmt_like->setInt(1, uid);
			pstmt_like->setInt(2, info->post_id);
			std::unique_ptr<sql::ResultSet> res_like(pstmt_like->executeQuery());
			info->is_liked = res_like->next();

			post_list.push_back(info);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException in GetPostList: " << e.what() << std::endl;
		return false;
	}
}

// 获取回复列表
bool TiebaDao::GetReplyList(int uid, int post_id, int offset, int limit, std::vector<std::shared_ptr<ReplyInfo>>& reply_list, int& total) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		if (con) {
			pool_->returnConnection(std::move(con));
		}
	});

	try {
		// 获取总数
		std::unique_ptr<sql::PreparedStatement> pstmt_count(con->_con->prepareStatement(
			"SELECT COUNT(*) as total FROM tieba_reply WHERE post_id = ?"));
		pstmt_count->setInt(1, post_id);
		std::unique_ptr<sql::ResultSet> res_count(pstmt_count->executeQuery());
		if (res_count->next()) {
			total = res_count->getInt("total");
		}

		// 获取回复列表（JOIN user表获取用户名和头像）
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT r.reply_id, r.post_id, r.uid, u.name as user_name, u.icon as user_icon, "
			"r.content, r.reply_to_uid, r.reply_to_reply_id, r.like_count, r.create_time, "
			"ru.name as reply_to_user_name "
			"FROM tieba_reply r "
			"LEFT JOIN user u ON r.uid = u.uid "
			"LEFT JOIN user ru ON r.reply_to_uid = ru.uid "
			"WHERE r.post_id = ? ORDER BY r.create_time ASC LIMIT ? OFFSET ?"));
		pstmt->setInt(1, post_id);
		pstmt->setInt(2, limit);
		pstmt->setInt(3, offset);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		// 检查是否已点赞
		std::unique_ptr<sql::PreparedStatement> pstmt_like(con->_con->prepareStatement(
			"SELECT 1 FROM tieba_like WHERE uid = ? AND target_type = 2 AND target_id = ?"));

		while (res->next()) {
			auto info = std::make_shared<ReplyInfo>();
			info->reply_id = res->getInt("reply_id");
			info->post_id = res->getInt("post_id");
			info->uid = res->getInt("uid");
			info->user_name = res->getString("user_name");
			info->user_icon = res->getString("user_icon");
			info->content = res->getString("content");
			info->reply_to_uid = res->getInt("reply_to_uid");
			info->reply_to_user_name = res->getString("reply_to_user_name");
			info->reply_to_reply_id = res->getInt("reply_to_reply_id");
			info->like_count = res->getInt("like_count");
			
			// std::unique_ptr<sql::SQLString> time_str = res->getString("create_time");
			// if (time_str) {
			// 	info->create_time = time_str->asStdString();
			// }
			info->create_time = res->getString("create_time").asStdString();

			// 检查是否已点赞
			pstmt_like->setInt(1, uid);
			pstmt_like->setInt(2, info->reply_id);
			std::unique_ptr<sql::ResultSet> res_like(pstmt_like->executeQuery());
			info->is_liked = res_like->next();

			reply_list.push_back(info);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException in GetReplyList: " << e.what() << std::endl;
		return false;
	}
}

// 获取帖子信息
bool TiebaDao::GetPostInfo(int uid, int post_id, std::shared_ptr<PostInfo>& post_info) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		if (con) {
			pool_->returnConnection(std::move(con));
		}
	});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT p.post_id, p.tieba_id, p.uid, u.name as user_name, u.icon as user_icon, "
			"p.title, p.content, p.reply_count, p.like_count, p.is_top, p.is_essence, p.create_time "
			"FROM tieba_post p LEFT JOIN user u ON p.uid = u.uid WHERE p.post_id = ?"));
		pstmt->setInt(1, post_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		if (res->next()) {
			post_info = std::make_shared<PostInfo>();
			post_info->post_id = res->getInt("post_id");
			post_info->tieba_id = res->getInt("tieba_id");
			post_info->uid = res->getInt("uid");
			post_info->user_name = res->getString("user_name");
			post_info->user_icon = res->getString("user_icon");
			post_info->title = res->getString("title");
			post_info->content = res->getString("content");
			post_info->reply_count = res->getInt("reply_count");
			post_info->like_count = res->getInt("like_count");
			post_info->is_top = res->getInt("is_top") == 1;
			post_info->is_essence = res->getInt("is_essence") == 1;
			
			// std::unique_ptr<sql::SQLString> time_str = res->getString("create_time");
			// if (time_str) {
			// 	post_info->create_time = time_str->asStdString();
			// }
			post_info->create_time = res->getString("create_time").asStdString();

			// 检查是否已点赞
			std::unique_ptr<sql::PreparedStatement> pstmt_like(con->_con->prepareStatement(
				"SELECT 1 FROM tieba_like WHERE uid = ? AND target_type = 1 AND target_id = ?"));
			pstmt_like->setInt(1, uid);
			pstmt_like->setInt(2, post_id);
			std::unique_ptr<sql::ResultSet> res_like(pstmt_like->executeQuery());
			post_info->is_liked = res_like->next();

			return true;
		}
		return false;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException in GetPostInfo: " << e.what() << std::endl;
		return false;
	}
}

// 获取回复信息
bool TiebaDao::GetReplyInfo(int uid, int reply_id, std::shared_ptr<ReplyInfo>& reply_info) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		if (con) {
			pool_->returnConnection(std::move(con));
		}
	});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT r.reply_id, r.post_id, r.uid, u.name as user_name, u.icon as user_icon, "
			"r.content, r.reply_to_uid, r.reply_to_reply_id, r.like_count, r.create_time, "
			"ru.name as reply_to_user_name "
			"FROM tieba_reply r "
			"LEFT JOIN user u ON r.uid = u.uid "
			"LEFT JOIN user ru ON r.reply_to_uid = ru.uid "
			"WHERE r.reply_id = ?"));
		pstmt->setInt(1, reply_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		if (res->next()) {
			reply_info = std::make_shared<ReplyInfo>();
			reply_info->reply_id = res->getInt("reply_id");
			reply_info->post_id = res->getInt("post_id");
			reply_info->uid = res->getInt("uid");
			reply_info->user_name = res->getString("user_name");
			reply_info->user_icon = res->getString("user_icon");
			reply_info->content = res->getString("content");
			reply_info->reply_to_uid = res->getInt("reply_to_uid");
			reply_info->reply_to_user_name = res->getString("reply_to_user_name");
			reply_info->reply_to_reply_id = res->getInt("reply_to_reply_id");
			reply_info->like_count = res->getInt("like_count");
			
			// std::unique_ptr<sql::SQLString> time_str = res->getString("create_time");
			// if (time_str) {
			// 	reply_info->create_time = time_str->asStdString();
			// }
			reply_info->create_time = res->getString("create_time").asStdString();

			// 检查是否已点赞
			std::unique_ptr<sql::PreparedStatement> pstmt_like(con->_con->prepareStatement(
				"SELECT 1 FROM tieba_like WHERE uid = ? AND target_type = 2 AND target_id = ?"));
			pstmt_like->setInt(1, uid);
			pstmt_like->setInt(2, reply_id);
			std::unique_ptr<sql::ResultSet> res_like(pstmt_like->executeQuery());
			reply_info->is_liked = res_like->next();

			return true;
		}
		return false;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException in GetReplyInfo: " << e.what() << std::endl;
		return false;
	}
}

// 删除帖子
bool TiebaDao::DeletePost(int uid, int post_id, int tieba_id) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		if (con) {
			pool_->returnConnection(std::move(con));
		}
	});

	try {
		con->_con->setAutoCommit(false);

		// 检查权限（只有发帖人、吧主或小吧主可以删除）
		int role = 0;
		bool is_member = IsTiebaMember(uid, tieba_id, role);
		
		std::unique_ptr<sql::PreparedStatement> pstmt_check(con->_con->prepareStatement(
			"SELECT uid FROM tieba_post WHERE post_id = ?"));
		pstmt_check->setInt(1, post_id);
		std::unique_ptr<sql::ResultSet> res_check(pstmt_check->executeQuery());
		if (!res_check->next()) {
			con->_con->rollback();
			return false;  // 帖子不存在
		}
		int post_uid = res_check->getInt("uid");
		
		// 检查权限：发帖人、吧主(role=2)或小吧主(role=1)可以删除
		if (post_uid != uid && (!is_member || (role != 1 && role != 2))) {
			con->_con->rollback();
			return false;  // 权限不足
		}

		// 先获取所有回复ID
		std::vector<int> reply_ids;
		std::unique_ptr<sql::PreparedStatement> pstmt_get_replies(con->_con->prepareStatement(
			"SELECT reply_id FROM tieba_reply WHERE post_id = ?"));
		pstmt_get_replies->setInt(1, post_id);
		std::unique_ptr<sql::ResultSet> res_replies(pstmt_get_replies->executeQuery());
		while (res_replies->next()) {
			reply_ids.push_back(res_replies->getInt("reply_id"));
		}

		// 删除所有回复的点赞记录
		if (!reply_ids.empty()) {
			std::unique_ptr<sql::PreparedStatement> pstmt_delete_reply_likes(con->_con->prepareStatement(
				"DELETE FROM tieba_like WHERE target_type = 2 AND target_id = ?"));
			for (int reply_id : reply_ids) {
				pstmt_delete_reply_likes->setInt(1, reply_id);
				pstmt_delete_reply_likes->executeUpdate();
			}
		}

		// 删除所有回复
		std::unique_ptr<sql::PreparedStatement> pstmt_delete_replies(con->_con->prepareStatement(
			"DELETE FROM tieba_reply WHERE post_id = ?"));
		pstmt_delete_replies->setInt(1, post_id);
		pstmt_delete_replies->executeUpdate();

		// 删除帖子的点赞记录
		std::unique_ptr<sql::PreparedStatement> pstmt_delete_post_likes(con->_con->prepareStatement(
			"DELETE FROM tieba_like WHERE target_type = 1 AND target_id = ?"));
		pstmt_delete_post_likes->setInt(1, post_id);
		pstmt_delete_post_likes->executeUpdate();

		// 删除帖子
		std::unique_ptr<sql::PreparedStatement> pstmt_delete_post(con->_con->prepareStatement(
			"DELETE FROM tieba_post WHERE post_id = ?"));
		pstmt_delete_post->setInt(1, post_id);
		pstmt_delete_post->executeUpdate();

		// 更新贴吧帖子数量
		std::unique_ptr<sql::PreparedStatement> pstmt_update(con->_con->prepareStatement(
			"UPDATE tieba SET post_count = post_count - 1 WHERE tieba_id = ?"));
		pstmt_update->setInt(1, tieba_id);
		pstmt_update->executeUpdate();

		con->_con->commit();
		return true;
	}
	catch (sql::SQLException& e) {
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException in DeletePost: " << e.what() << std::endl;
		return false;
	}
}

// 删除回复
bool TiebaDao::DeleteReply(int uid, int reply_id, int post_id) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		if (con) {
			pool_->returnConnection(std::move(con));
		}
	});

	try {
		con->_con->setAutoCommit(false);

		// 检查权限（只有回复人、吧主或小吧主可以删除）
		std::unique_ptr<sql::PreparedStatement> pstmt_check(con->_con->prepareStatement(
			"SELECT r.uid, p.tieba_id FROM tieba_reply r "
			"JOIN tieba_post p ON r.post_id = p.post_id "
			"WHERE r.reply_id = ?"));
		pstmt_check->setInt(1, reply_id);
		std::unique_ptr<sql::ResultSet> res_check(pstmt_check->executeQuery());
		if (!res_check->next()) {
			con->_con->rollback();
			return false;  // 回复不存在
		}
		int reply_uid = res_check->getInt("uid");
		int tieba_id = res_check->getInt("tieba_id");
		
		int role = 0;
		bool is_member = IsTiebaMember(uid, tieba_id, role);
		
		// 检查权限：回复人、吧主(role=2)或小吧主(role=1)可以删除
		if (reply_uid != uid && (!is_member || (role != 1 && role != 2))) {
			con->_con->rollback();
			return false;  // 权限不足
		}

		// 删除回复的点赞记录
		std::unique_ptr<sql::PreparedStatement> pstmt_delete_likes(con->_con->prepareStatement(
			"DELETE FROM tieba_like WHERE target_type = 2 AND target_id = ?"));
		pstmt_delete_likes->setInt(1, reply_id);
		pstmt_delete_likes->executeUpdate();

		// 删除回复
		std::unique_ptr<sql::PreparedStatement> pstmt_delete_reply(con->_con->prepareStatement(
			"DELETE FROM tieba_reply WHERE reply_id = ?"));
		pstmt_delete_reply->setInt(1, reply_id);
		pstmt_delete_reply->executeUpdate();

		// 更新帖子回复数量
		std::unique_ptr<sql::PreparedStatement> pstmt_update(con->_con->prepareStatement(
			"UPDATE tieba_post SET reply_count = reply_count - 1 WHERE post_id = ?"));
		pstmt_update->setInt(1, post_id);
		pstmt_update->executeUpdate();

		con->_con->commit();
		return true;
	}
	catch (sql::SQLException& e) {
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException in DeleteReply: " << e.what() << std::endl;
		return false;
	}
}

// 点赞/取消点赞
bool TiebaDao::Like(int uid, int target_type, int target_id, bool is_like, int& like_count) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		if (con) {
			pool_->returnConnection(std::move(con));
		}
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

// 检查是否点赞
bool TiebaDao::IsLiked(int uid, int target_type, int target_id) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		if (con) {
			pool_->returnConnection(std::move(con));
		}
	});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT 1 FROM tieba_like WHERE uid = ? AND target_type = ? AND target_id = ?"));
		pstmt->setInt(1, uid);
		pstmt->setInt(2, target_type);
		pstmt->setInt(3, target_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		return res->next();
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException in IsLiked: " << e.what() << std::endl;
		return false;
	}
}

// 置顶
bool TiebaDao::SetTop(int uid, int post_id, int tieba_id, bool is_top) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		if (con) {
			pool_->returnConnection(std::move(con));
		}
	});

	try {
		con->_con->setAutoCommit(false);

		// 检查权限（只有吧主或小吧主可以置顶）
		int role = 0;
		bool is_member = IsTiebaMember(uid, tieba_id, role);
		if (!is_member || (role != 1 && role != 2)) {
			con->_con->rollback();
			return false;  // 权限不足
		}

		// 检查帖子是否存在
		std::unique_ptr<sql::PreparedStatement> pstmt_check(con->_con->prepareStatement(
			"SELECT 1 FROM tieba_post WHERE post_id = ? AND tieba_id = ?"));
		pstmt_check->setInt(1, post_id);
		pstmt_check->setInt(2, tieba_id);
		std::unique_ptr<sql::ResultSet> res_check(pstmt_check->executeQuery());
		if (!res_check->next()) {
			con->_con->rollback();
			return false;  // 帖子不存在
		}

		// 更新置顶状态
		std::unique_ptr<sql::PreparedStatement> pstmt_update(con->_con->prepareStatement(
			"UPDATE tieba_post SET is_top = ? WHERE post_id = ?"));
		pstmt_update->setInt(1, is_top ? 1 : 0);
		pstmt_update->setInt(2, post_id);
		pstmt_update->executeUpdate();

		con->_con->commit();
		return true;
	}
	catch (sql::SQLException& e) {
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException in SetTop: " << e.what() << std::endl;
		return false;
	}
}

// 加精
bool TiebaDao::SetEssence(int uid, int post_id, int tieba_id, bool is_essence) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		if (con) {
			pool_->returnConnection(std::move(con));
		}
	});

	try {
		con->_con->setAutoCommit(false);

		// 检查权限（只有吧主或小吧主可以加精）
		int role = 0;
		bool is_member = IsTiebaMember(uid, tieba_id, role);
		if (!is_member || (role != 1 && role != 2)) {
			con->_con->rollback();
			return false;  // 权限不足
		}

		// 检查帖子是否存在
		std::unique_ptr<sql::PreparedStatement> pstmt_check(con->_con->prepareStatement(
			"SELECT 1 FROM tieba_post WHERE post_id = ? AND tieba_id = ?"));
		pstmt_check->setInt(1, post_id);
		pstmt_check->setInt(2, tieba_id);
		std::unique_ptr<sql::ResultSet> res_check(pstmt_check->executeQuery());
		if (!res_check->next()) {
			con->_con->rollback();
			return false;  // 帖子不存在
		}

		// 更新加精状态
		std::unique_ptr<sql::PreparedStatement> pstmt_update(con->_con->prepareStatement(
			"UPDATE tieba_post SET is_essence = ? WHERE post_id = ?"));
		pstmt_update->setInt(1, is_essence ? 1 : 0);
		pstmt_update->setInt(2, post_id);
		pstmt_update->executeUpdate();

		con->_con->commit();
		return true;
	}
	catch (sql::SQLException& e) {
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException in SetEssence: " << e.what() << std::endl;
		return false;
	}
}

// 获取我的贴吧列表
bool TiebaDao::GetMyTiebaList(int uid, std::vector<std::shared_ptr<TiebaInfo>>& tieba_list) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		if (con) {
			pool_->returnConnection(std::move(con));
		}
	});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT t.tieba_id, t.tieba_name, t.tieba_desc, t.tieba_icon, t.owner_uid, "
			"t.member_count, t.post_count, t.create_time "
			"FROM tieba t "
			"JOIN tieba_member tm ON t.tieba_id = tm.tieba_id "
			"WHERE tm.uid = ? ORDER BY t.create_time DESC"));
		pstmt->setInt(1, uid);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		while (res->next()) {
			auto info = std::make_shared<TiebaInfo>();
			info->tieba_id = res->getInt("tieba_id");
			info->tieba_name = res->getString("tieba_name");
			info->tieba_desc = res->getString("tieba_desc");
			info->tieba_icon = res->getString("tieba_icon");
			info->owner_uid = res->getInt("owner_uid");
			info->member_count = res->getInt("member_count");
			info->post_count = res->getInt("post_count");
			
			// std::unique_ptr<sql::SQLString> time_str = res->getString("create_time");
			// if (time_str) {
			// 	info->create_time = time_str->asStdString();
			// }
			info->create_time = res->getString("create_time").asStdString();
			tieba_list.push_back(info);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException in GetMyTiebaList: " << e.what() << std::endl;
		return false;
	}
}

// 获取贴吧成员列表
bool TiebaDao::GetTiebaMemberList(int uid, int tieba_id, int offset, int limit, std::vector<std::shared_ptr<TiebaMemberInfo>>& member_list, int& total) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		if (con) {
			pool_->returnConnection(std::move(con));
		}
	});

	try {
		// 获取总数
		std::unique_ptr<sql::PreparedStatement> pstmt_count(con->_con->prepareStatement(
			"SELECT COUNT(*) as total FROM tieba_member WHERE tieba_id = ?"));
		pstmt_count->setInt(1, tieba_id);
		std::unique_ptr<sql::ResultSet> res_count(pstmt_count->executeQuery());
		if (res_count->next()) {
			total = res_count->getInt("total");
		}

		// 获取成员列表（JOIN user表获取用户名和头像）
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT tm.uid, u.name as user_name, u.icon as user_icon, tm.role, tm.join_time "
			"FROM tieba_member tm "
			"LEFT JOIN user u ON tm.uid = u.uid "
			"WHERE tm.tieba_id = ? ORDER BY tm.role DESC, tm.join_time ASC LIMIT ? OFFSET ?"));
		pstmt->setInt(1, tieba_id);
		pstmt->setInt(2, limit);
		pstmt->setInt(3, offset);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		while (res->next()) {
			auto info = std::make_shared<TiebaMemberInfo>();
			info->uid = res->getInt("uid");
			info->user_name = res->getString("user_name");
			info->user_icon = res->getString("user_icon");
			info->role = res->getInt("role");
			
			// std::unique_ptr<sql::SQLString> time_str = res->getString("join_time");
			// if (time_str) {
			// 	info->join_time = time_str->asStdString();
			// }
			info->join_time = res->getString("join_time").asStdString();
			member_list.push_back(info);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException in GetTiebaMemberList: " << e.what() << std::endl;
		return false;
	}
}

