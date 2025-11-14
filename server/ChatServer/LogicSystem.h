#pragma once
#include "Singleton.h"
#include <queue>
#include <thread>
#include "CSession.h"
#include <queue>
#include <map>
#include <functional>
#include "const.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <unordered_map>
#include "data.h"
#include "message.pb.h"

class CServer;
typedef  function<void(shared_ptr<CSession>, const short &msg_id, const string &msg_data)> FunCallBack;
class LogicSystem:public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem();
	void PostMsgToQue(shared_ptr < LogicNode> msg);
	void SetServer(std::shared_ptr<CServer> pserver);
private:
	LogicSystem();
	void DealMsg();
	void RegisterCallBacks();
	void LoginHandler(shared_ptr<CSession> session, const short &msg_id, const string &msg_data);
	void SearchInfo(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void AddFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void AuthFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void DealChatTextMsg(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void HeartBeatHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	
	// 贴吧相关消息处理函数
	void CreateTiebaHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void SearchTiebaHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void JoinTiebaHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void LeaveTiebaHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void CreatePostHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void CreateReplyHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void GetPostListHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void GetReplyListHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void DeletePostHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void DeleteReplyHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void LikeHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void SetTopHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void SetEssenceHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void GetTiebaInfoHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void GetMyTiebaListHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void GetTiebaMemberListHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	
	// 辅助函数
	bool isPureDigit(const std::string& str);
	void GetUserByUid(std::string uid_str, Json::Value& rtvalue);
	void GetUserByName(std::string name, Json::Value& rtvalue);
	bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo> &userinfo);
	bool GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>>& list);
	bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>> & user_list);
	
	std::thread _worker_thread;
	std::queue<shared_ptr<LogicNode>> _msg_que;
	std::mutex _mutex;
	std::condition_variable _consume;
	bool _b_stop;
	std::map<short, FunCallBack> _fun_callbacks;
	std::shared_ptr<CServer> _p_server;
};

