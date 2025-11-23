#include "MysqlMgr.h"


MysqlMgr::~MysqlMgr() {

}

int MysqlMgr::RegUser(const std::string& name, const std::string& email, const std::string& pwd, const std::string& icon)
{
	return _dao.RegUserTransaction(name, email, pwd, icon);
}

bool MysqlMgr::CheckEmail(const std::string& name, const std::string& email) {
	return _dao.CheckEmail(name, email);
}

bool MysqlMgr::UpdatePwd(const std::string& name, const std::string& pwd) {
	return _dao.UpdatePwd(name, pwd);
}

bool MysqlMgr::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
	return _dao.CheckPwd(email, pwd, userInfo);
}

bool MysqlMgr::TestProcedure(const std::string& email, int& uid, string& name) {
	return _dao.TestProcedure(email,uid, name);
}

bool MysqlMgr::ListForums(int page, int limit, std::vector<ForumInfo>& forums) {
	return _forum_dao->ListForums(page, limit, forums);
}

bool MysqlMgr::GetForum(int forum_id, ForumInfo& info) {
	return _forum_dao->GetForum(forum_id, info);
}

bool MysqlMgr::IsFollowed(int uid, int forum_id, bool& followed) {
	return _forum_dao->IsFollowed(uid, forum_id, followed);
}

int MysqlMgr::FollowForum(int uid, int forum_id) {
	return _forum_dao->FollowForum(uid, forum_id);
}

int MysqlMgr::UnfollowForum(int uid, int forum_id) {
	return _forum_dao->UnfollowForum(uid, forum_id);
}

bool MysqlMgr::ListFollowedForums(int uid, int page, int limit, std::vector<ForumInfo>& forums) {
	return _forum_dao->ListFollowedForums(uid, page, limit, forums);
}

MysqlMgr::MysqlMgr() {
	_forum_dao = std::make_unique<ForumDao>(_dao.GetPool());
	_post_dao = std::make_unique<PostDao>(_dao.GetPool());
	_reply_dao = std::make_unique<ReplyDao>(_dao.GetPool());
	_search_dao = std::make_unique<SearchDao>(_dao.GetPool());
	_user_content_dao = std::make_unique<UserContentDao>(_dao.GetPool());
}

bool MysqlMgr::ListPosts(int forum_id, int page, int limit, const std::string& sort, std::vector<PostSummary>& posts) {
	return _post_dao->ListPosts(forum_id, page, limit, sort, posts);
}

bool MysqlMgr::GetPostDetail(int post_id, PostDetail& detail) {
	return _post_dao->GetPostDetail(post_id, detail);
}

bool MysqlMgr::GetPostOwner(int post_id, int& owner_uid, int& forum_id) {
	return _post_dao->GetPostOwner(post_id, owner_uid, forum_id);
}

bool MysqlMgr::CreatePost(int uid, int forum_id, const std::string& title, const std::string& content, int& post_id) {
	return _post_dao->CreatePost(uid, forum_id, title, content, post_id);
}

bool MysqlMgr::UpdatePost(int post_id, int uid, const std::string& title, const std::string& content) {
	return _post_dao->UpdatePost(post_id, uid, title, content);
}

bool MysqlMgr::DeletePost(int post_id, int forum_id) {
	return _post_dao->DeletePost(post_id, forum_id);
}

int MysqlMgr::LikePost(int uid, int post_id) {
	return _post_dao->LikePost(uid, post_id);
}

int MysqlMgr::UnlikePost(int uid, int post_id) {
	return _post_dao->UnlikePost(uid, post_id);
}

int MysqlMgr::CollectPost(int uid, int post_id) {
	return _post_dao->CollectPost(uid, post_id);
}

int MysqlMgr::UncollectPost(int uid, int post_id) {
	return _post_dao->UncollectPost(uid, post_id);
}

