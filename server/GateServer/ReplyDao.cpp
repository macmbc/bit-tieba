#include "ReplyDao.h"
#include <sstream>
#include <cctype>

namespace {
std::string NormalizeSort(const std::string& sort) {
	std::string s = sort;
	for (auto& c : s) c = std::tolower(c);
	if (s == "hot") return "hot";
	if (s == "time") return "time";
	return "time";
}
}

ReplyDao::ReplyDao(MySqlPool* pool) : _pool(pool) {}

bool ReplyDao::ListTopReplies(int post_id, int page, int limit, const std::string& sort, std::vector<ReplyInfo>& replies) {
	auto con = _pool->getConnection();
	if (con == nullptr) return false;
	Defer defer([this, &con]() { _pool->returnConnection(std::move(con)); });

	try {
		auto offset = (page - 1) * limit;
		auto sort_mode = NormalizeSort(sort);
		std::string order_by = sort_mode == "hot" ? "r.like_cnt DESC, r.created_at DESC" : "r.created_at DESC";
		std::string sql = "SELECT r.reply_id,r.post_id,r.uid,r.content,r.parent_reply_id,r.root_reply_id,"
			"r.floor,r.like_cnt,r.created_at "
			"FROM reply r "
			"WHERE r.post_id = ? AND r.status = 1 AND r.parent_reply_id = 0 "
			"ORDER BY " + order_by + " LIMIT ? OFFSET ?";
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(sql));
		pstmt->setInt(1, post_id);
		pstmt->setInt(2, limit);
		pstmt->setInt(3, offset);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			ReplyInfo info;
			info.reply_id = res->getInt64("reply_id");
			info.post_id = res->getInt("post_id");
			info.uid = res->getInt("uid");
			info.content = res->getString("content");
			info.parent_reply_id = res->getInt64("parent_reply_id");
			info.root_reply_id = res->getInt64("root_reply_id");
			info.floor = res->getInt("floor");
			info.like_cnt = res->getInt("like_cnt");
			info.created_at = res->getString("created_at");
			replies.push_back(info);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException ListTopReplies: " << e.what() << std::endl;
		return false;
	}
}

bool ReplyDao::ListChildren(int post_id, const std::vector<long long>& root_ids, std::unordered_map<long long, std::vector<ReplyInfo>>& children) {
	if (root_ids.empty()) return true;
	auto con = _pool->getConnection();
	if (con == nullptr) return false;
	Defer defer([this, &con]() { _pool->returnConnection(std::move(con)); });

	try {
		std::stringstream ss;
		ss << "SELECT reply_id,post_id,uid,content,parent_reply_id,root_reply_id,floor,like_cnt,created_at "
			"FROM reply WHERE post_id = ? AND status = 1 AND root_reply_id IN (";
		for (size_t i = 0; i < root_ids.size(); ++i) {
			if (i > 0) ss << ",";
			ss << "?";
		}
		ss << ") AND parent_reply_id <> 0 ORDER BY created_at ASC";

		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(ss.str()));
		pstmt->setInt(1, post_id);
		for (size_t i = 0; i < root_ids.size(); ++i) {
			pstmt->setInt64(static_cast<int>(i + 2), root_ids[i]);
		}
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			ReplyInfo info;
			info.reply_id = res->getInt64("reply_id");
			info.post_id = res->getInt("post_id");
			info.uid = res->getInt("uid");
			info.content = res->getString("content");
			info.parent_reply_id = res->getInt64("parent_reply_id");
			info.root_reply_id = res->getInt64("root_reply_id");
			info.floor = res->getInt("floor");
			info.like_cnt = res->getInt("like_cnt");
			info.created_at = res->getString("created_at");
			children[info.root_reply_id].push_back(info);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException ListChildren: " << e.what() << std::endl;
		return false;
	}
}

