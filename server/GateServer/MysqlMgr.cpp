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
}

