#include "ForumDao.h"

ForumDao::ForumDao(MySqlPool* pool) : _pool(pool) {}

bool ForumDao::ListForums(int page, int limit, std::vector<ForumInfo>& forums) {
	auto con = _pool->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		_pool->returnConnection(std::move(con));
		});

	try {
		auto offset = (page - 1) * limit;
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT f.forum_id,f.name,f.avatar,f.description,f.follower_cnt,f.post_cnt,f.owner_uid "
			"FROM forum f ORDER BY f.forum_id LIMIT ? OFFSET ?"));
		pstmt->setInt(1, limit);
		pstmt->setInt(2, offset);
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
		std::cerr << "SQLException ListForums: " << e.what() << std::endl;
		return false;
	}
}

bool ForumDao::GetForum(int forum_id, ForumInfo& info) {
	auto con = _pool->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		_pool->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT f.forum_id,f.name,f.avatar,f.description,f.follower_cnt,f.post_cnt,f.owner_uid "
			"FROM forum f WHERE f.forum_id = ?"));
		pstmt->setInt(1, forum_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (!res->next()) {
			return false;
		}
		info.forum_id = res->getInt("forum_id");
		info.name = res->getString("name");
		info.avatar = res->getString("avatar");
		info.description = res->getString("description");
		info.follower_count = res->getInt("follower_cnt");
		info.post_count = res->getInt("post_cnt");
		info.owner_uid = res->getInt("owner_uid");
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException GetForum: " << e.what() << std::endl;
		return false;
	}
}

bool ForumDao::IsFollowed(int uid, int forum_id, bool& followed) {
	auto con = _pool->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		_pool->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT 1 FROM forum_follow WHERE forum_id = ? AND uid = ? LIMIT 1"));
		pstmt->setInt(1, forum_id);
		pstmt->setInt(2, uid);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		followed = res->next();
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException IsFollowed: " << e.what() << std::endl;
		return false;
	}
}

int ForumDao::FollowForum(int uid, int forum_id) {
	auto con = _pool->getConnection();
	if (con == nullptr) {
		return -1;
	}
	Defer defer([this, &con]() {
		_pool->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt_forum(con->_con->prepareStatement(
			"SELECT 1 FROM forum WHERE forum_id = ?"));
		pstmt_forum->setInt(1, forum_id);
		std::unique_ptr<sql::ResultSet> res_forum(pstmt_forum->executeQuery());
		if (!res_forum->next()) {
			return 2;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt_exist(con->_con->prepareStatement(
			"SELECT 1 FROM forum_follow WHERE forum_id = ? AND uid = ? LIMIT 1"));
		pstmt_exist->setInt(1, forum_id);
		pstmt_exist->setInt(2, uid);
		std::unique_ptr<sql::ResultSet> res(pstmt_exist->executeQuery());
		if (res->next()) {
			return 1;
		}

		con->_con->setAutoCommit(false);
		std::unique_ptr<sql::PreparedStatement> pstmt_insert(con->_con->prepareStatement(
			"INSERT INTO forum_follow(forum_id, uid) VALUES(?, ?)"));
		pstmt_insert->setInt(1, forum_id);
		pstmt_insert->setInt(2, uid);
		pstmt_insert->executeUpdate();

		std::unique_ptr<sql::PreparedStatement> pstmt_update(con->_con->prepareStatement(
			"UPDATE forum SET follower_cnt = follower_cnt + 1 WHERE forum_id = ?"));
		pstmt_update->setInt(1, forum_id);
		pstmt_update->executeUpdate();
		con->_con->commit();
		return 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException FollowForum: " << e.what() << std::endl;
		try {
			con->_con->rollback();
		}
		catch (...) {}
		return -1;
	}
}

int ForumDao::UnfollowForum(int uid, int forum_id) {
	auto con = _pool->getConnection();
	if (con == nullptr) {
		return -1;
	}
	Defer defer([this, &con]() {
		_pool->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt_forum(con->_con->prepareStatement(
			"SELECT 1 FROM forum WHERE forum_id = ?"));
		pstmt_forum->setInt(1, forum_id);
		std::unique_ptr<sql::ResultSet> res_forum(pstmt_forum->executeQuery());
		if (!res_forum->next()) {
			return 2;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt_exist(con->_con->prepareStatement(
			"SELECT 1 FROM forum_follow WHERE forum_id = ? AND uid = ? LIMIT 1"));
		pstmt_exist->setInt(1, forum_id);
		pstmt_exist->setInt(2, uid);
		std::unique_ptr<sql::ResultSet> res(pstmt_exist->executeQuery());
		if (!res->next()) {
			return 1;
		}

		con->_con->setAutoCommit(false);
		std::unique_ptr<sql::PreparedStatement> pstmt_del(con->_con->prepareStatement(
			"DELETE FROM forum_follow WHERE forum_id = ? AND uid = ?"));
		pstmt_del->setInt(1, forum_id);
		pstmt_del->setInt(2, uid);
		pstmt_del->executeUpdate();

		std::unique_ptr<sql::PreparedStatement> pstmt_update(con->_con->prepareStatement(
			"UPDATE forum SET follower_cnt = GREATEST(follower_cnt - 1, 0) WHERE forum_id = ?"));
		pstmt_update->setInt(1, forum_id);
		pstmt_update->executeUpdate();
		con->_con->commit();
		return 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException UnfollowForum: " << e.what() << std::endl;
		try {
			con->_con->rollback();
		}
		catch (...) {}
		return -1;
	}
}

bool ForumDao::ListFollowedForums(int uid, int page, int limit, std::vector<ForumInfo>& forums) {
	auto con = _pool->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		_pool->returnConnection(std::move(con));
		});

	try {
		auto offset = (page - 1) * limit;
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT f.forum_id,f.name,f.avatar,f.description,f.follower_cnt,f.post_cnt,f.owner_uid "
			"FROM forum_follow ff JOIN forum f ON ff.forum_id = f.forum_id "
			"WHERE ff.uid = ? ORDER BY ff.id DESC LIMIT ? OFFSET ?"));
		pstmt->setInt(1, uid);
		pstmt->setInt(2, limit);
		pstmt->setInt(3, offset);
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
		std::cerr << "SQLException ListFollowedForums: " << e.what() << std::endl;
		return false;
	}
}