bool MysqlMgr::IsLiked(int uid, int post_id, bool& liked) {
	return _post_dao->IsLiked(uid, post_id, liked);
}

bool MysqlMgr::IsCollected(int uid, int post_id, bool& collected) {
	return _post_dao->IsCollected(uid, post_id, collected);
}

bool MysqlMgr::SetTop(int post_id, bool is_top) {
	return _post_dao->SetTop(post_id, is_top);
}

bool MysqlMgr::SetEssence(int post_id, bool is_essence) {
	return _post_dao->SetEssence(post_id, is_essence);
}

bool MysqlMgr::ListTopReplies(int post_id, int page, int limit, const std::string& sort, std::vector<ReplyInfo>& replies) {
	return _reply_dao->ListTopReplies(post_id, page, limit, sort, replies);
}

bool MysqlMgr::ListChildrenReplies(int post_id, const std::vector<long long>& root_ids, std::unordered_map<long long, std::vector<ReplyInfo>>& children) {
	return _reply_dao->ListChildren(post_id, root_ids, children);
}

bool MysqlMgr::CreateReply(int uid, int post_id, const std::string& content, long long parent_reply_id,
	long long& reply_id, long long& root_reply_id, int& floor) {
	return _reply_dao->CreateReply(uid, post_id, content, parent_reply_id, reply_id, root_reply_id, floor);
}

bool MysqlMgr::GetReplyOwner(long long reply_id, int& owner_uid, int& post_id, long long& parent_reply_id, long long& root_reply_id) {
	return _reply_dao->GetReplyOwner(reply_id, owner_uid, post_id, parent_reply_id, root_reply_id);
}

bool MysqlMgr::DeleteReply(long long reply_id, int post_id) {
	return _reply_dao->DeleteReply(reply_id, post_id);
}

int MysqlMgr::LikeReply(int uid, long long reply_id) {
	return _reply_dao->LikeReply(uid, reply_id);
}

int MysqlMgr::UnlikeReply(int uid, long long reply_id) {
	return _reply_dao->UnlikeReply(uid, reply_id);
}

bool MysqlMgr::BatchReplyLiked(int uid, const std::vector<long long>& reply_ids, std::unordered_set<long long>& liked_set) {
	return _reply_dao->BatchLiked(uid, reply_ids, liked_set);
}

bool MysqlMgr::SearchPosts(const std::string& keyword, int forum_id, int page, int limit, std::vector<PostSummary>& posts) {
	return _search_dao->SearchPosts(keyword, forum_id, page, limit, posts);
}

bool MysqlMgr::SearchForums(const std::string& keyword, int page, int limit, std::vector<ForumInfo>& forums) {
	return _search_dao->SearchForums(keyword, page, limit, forums);
}

bool MysqlMgr::SearchUsers(const std::string& keyword, int page, int limit, std::vector<UserBrief>& users) {
	return _search_dao->SearchUsers(keyword, page, limit, users);
}

bool MysqlMgr::ListMyPosts(int uid, int page, int limit, std::vector<PostSummary>& posts) {
	return _user_content_dao->ListMyPosts(uid, page, limit, posts);
}

bool MysqlMgr::ListMyReplies(int uid, int page, int limit, std::vector<ReplyWithPost>& replies) {
	return _user_content_dao->ListMyReplies(uid, page, limit, replies);
}

bool MysqlMgr::ListMyCollections(int uid, int page, int limit, std::vector<PostSummary>& posts) {
	return _user_content_dao->ListMyCollections(uid, page, limit, posts);
}

bool MysqlMgr::ListMyFollowers(int uid, int page, int limit, std::vector<UserBrief>& users) {
	return _user_content_dao->ListMyFollowers(uid, page, limit, users);
}

bool MysqlMgr::ListMyFollowing(int uid, int page, int limit, std::vector<UserBrief>& users) {
	return _user_content_dao->ListMyFollowing(uid, page, limit, users);
}
