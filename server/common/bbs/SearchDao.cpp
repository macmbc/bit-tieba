#include "SearchDao.h"

SearchDao::SearchDao(MySqlPool* pool) : _pool(pool) {}

bool SearchDao::SearchPosts(const std::string& keyword, int forum_id, int page, int limit, std::vector<PostSummary>& posts) {
	auto con = _pool->getConnection();
	if (con == nullptr) return false;
	Defer defer([this, &con]() { _pool->returnConnection(std::move(con)); });

	try {
		auto offset = (page - 1) * limit;
		std::unique_ptr<sql::PreparedStatement> pstmt;
		if (forum_id > 0) {
			pstmt.reset(con->_con->prepareStatement(
				"SELECT p.post_id,p.forum_id,p.uid,p.title,p.created_at,p.reply_cnt,p.like_cnt,p.is_top,p.is_essence,"
				"SUBSTRING(p.content,1,120) AS preview, IFNULL(u.nick,u.name) AS author, f.name AS forum_name "
				"FROM post p LEFT JOIN user u ON p.uid=u.uid "
				"LEFT JOIN forum f ON p.forum_id = f.forum_id "
				"WHERE p.status = 1 AND p.forum_id = ? AND (p.title LIKE ? OR p.content LIKE ?) "
				"ORDER BY p.created_at DESC LIMIT ? OFFSET ?"));
			pstmt->setInt(1, forum_id);
			pstmt->setString(2, "%" + keyword + "%");
			pstmt->setString(3, "%" + keyword + "%");
			pstmt->setInt(4, limit);
			pstmt->setInt(5, offset);
		}
		else {
			pstmt.reset(con->_con->prepareStatement(
				"SELECT p.post_id,p.forum_id,p.uid,p.title,p.created_at,p.reply_cnt,p.like_cnt,p.is_top,p.is_essence,"
				"SUBSTRING(p.content,1,120) AS preview, IFNULL(u.nick,u.name) AS author, f.name AS forum_name "
				"FROM post p LEFT JOIN user u ON p.uid=u.uid "
				"LEFT JOIN forum f ON p.forum_id = f.forum_id "
				"WHERE p.status = 1 AND (p.title LIKE ? OR p.content LIKE ?) "
				"ORDER BY p.created_at DESC LIMIT ? OFFSET ?"));
			pstmt->setString(1, "%" + keyword + "%");
			pstmt->setString(2, "%" + keyword + "%");
			pstmt->setInt(3, limit);
			pstmt->setInt(4, offset);
		}
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
		std::cerr << "SQLException SearchPosts: " << e.what() << std::endl;
		return false;
	}
}

bool SearchDao::SearchForums(const std::string& keyword, int page, int limit, std::vector<ForumInfo>& forums) {
	auto con = _pool->getConnection();
	if (con == nullptr) return false;
	Defer defer([this, &con]() { _pool->returnConnection(std::move(con)); });

	try {
		auto offset = (page - 1) * limit;
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT forum_id,name,avatar,description,follower_cnt,post_cnt,owner_uid "
			"FROM forum WHERE name LIKE ? OR description LIKE ? "
			"ORDER BY follower_cnt DESC, post_cnt DESC LIMIT ? OFFSET ?"));
		pstmt->setString(1, "%" + keyword + "%");
		pstmt->setString(2, "%" + keyword + "%");
		pstmt->setInt(3, limit);
		pstmt->setInt(4, offset);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			ForumInfo info;
			info.forum_id = res->getInt("forum_id");
			info.name = res->getString("name");
			info.avatar = res->getString("avatar");
			info.description = res->getString("description");
			info.follower_count = res->getInt("follower_cnt");
			info.post_count = res->getInt("post_cnt");
			info.owner_uid = res->getInt("owner_uid");
			forums.push_back(info);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException SearchForums: " << e.what() << std::endl;
		return false;
	}
}

bool SearchDao::SearchUsers(const std::string& keyword, int page, int limit, std::vector<UserBrief>& users) {
	auto con = _pool->getConnection();
	if (con == nullptr) return false;
	Defer defer([this, &con]() { _pool->returnConnection(std::move(con)); });

	try {
		auto offset = (page - 1) * limit;
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT uid,name,nick,email,icon,sex FROM user WHERE name LIKE ? OR nick LIKE ? OR email LIKE ? "
			"ORDER BY uid DESC LIMIT ? OFFSET ?"));
		pstmt->setString(1, "%" + keyword + "%");
		pstmt->setString(2, "%" + keyword + "%");
		pstmt->setString(3, "%" + keyword + "%");
		pstmt->setInt(4, limit);
		pstmt->setInt(5, offset);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			UserBrief u;
			u.uid = res->getInt("uid");
			u.name = res->getString("name");
			u.nick = res->getString("nick");
			u.email = res->getString("email");
			u.icon = res->getString("icon");
			u.sex = res->getInt("sex");
			users.push_back(u);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException SearchUsers: " << e.what() << std::endl;
		return false;
	}
}
