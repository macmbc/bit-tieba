#pragma once
#include "const.h"
#include "MysqlDao.h"
#include <vector>
#include <unordered_set>
#include <string>

struct ReplyInfo {
	long long reply_id;
	int post_id;
	int uid;
	std::string content;
	long long parent_reply_id;
	long long root_reply_id;
	int floor;
	int like_cnt;
	std::string created_at;
	std::string author;
};

class ReplyDao {
public:
	ReplyDao(MySqlPool* pool);

	bool ListTopReplies(int post_id, int page, int limit, const std::string& sort, std::vector<ReplyInfo>& replies);
	bool ListChildren(int post_id, const std::vector<long long>& root_ids, std::unordered_map<long long, std::vector<ReplyInfo>>& children);
	bool CreateReply(int uid, int post_id, const std::string& content, long long parent_reply_id,
		long long& reply_id, long long& root_reply_id, int& floor);
	bool GetReplyOwner(long long reply_id, int& owner_uid, int& post_id, long long& parent_reply_id, long long& root_reply_id);
	bool DeleteReply(long long reply_id, int post_id);
	// return 0 success,1 already liked,2 reply missing/disabled,-1 db error
	int LikeReply(int uid, long long reply_id);
	int UnlikeReply(int uid, long long reply_id);
	bool BatchLiked(int uid, const std::vector<long long>& reply_ids, std::unordered_set<long long>& liked_set);
private:
	MySqlPool* _pool;
};
