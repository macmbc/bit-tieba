#pragma once
#include "const.h"
#include "MysqlDao.h"

struct ForumInfo {
	int forum_id;
	std::string name;
	std::string avatar;
	std::string description;
	int follower_count;
	int post_count;
	int owner_uid;
};

class ForumDao {
public:
	ForumDao(MySqlPool* pool);

	bool ListForums(int page, int limit, std::vector<ForumInfo>& forums);
	bool GetForum(int forum_id, ForumInfo& info);
	bool IsFollowed(int uid, int forum_id, bool& followed);
	// 返回 0 成功, 1 已关注, 2 贴吧不存在, -1 失败
	int FollowForum(int uid, int forum_id);
	// 返回 0 成功, 1 未关注, 2 贴吧不存在, -1 失败
	int UnfollowForum(int uid, int forum_id);
	bool ListFollowedForums(int uid, int page, int limit, std::vector<ForumInfo>& forums);
private:
	MySqlPool* _pool;
};
