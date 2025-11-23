#pragma once
#include "const.h"
#include "MysqlDao.h"
#include "ForumDao.h"
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
private:
	MysqlMgr();
	MysqlDao  _dao;
	std::unique_ptr<ForumDao> _forum_dao;
};
