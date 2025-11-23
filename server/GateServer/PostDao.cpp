#include "PostDao.h"
#include <cctype>

namespace {
std::string NormalizeSort(const std::string& sort) {
	std::string s = sort;
	for (auto& c : s) c = std::tolower(c);
	if (s == "hot") return "hot";
	if (s == "reply") return "reply";
	return "time";
}
}

PostDao::PostDao(MySqlPool* pool) : _pool(pool) {}

bool PostDao::ListPosts(int forum_id, int page, int limit, const std::string& sort, std::vector<PostSummary>& posts) {
	auto con = _pool->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		_pool->returnConnection(std::move(con));
		});

	try {
		auto offset = (page - 1) * limit;
		auto sort_mode = NormalizeSort(sort);
		std::string order_by;
		if (sort_mode == "reply") {
			order_by = "p.reply_cnt DESC, p.created_at DESC";
		}
		else if (sort_mode == "hot") {
			order_by = "(p.reply_cnt * 2 + p.like_cnt) DESC, p.created_at DESC";
		}
		else {
			order_by = "p.created_at DESC";
		}

		std::string sql = "SELECT p.post_id,p.forum_id,p.uid,p.title,p.created_at,"
			"p.reply_cnt,p.like_cnt,p.is_top,p.is_essence,"
			"SUBSTRING(p.content,1,120) AS preview, IFNULL(u.nick, u.name) AS author "
			"FROM post p LEFT JOIN user u ON p.uid = u.uid "
			"WHERE p.forum_id = ? AND p.status = 1 "
			"ORDER BY " + order_by + " LIMIT ? OFFSET ?";

		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(sql));
		pstmt->setInt(1, forum_id);
		pstmt->setInt(2, limit);
		pstmt->setInt(3, offset);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			PostSummary ps;
			ps.post_id = res->getInt("post_id");
			ps.forum_id = res->getInt("forum_id");
			ps.uid = res->getInt("uid");
			ps.title = res->getString("title");
			ps.created_at = res->getString("created_at");
			ps.reply_cnt = res->getInt("reply_cnt");
			ps.like_cnt = res->getInt("like_cnt");
			ps.is_top = res->getInt("is_top") != 0;
			ps.is_essence = res->getInt("is_essence") != 0;
			ps.author = res->getString("author");
			ps.content_preview = res->getString("preview");
			posts.push_back(ps);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException ListPosts: " << e.what() << std::endl;
		return false;
	}
}

bool PostDao::GetPostDetail(int post_id, PostDetail& detail) {
	auto con = _pool->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		_pool->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT p.post_id,p.forum_id,p.uid,p.title,p.content,p.created_at,"
			"p.reply_cnt,p.like_cnt,p.collect_cnt,p.is_top,p.is_essence,p.status,"
			"IFNULL(u.nick,u.name) AS author "
			"FROM post p LEFT JOIN user u ON p.uid = u.uid "
			"WHERE p.post_id = ?"));
		pstmt->setInt(1, post_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (!res->next()) {
			return false;
		}
		if (res->getInt("status") != 1) {
			return false;
		}
		detail.post_id = res->getInt("post_id");
		detail.forum_id = res->getInt("forum_id");
		detail.uid = res->getInt("uid");
		detail.title = res->getString("title");
		detail.content = res->getString("content");
		detail.created_at = res->getString("created_at");
		detail.reply_cnt = res->getInt("reply_cnt");
		detail.like_cnt = res->getInt("like_cnt");
		detail.collect_cnt = res->getInt("collect_cnt");
		detail.is_top = res->getInt("is_top") != 0;
		detail.is_essence = res->getInt("is_essence") != 0;
		detail.author = res->getString("author");
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException GetPostDetail: " << e.what() << std::endl;
		return false;
	}
}

