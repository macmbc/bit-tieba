#include "UserContentDao.h"

UserContentDao::UserContentDao(MySqlPool* pool) : _pool(pool) {}

bool UserContentDao::ListMyPosts(int uid, int page, int limit, std::vector<PostSummary>& posts) {
	auto con = _pool->getConnection();
	if (con == nullptr) return false;
	Defer defer([this, &con]() { _pool->returnConnection(std::move(con)); });
	try {
		auto offset = (page - 1) * limit;
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT p.post_id,p.forum_id,p.uid,p.title,p.created_at,p.reply_cnt,p.like_cnt,p.is_top,p.is_essence,"
			"SUBSTRING(p.content,1,120) AS preview, IFNULL(u.nick,u.name) AS author, f.name AS forum_name "
			"FROM post p LEFT JOIN user u ON p.uid = u.uid "
			"LEFT JOIN forum f ON p.forum_id = f.forum_id "
			"WHERE p.status = 1 AND p.uid = ? ORDER BY p.created_at DESC LIMIT ? OFFSET ?"));
		pstmt->setInt(1, uid);
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
			ps.forum_name = res->getString("forum_name");
			posts.push_back(ps);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException ListMyPosts: " << e.what() << std::endl;
		return false;
	}
}

bool UserContentDao::ListMyReplies(int uid, int page, int limit, std::vector<ReplyWithPost>& replies) {
	auto con = _pool->getConnection();
	if (con == nullptr) return false;
	Defer defer([this, &con]() { _pool->returnConnection(std::move(con)); });
	try {
		auto offset = (page - 1) * limit;
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT r.reply_id,r.post_id,r.uid,r.content,r.parent_reply_id,r.root_reply_id,"
			"r.floor,r.like_cnt,r.created_at,p.title AS post_title "
			"FROM reply r JOIN post p ON r.post_id = p.post_id "
			"WHERE r.status = 1 AND p.status = 1 AND r.uid = ? "
			"ORDER BY r.created_at DESC LIMIT ? OFFSET ?"));
		pstmt->setInt(1, uid);
		pstmt->setInt(2, limit);
		pstmt->setInt(3, offset);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			ReplyWithPost rp;
			rp.reply.reply_id = res->getInt64("reply_id");
			rp.reply.post_id = res->getInt("post_id");
			rp.reply.uid = res->getInt("uid");
			rp.reply.content = res->getString("content");
			rp.reply.parent_reply_id = res->getInt64("parent_reply_id");
			rp.reply.root_reply_id = res->getInt64("root_reply_id");
			rp.reply.floor = res->getInt("floor");
			rp.reply.like_cnt = res->getInt("like_cnt");
			rp.reply.created_at = res->getString("created_at");
			rp.post_title = res->getString("post_title");
			replies.push_back(rp);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException ListMyReplies: " << e.what() << std::endl;
		return false;
	}
}

bool UserContentDao::ListMyCollections(int uid, int page, int limit, std::vector<PostSummary>& posts) {
	auto con = _pool->getConnection();
	if (con == nullptr) return false;
	Defer defer([this, &con]() { _pool->returnConnection(std::move(con)); });
	try {
		auto offset = (page - 1) * limit;
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT p.post_id,p.forum_id,p.uid,p.title,p.created_at,p.reply_cnt,p.like_cnt,p.is_top,p.is_essence,"
			"SUBSTRING(p.content,1,120) AS preview, IFNULL(u.nick,u.name) AS author, f.name AS forum_name "
			"FROM post_collect pc JOIN post p ON pc.post_id = p.post_id "
			"LEFT JOIN user u ON p.uid = u.uid "
			"LEFT JOIN forum f ON p.forum_id = f.forum_id "
			"WHERE p.status = 1 AND pc.uid = ? ORDER BY pc.id DESC LIMIT ? OFFSET ?"));
		pstmt->setInt(1, uid);
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
			ps.forum_name = res->getString("forum_name");
			posts.push_back(ps);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException ListMyCollections: " << e.what() << std::endl;
		return false;
	}
}

bool UserContentDao::ListMyFollowers(int uid, int page, int limit, std::vector<UserBrief>& users) {
	auto con = _pool->getConnection();
	if (con == nullptr) return false;
	Defer defer([this, &con]() { _pool->returnConnection(std::move(con)); });
	try {
		auto offset = (page - 1) * limit;
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT u.uid,u.name,u.nick,u.email,u.icon,u.sex "
			"FROM user_follow uf JOIN user u ON uf.uid = u.uid "
			"WHERE uf.target_uid = ? ORDER BY uf.id DESC LIMIT ? OFFSET ?"));
		pstmt->setInt(1, uid);
		pstmt->setInt(2, limit);
		pstmt->setInt(3, offset);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			UserBrief ub;
			ub.uid = res->getInt("uid");
			ub.name = res->getString("name");
			ub.nick = res->getString("nick");
			ub.email = res->getString("email");
			ub.icon = res->getString("icon");
			ub.sex = res->getInt("sex");
			users.push_back(ub);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException ListMyFollowers: " << e.what() << std::endl;
		return false;
	}
}

bool UserContentDao::ListMyFollowing(int uid, int page, int limit, std::vector<UserBrief>& users) {
	auto con = _pool->getConnection();
	if (con == nullptr) return false;
	Defer defer([this, &con]() { _pool->returnConnection(std::move(con)); });
	try {
		auto offset = (page - 1) * limit;
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT u.uid,u.name,u.nick,u.email,u.icon,u.sex "
			"FROM user_follow uf JOIN user u ON uf.target_uid = u.uid "
			"WHERE uf.uid = ? ORDER BY uf.id DESC LIMIT ? OFFSET ?"));
		pstmt->setInt(1, uid);
		pstmt->setInt(2, limit);
		pstmt->setInt(3, offset);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			UserBrief ub;
			ub.uid = res->getInt("uid");
			ub.name = res->getString("name");
			ub.nick = res->getString("nick");
			ub.email = res->getString("email");
			ub.icon = res->getString("icon");
			ub.sex = res->getInt("sex");
			users.push_back(ub);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException ListMyFollowing: " << e.what() << std::endl;
		return false;
	}
}
