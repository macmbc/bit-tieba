#pragma once
#include "const.h"
#include "MysqlDao.h"
#include "PostDao.h"
#include "ForumDao.h"
#include <vector>

struct UserBrief {
	int uid;
	std::string name;
	std::string nick;
	std::string email;
	std::string icon;
	int sex;
};

class SearchDao {
public:
	SearchDao(MySqlPool* pool);

	bool SearchPosts(const std::string& keyword, int forum_id, int page, int limit, std::vector<PostSummary>& posts);
	bool SearchForums(const std::string& keyword, int page, int limit, std::vector<ForumInfo>& forums);
	bool SearchUsers(const std::string& keyword, int page, int limit, std::vector<UserBrief>& users);
private:
	MySqlPool* _pool;
};
