#pragma once
#include "const.h"
#include "MysqlDao.h"
#include "PostDao.h"
#include "ReplyDao.h"
#include "SearchDao.h"
#include <vector>

struct ReplyWithPost {
	ReplyInfo reply;
	std::string post_title;
};

class UserContentDao {
public:
	UserContentDao(MySqlPool* pool);

	bool ListMyPosts(int uid, int page, int limit, std::vector<PostSummary>& posts);
	bool ListMyReplies(int uid, int page, int limit, std::vector<ReplyWithPost>& replies);
	bool ListMyCollections(int uid, int page, int limit, std::vector<PostSummary>& posts);
	bool ListMyFollowers(int uid, int page, int limit, std::vector<UserBrief>& users);
	bool ListMyFollowing(int uid, int page, int limit, std::vector<UserBrief>& users);
private:
	MySqlPool* _pool;
};
