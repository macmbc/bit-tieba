#pragma once
#include "const.h"
#include "MysqlDao.h"
#include "ForumDao.h"
#include "PostDao.h"
#include "ReplyDao.h"
class MysqlMgr: public Singleton<MysqlMgr>
{
	friend class Singleton<MysqlMgr>;
public:
	~MysqlMgr();

	int RegUser(const std::string& name, const std::string& email,  const std::string& pwd, const std::string& icon);
	bool CheckEmail(const std::string& name, const std::string & email);
	bool UpdatePwd(const std::string& name, const std::string& email);
	bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);
	bool TestProcedure(const std::string &email, int& uid, string & name);
	// forum
	bool ListForums(int page, int limit, std::vector<ForumInfo>& forums);
	bool GetForum(int forum_id, ForumInfo& info);
	bool IsFollowed(int uid, int forum_id, bool& followed);
	int FollowForum(int uid, int forum_id);
	int UnfollowForum(int uid, int forum_id);
	bool ListFollowedForums(int uid, int page, int limit, std::vector<ForumInfo>& forums);
	// post
	bool ListPosts(int forum_id, int page, int limit, const std::string& sort, std::vector<PostSummary>& posts);
	bool GetPostDetail(int post_id, PostDetail& detail);
	bool GetPostOwner(int post_id, int& owner_uid, int& forum_id);
	bool CreatePost(int uid, int forum_id, const std::string& title, const std::string& content, int& post_id);
	bool UpdatePost(int post_id, int uid, const std::string& title, const std::string& content);
	bool DeletePost(int post_id, int forum_id);
	int LikePost(int uid, int post_id);
	int UnlikePost(int uid, int post_id);
	int CollectPost(int uid, int post_id);
	int UncollectPost(int uid, int post_id);
	bool IsLiked(int uid, int post_id, bool& liked);
	bool IsCollected(int uid, int post_id, bool& collected);
	bool SetTop(int post_id, bool is_top);
	bool SetEssence(int post_id, bool is_essence);
	// reply
	bool ListTopReplies(int post_id, int page, int limit, const std::string& sort, std::vector<ReplyInfo>& replies);
	bool ListChildrenReplies(int post_id, const std::vector<long long>& root_ids, std::unordered_map<long long, std::vector<ReplyInfo>>& children);
	bool CreateReply(int uid, int post_id, const std::string& content, long long parent_reply_id,
		long long& reply_id, long long& root_reply_id, int& floor);
	bool GetReplyOwner(long long reply_id, int& owner_uid, int& post_id, long long& parent_reply_id, long long& root_reply_id);
	bool DeleteReply(long long reply_id, int post_id);
	int LikeReply(int uid, long long reply_id);
	int UnlikeReply(int uid, long long reply_id);
	bool BatchReplyLiked(int uid, const std::vector<long long>& reply_ids, std::unordered_set<long long>& liked_set);
private:
	MysqlMgr();
	MysqlDao  _dao;
	std::unique_ptr<ForumDao> _forum_dao;
	std::unique_ptr<PostDao> _post_dao;
	std::unique_ptr<ReplyDao> _reply_dao;
};