bool ReplyDao::CreateReply(int uid, int post_id, const std::string& content, long long parent_reply_id,
	long long& reply_id, long long& root_reply_id, int& floor) {
	auto con = _pool->getConnection();
	if (con == nullptr) return false;
	Defer defer([this, &con]() { _pool->returnConnection(std::move(con)); });

	try {
		bool orig_ac = con->_con->getAutoCommit();
		con->_con->setAutoCommit(false);

		root_reply_id = 0;
		if (parent_reply_id > 0) {
			std::unique_ptr<sql::PreparedStatement> pstmt_parent(con->_con->prepareStatement(
				"SELECT root_reply_id,parent_reply_id,status FROM reply WHERE reply_id = ?"));
			pstmt_parent->setInt64(1, parent_reply_id);
			std::unique_ptr<sql::ResultSet> res_parent(pstmt_parent->executeQuery());
			if (!res_parent->next() || res_parent->getInt("status") != 1) {
				con->_con->rollback();
				con->_con->setAutoCommit(orig_ac);
				return false;
			}
			auto parent_root = res_parent->getInt64("root_reply_id");
			auto parent_parent = res_parent->getInt64("parent_reply_id");
			root_reply_id = parent_root == 0 ? parent_reply_id : parent_root;
			floor = 0; // 楼中楼不需要楼层号
		}
		else {
			// 顶层楼层号递增
			std::unique_ptr<sql::PreparedStatement> pstmt_floor(con->_con->prepareStatement(
				"SELECT IFNULL(MAX(floor),0) AS max_floor FROM reply WHERE post_id = ? AND parent_reply_id = 0 AND status = 1"));
			pstmt_floor->setInt(1, post_id);
			std::unique_ptr<sql::ResultSet> res_floor(pstmt_floor->executeQuery());
			int max_floor = 0;
			if (res_floor->next()) max_floor = res_floor->getInt("max_floor");
			floor = max_floor + 1;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt_insert(con->_con->prepareStatement(
			"INSERT INTO reply(post_id, uid, content, parent_reply_id, root_reply_id, floor) "
			"VALUES(?, ?, ?, ?, ?, ?)"));
		pstmt_insert->setInt(1, post_id);
		pstmt_insert->setInt(2, uid);
		pstmt_insert->setString(3, content);
		pstmt_insert->setInt64(4, parent_reply_id);
		pstmt_insert->setInt64(5, root_reply_id);
		pstmt_insert->setInt(6, floor);
		pstmt_insert->executeUpdate();

		std::unique_ptr<sql::PreparedStatement> pstmt_id(con->_con->prepareStatement("SELECT LAST_INSERT_ID() AS id"));
		std::unique_ptr<sql::ResultSet> res_id(pstmt_id->executeQuery());
		if (!res_id->next()) {
			con->_con->rollback();
			con->_con->setAutoCommit(orig_ac);
			return false;
		}
		reply_id = res_id->getInt64("id");

		// 更新 root_reply_id 对于顶层回复（保持为 0 即可）不改；楼中楼已设置
		std::unique_ptr<sql::PreparedStatement> pstmt_up_post(con->_con->prepareStatement(
			"UPDATE post SET reply_cnt = reply_cnt + 1, last_reply_at = NOW() WHERE post_id = ? AND status = 1"));
		pstmt_up_post->setInt(1, post_id);
		pstmt_up_post->executeUpdate();

		con->_con->commit();
		con->_con->setAutoCommit(orig_ac);
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException CreateReply: " << e.what() << std::endl;
		try { con->_con->rollback(); con->_con->setAutoCommit(true); } catch (...) {}
		return false;
	}
}

bool ReplyDao::GetReplyOwner(long long reply_id, int& owner_uid, int& post_id, long long& parent_reply_id, long long& root_reply_id) {
	auto con = _pool->getConnection();
	if (con == nullptr) return false;
	Defer defer([this, &con]() { _pool->returnConnection(std::move(con)); });
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT uid, post_id, parent_reply_id, root_reply_id, status FROM reply WHERE reply_id = ?"));
		pstmt->setInt64(1, reply_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (!res->next() || res->getInt("status") != 1) {
			return false;
		}
		owner_uid = res->getInt("uid");
		post_id = res->getInt("post_id");
		parent_reply_id = res->getInt64("parent_reply_id");
		root_reply_id = res->getInt64("root_reply_id");
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException GetReplyOwner: " << e.what() << std::endl;
		return false;
	}
}

bool ReplyDao::DeleteReply(long long reply_id, int post_id) {
	auto con = _pool->getConnection();
	if (con == nullptr) return false;
	Defer defer([this, &con]() { _pool->returnConnection(std::move(con)); });
	try {
		bool orig_ac = con->_con->getAutoCommit();
		con->_con->setAutoCommit(false);

		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"UPDATE reply SET status = 0 WHERE reply_id = ? AND status = 1"));
		pstmt->setInt64(1, reply_id);
		auto count = pstmt->executeUpdate();
		if (count == 0) {
			con->_con->rollback();
			con->_con->setAutoCommit(orig_ac);
			return false;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt_up(con->_con->prepareStatement(
			"UPDATE post SET reply_cnt = GREATEST(reply_cnt - 1, 0) WHERE post_id = ? AND status = 1"));
		pstmt_up->setInt(1, post_id);
		pstmt_up->executeUpdate();

		con->_con->commit();
		con->_con->setAutoCommit(orig_ac);
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException DeleteReply: " << e.what() << std::endl;
		try { con->_con->rollback(); con->_con->setAutoCommit(true); } catch (...) {}
		return false;
	}
}

