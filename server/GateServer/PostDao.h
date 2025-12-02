#pragma once
#include "const.h"
#include "MysqlDao.h"

struct PostSummary {
	int post_id;
	int forum_id;
	int uid;
	std::string title;
	std::string author;
	std::string created_at;
	int reply_cnt;
	int like_cnt;
	bool is_top;
	bool is_essence;
	std::string content_preview;
	std::string forum_name;
};

struct PostDetail {
	int post_id;
	int forum_id;
	int uid;
	std::string title;
	std::string content;
	std::string author;
	std::string created_at;
	int reply_cnt;
	int like_cnt;
	int collect_cnt;
	bool is_top;
	bool is_essence;
	std::string forum_name;
};

class PostDao {
public:
	PostDao(MySqlPool* pool);

	bool ListPosts(int forum_id, int page, int limit, const std::string& sort, std::vector<PostSummary>& posts);
	bool GetPostDetail(int post_id, PostDetail& detail);
	bool GetPostOwner(int post_id, int& owner_uid, int& forum_id);
	bool CreatePost(int uid, int forum_id, const std::string& title, const std::string& content, int& post_id);
	bool UpdatePost(int post_id, int uid, const std::string& title, const std::string& content);
	bool DeletePost(int post_id, int forum_id);
	// return: 0 success, 1 already liked, 2 post missing/disabled, -1 db error
	int LikePost(int uid, int post_id);
	int UnlikePost(int uid, int post_id);
	int CollectPost(int uid, int post_id);
	int UncollectPost(int uid, int post_id);
	bool IsLiked(int uid, int post_id, bool& liked);
	bool IsCollected(int uid, int post_id, bool& collected);
	bool SetTop(int post_id, bool is_top);
	bool SetEssence(int post_id, bool is_essence);
private:
	MySqlPool* _pool;
};