bool PostDao::GetPostOwner(int post_id, int& owner_uid, int& forum_id) {
	auto con = _pool->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		_pool->returnConnection(std::move(con));
		});
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT uid, forum_id, status FROM post WHERE post_id = ?"));
		pstmt->setInt(1, post_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (!res->next()) {
			return false;
		}
		if (res->getInt("status") != 1) {
			return false;
		}
		owner_uid = res->getInt("uid");
		forum_id = res->getInt("forum_id");
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException GetPostOwner: " << e.what() << std::endl;
		return false;
	}
}

bool PostDao::CreatePost(int uid, int forum_id, const std::string& title, const std::string& content, int& post_id) {
	auto con = _pool->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		_pool->returnConnection(std::move(con));
		});

	try {
		con->_con->setAutoCommit(false);
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"INSERT INTO post(forum_id, uid, title, content) VALUES(?, ?, ?, ?)"));
		pstmt->setInt(1, forum_id);
		pstmt->setInt(2, uid);
		pstmt->setString(3, title);
		pstmt->setString(4, content);
		pstmt->executeUpdate();

		std::unique_ptr<sql::PreparedStatement> pstmt_id(con->_con->prepareStatement("SELECT LAST_INSERT_ID() AS id"));
		std::unique_ptr<sql::ResultSet> res(pstmt_id->executeQuery());
		if (!res->next()) {
			con->_con->rollback();
			return false;
		}
		post_id = res->getInt("id");

		std::unique_ptr<sql::PreparedStatement> pstmt_forum(con->_con->prepareStatement(
			"UPDATE forum SET post_cnt = post_cnt + 1 WHERE forum_id = ?"));
		pstmt_forum->setInt(1, forum_id);
		pstmt_forum->executeUpdate();

		con->_con->commit();
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException CreatePost: " << e.what() << std::endl;
		try { con->_con->rollback(); } catch (...) {}
		return false;
	}
}

bool PostDao::UpdatePost(int post_id, int uid, const std::string& title, const std::string& content) {
	auto con = _pool->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		_pool->returnConnection(std::move(con));
		});
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"UPDATE post SET title = ?, content = ? WHERE post_id = ? AND uid = ? AND status = 1"));
		pstmt->setString(1, title);
		pstmt->setString(2, content);
		pstmt->setInt(3, post_id);
		pstmt->setInt(4, uid);
		auto count = pstmt->executeUpdate();
		return count > 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException UpdatePost: " << e.what() << std::endl;
		return false;
	}
}

bool PostDao::DeletePost(int post_id, int forum_id) {
	auto con = _pool->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		_pool->returnConnection(std::move(con));
		});
	try {
		con->_con->setAutoCommit(false);
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"UPDATE post SET status = 0 WHERE post_id = ? AND status = 1"));
		pstmt->setInt(1, post_id);
		auto count = pstmt->executeUpdate();
		if (count == 0) {
			con->_con->rollback();
			return false;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt_forum(con->_con->prepareStatement(
			"UPDATE forum SET post_cnt = GREATEST(post_cnt - 1, 0) WHERE forum_id = ?"));
		pstmt_forum->setInt(1, forum_id);
		pstmt_forum->executeUpdate();

		con->_con->commit();
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException DeletePost: " << e.what() << std::endl;
		try { con->_con->rollback(); } catch (...) {}
		return false;
	}
}

int PostDao::LikePost(int uid, int post_id) {
	auto con = _pool->getConnection();
	if (con == nullptr) {
		return -1;
	}
	Defer defer([this, &con]() {
		_pool->returnConnection(std::move(con));
		});
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt_check(con->_con->prepareStatement(
			"SELECT status FROM post WHERE post_id = ?"));
		pstmt_check->setInt(1, post_id);
		std::unique_ptr<sql::ResultSet> res(pstmt_check->executeQuery());
		if (!res->next() || res->getInt("status") != 1) {
			return 2;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt_exist(con->_con->prepareStatement(
			"SELECT 1 FROM post_like WHERE post_id = ? AND uid = ? LIMIT 1"));
		pstmt_exist->setInt(1, post_id);
		pstmt_exist->setInt(2, uid);
		std::unique_ptr<sql::ResultSet> res_exist(pstmt_exist->executeQuery());
		if (res_exist->next()) {
			return 1;
		}

		con->_con->setAutoCommit(false);
		std::unique_ptr<sql::PreparedStatement> pstmt_insert(con->_con->prepareStatement(
			"INSERT INTO post_like(post_id, uid) VALUES(?, ?)"));
		pstmt_insert->setInt(1, post_id);
		pstmt_insert->setInt(2, uid);
		pstmt_insert->executeUpdate();

		std::unique_ptr<sql::PreparedStatement> pstmt_up(con->_con->prepareStatement(
			"UPDATE post SET like_cnt = like_cnt + 1 WHERE post_id = ?"));
		pstmt_up->setInt(1, post_id);
		pstmt_up->executeUpdate();

		con->_con->commit();
		return 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException LikePost: " << e.what() << std::endl;
		try { con->_con->rollback(); } catch (...) {}
		return -1;
	}
}