int ReplyDao::LikeReply(int uid, long long reply_id) {
	auto con = _pool->getConnection();
	if (con == nullptr) return -1;
	Defer defer([this, &con]() { _pool->returnConnection(std::move(con)); });
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt_check(con->_con->prepareStatement(
			"SELECT status FROM reply WHERE reply_id = ?"));
		pstmt_check->setInt64(1, reply_id);
		std::unique_ptr<sql::ResultSet> res(pstmt_check->executeQuery());
		if (!res->next() || res->getInt("status") != 1) {
			return 2;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt_exist(con->_con->prepareStatement(
			"SELECT 1 FROM reply_like WHERE reply_id = ? AND uid = ? LIMIT 1"));
		pstmt_exist->setInt64(1, reply_id);
		pstmt_exist->setInt(2, uid);
		std::unique_ptr<sql::ResultSet> res_exist(pstmt_exist->executeQuery());
		if (res_exist->next()) return 1;

		bool orig_ac = con->_con->getAutoCommit();
		con->_con->setAutoCommit(false);

		std::unique_ptr<sql::PreparedStatement> pstmt_insert(con->_con->prepareStatement(
			"INSERT INTO reply_like(reply_id, uid) VALUES(?, ?)"));
		pstmt_insert->setInt64(1, reply_id);
		pstmt_insert->setInt(2, uid);
		pstmt_insert->executeUpdate();

		std::unique_ptr<sql::PreparedStatement> pstmt_up(con->_con->prepareStatement(
			"UPDATE reply SET like_cnt = like_cnt + 1 WHERE reply_id = ?"));
		pstmt_up->setInt64(1, reply_id);
		pstmt_up->executeUpdate();

		con->_con->commit();
		con->_con->setAutoCommit(orig_ac);
		return 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException LikeReply: " << e.what() << std::endl;
		try { con->_con->rollback(); con->_con->setAutoCommit(true); } catch (...) {}
		return -1;
	}
}

int ReplyDao::UnlikeReply(int uid, long long reply_id) {
	auto con = _pool->getConnection();
	if (con == nullptr) return -1;
	Defer defer([this, &con]() { _pool->returnConnection(std::move(con)); });
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt_check(con->_con->prepareStatement(
			"SELECT status FROM reply WHERE reply_id = ?"));
		pstmt_check->setInt64(1, reply_id);
		std::unique_ptr<sql::ResultSet> res(pstmt_check->executeQuery());
		if (!res->next() || res->getInt("status") != 1) {
			return 2;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt_exist(con->_con->prepareStatement(
			"SELECT 1 FROM reply_like WHERE reply_id = ? AND uid = ? LIMIT 1"));
		pstmt_exist->setInt64(1, reply_id);
		pstmt_exist->setInt(2, uid);
		std::unique_ptr<sql::ResultSet> res_exist(pstmt_exist->executeQuery());
		if (!res_exist->next()) return 1;

		bool orig_ac = con->_con->getAutoCommit();
		con->_con->setAutoCommit(false);

		std::unique_ptr<sql::PreparedStatement> pstmt_del(con->_con->prepareStatement(
			"DELETE FROM reply_like WHERE reply_id = ? AND uid = ?"));
		pstmt_del->setInt64(1, reply_id);
		pstmt_del->setInt(2, uid);
		pstmt_del->executeUpdate();

		std::unique_ptr<sql::PreparedStatement> pstmt_up(con->_con->prepareStatement(
			"UPDATE reply SET like_cnt = GREATEST(like_cnt - 1, 0) WHERE reply_id = ?"));
		pstmt_up->setInt64(1, reply_id);
		pstmt_up->executeUpdate();

		con->_con->commit();
		con->_con->setAutoCommit(orig_ac);
		return 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException UnlikeReply: " << e.what() << std::endl;
		try { con->_con->rollback(); con->_con->setAutoCommit(true); } catch (...) {}
		return -1;
	}
}

bool ReplyDao::BatchLiked(int uid, const std::vector<long long>& reply_ids, std::unordered_set<long long>& liked_set) {
	if (reply_ids.empty()) return true;
	auto con = _pool->getConnection();
	if (con == nullptr) return false;
	Defer defer([this, &con]() { _pool->returnConnection(std::move(con)); });

	try {
		std::stringstream ss;
		ss << "SELECT reply_id FROM reply_like WHERE uid = ? AND reply_id IN (";
		for (size_t i = 0; i < reply_ids.size(); ++i) {
			if (i > 0) ss << ",";
			ss << "?";
		}
		ss << ")";
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(ss.str()));
		pstmt->setInt(1, uid);
		for (size_t i = 0; i < reply_ids.size(); ++i) {
			pstmt->setInt64(static_cast<int>(i + 2), reply_ids[i]);
		}
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			liked_set.insert(res->getInt64("reply_id"));
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException BatchLiked: " << e.what() << std::endl;
		return false;
	}
}