int PostDao::UnlikePost(int uid, int post_id) {
	auto con = _pool->getConnection();
	if (con == nullptr) {
		return -1;
	}
	Defer defer([this, &con]() {
		_pool->returnConnection(std::move(con));
		});
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt_check(con->_con->prepareStatement(
			"SELECT status FROM post WHERE post_id = ?"));
		pstmt_check->setInt(1, post_id);
		std::unique_ptr<sql::ResultSet> res(pstmt_check->executeQuery());
		if (!res->next() || res->getInt("status") != 1) {
			return 2;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt_exist(con->_con->prepareStatement(
			"SELECT 1 FROM post_like WHERE post_id = ? AND uid = ? LIMIT 1"));
		pstmt_exist->setInt(1, post_id);
		pstmt_exist->setInt(2, uid);
		std::unique_ptr<sql::ResultSet> res_exist(pstmt_exist->executeQuery());
		if (!res_exist->next()) {
			return 1;
		}

		con->_con->setAutoCommit(false);
		std::unique_ptr<sql::PreparedStatement> pstmt_del(con->_con->prepareStatement(
			"DELETE FROM post_like WHERE post_id = ? AND uid = ?"));
		pstmt_del->setInt(1, post_id);
		pstmt_del->setInt(2, uid);
		pstmt_del->executeUpdate();

		std::unique_ptr<sql::PreparedStatement> pstmt_up(con->_con->prepareStatement(
			"UPDATE post SET like_cnt = GREATEST(like_cnt - 1, 0) WHERE post_id = ?"));
		pstmt_up->setInt(1, post_id);
		pstmt_up->executeUpdate();
		con->_con->commit();
		return 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException UnlikePost: " << e.what() << std::endl;
		try { con->_con->rollback(); } catch (...) {}
		return -1;
	}
}

int PostDao::CollectPost(int uid, int post_id) {
	auto con = _pool->getConnection();
	if (con == nullptr) {
		return -1;
	}
	Defer defer([this, &con]() {
		_pool->returnConnection(std::move(con));
		});
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt_check(con->_con->prepareStatement(
			"SELECT status FROM post WHERE post_id = ?"));
		pstmt_check->setInt(1, post_id);
		std::unique_ptr<sql::ResultSet> res(pstmt_check->executeQuery());
		if (!res->next() || res->getInt("status") != 1) {
			return 2;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt_exist(con->_con->prepareStatement(
			"SELECT 1 FROM post_collect WHERE post_id = ? AND uid = ? LIMIT 1"));
		pstmt_exist->setInt(1, post_id);
		pstmt_exist->setInt(2, uid);
		std::unique_ptr<sql::ResultSet> res_exist(pstmt_exist->executeQuery());
		if (res_exist->next()) {
			return 1;
		}

		con->_con->setAutoCommit(false);
		std::unique_ptr<sql::PreparedStatement> pstmt_insert(con->_con->prepareStatement(
			"INSERT INTO post_collect(post_id, uid) VALUES(?, ?)"));
		pstmt_insert->setInt(1, post_id);
		pstmt_insert->setInt(2, uid);
		pstmt_insert->executeUpdate();

		std::unique_ptr<sql::PreparedStatement> pstmt_up(con->_con->prepareStatement(
			"UPDATE post SET collect_cnt = collect_cnt + 1 WHERE post_id = ?"));
		pstmt_up->setInt(1, post_id);
		pstmt_up->executeUpdate();
		con->_con->commit();
		return 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException CollectPost: " << e.what() << std::endl;
		try { con->_con->rollback(); } catch (...) {}
		return -1;
	}
}

int PostDao::UncollectPost(int uid, int post_id) {
	auto con = _pool->getConnection();
	if (con == nullptr) {
		return -1;
	}
	Defer defer([this, &con]() {
		_pool->returnConnection(std::move(con));
		});
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt_check(con->_con->prepareStatement(
			"SELECT status FROM post WHERE post_id = ?"));
		pstmt_check->setInt(1, post_id);
		std::unique_ptr<sql::ResultSet> res(pstmt_check->executeQuery());
		if (!res->next() || res->getInt("status") != 1) {
			return 2;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt_exist(con->_con->prepareStatement(
			"SELECT 1 FROM post_collect WHERE post_id = ? AND uid = ? LIMIT 1"));
		pstmt_exist->setInt(1, post_id);
		pstmt_exist->setInt(2, uid);
		std::unique_ptr<sql::ResultSet> res_exist(pstmt_exist->executeQuery());
		if (!res_exist->next()) {
			return 1;
		}

		con->_con->setAutoCommit(false);
		std::unique_ptr<sql::PreparedStatement> pstmt_del(con->_con->prepareStatement(
			"DELETE FROM post_collect WHERE post_id = ? AND uid = ?"));
		pstmt_del->setInt(1, post_id);
		pstmt_del->setInt(2, uid);
		pstmt_del->executeUpdate();

		std::unique_ptr<sql::PreparedStatement> pstmt_up(con->_con->prepareStatement(
			"UPDATE post SET collect_cnt = GREATEST(collect_cnt - 1, 0) WHERE post_id = ?"));
		pstmt_up->setInt(1, post_id);
		pstmt_up->executeUpdate();
		con->_con->commit();
		return 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException UncollectPost: " << e.what() << std::endl;
		try { con->_con->rollback(); } catch (...) {}
		return -1;
	}
}

bool PostDao::IsLiked(int uid, int post_id, bool& liked) {
	auto con = _pool->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		_pool->returnConnection(std::move(con));
		});
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT 1 FROM post_like WHERE post_id = ? AND uid = ? LIMIT 1"));
		pstmt->setInt(1, post_id);
		pstmt->setInt(2, uid);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		liked = res->next();
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException IsLiked: " << e.what() << std::endl;
		return false;
	}
}

bool PostDao::IsCollected(int uid, int post_id, bool& collected) {
	auto con = _pool->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		_pool->returnConnection(std::move(con));
		});
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT 1 FROM post_collect WHERE post_id = ? AND uid = ? LIMIT 1"));
		pstmt->setInt(1, post_id);
		pstmt->setInt(2, uid);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		collected = res->next();
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException IsCollected: " << e.what() << std::endl;
		return false;
	}
}

bool PostDao::SetTop(int post_id, bool is_top) {
	auto con = _pool->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		_pool->returnConnection(std::move(con));
		});
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"UPDATE post SET is_top = ? WHERE post_id = ? AND status = 1"));
		pstmt->setInt(1, is_top ? 1 : 0);
		pstmt->setInt(2, post_id);
		return pstmt->executeUpdate() > 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException SetTop: " << e.what() << std::endl;
		return false;
	}
}

bool PostDao::SetEssence(int post_id, bool is_essence) {
	auto con = _pool->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		_pool->returnConnection(std::move(con));
		});
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"UPDATE post SET is_essence = ? WHERE post_id = ? AND status = 1"));
		pstmt->setInt(1, is_essence ? 1 : 0);
		pstmt->setInt(2, post_id);
		return pstmt->executeUpdate() > 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException SetEssence: " << e.what() << std::endl;
		return false;
	}
}
