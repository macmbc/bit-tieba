#include "LogicSystem.h"
#include "StatusGrpcClient.h"
#include "MysqlMgr.h"
#include "const.h"
#include "RedisMgr.h"
#include "UserMgr.h"
#include "ChatGrpcClient.h"
#include "DistLock.h"
#include <string>
#include "CServer.h"
using namespace std;

LogicSystem::LogicSystem():_b_stop(false), _p_server(nullptr){
	RegisterCallBacks();
	_worker_thread = std::thread (&LogicSystem::DealMsg, this);
}

LogicSystem::~LogicSystem(){
	_b_stop = true;
	_consume.notify_one();
	_worker_thread.join();
}

void LogicSystem::PostMsgToQue(shared_ptr < LogicNode> msg) {
	std::unique_lock<std::mutex> unique_lk(_mutex);
	_msg_que.push(msg);
	//由0变为1则发送通知信号
	if (_msg_que.size() == 1) {
		unique_lk.unlock();
		_consume.notify_one();
	}
}


void LogicSystem::SetServer(std::shared_ptr<CServer> pserver) {
	_p_server = pserver;
}


void LogicSystem::DealMsg() {
	for (;;) {
		std::unique_lock<std::mutex> unique_lk(_mutex);
		//判断队列为空则用条件变量阻塞等待，并释放锁
		while (_msg_que.empty() && !_b_stop) {
			_consume.wait(unique_lk);
		}

		//判断是否为关闭状态，把所有逻辑执行完后则退出循环
		if (_b_stop ) {
			while (!_msg_que.empty()) {
				auto msg_node = _msg_que.front();
				cout << "recv_msg id  is " << msg_node->_recvnode->_msg_id << endl;
				auto call_back_iter = _fun_callbacks.find(msg_node->_recvnode->_msg_id);
				if (call_back_iter == _fun_callbacks.end()) {
					_msg_que.pop();
					continue;
				}
				call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id,
					std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
				_msg_que.pop();
			}
			break;
		}

		//如果没有停服，且说明队列中有数据
		auto msg_node = _msg_que.front();
		cout << "recv_msg id  is " << msg_node->_recvnode->_msg_id << endl;
		auto call_back_iter = _fun_callbacks.find(msg_node->_recvnode->_msg_id);
		if (call_back_iter == _fun_callbacks.end()) {
			_msg_que.pop();
			std::cout << "msg id [" << msg_node->_recvnode->_msg_id << "] handler not found" << std::endl;
			continue;
		}
		call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id, 
			std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
		_msg_que.pop();
	}
}

void LogicSystem::RegisterCallBacks() {
	_fun_callbacks[MSG_CHAT_LOGIN] = std::bind(&LogicSystem::LoginHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_SEARCH_USER_REQ] = std::bind(&LogicSystem::SearchInfo, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_ADD_FRIEND_REQ] = std::bind(&LogicSystem::AddFriendApply, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_AUTH_FRIEND_REQ] = std::bind(&LogicSystem::AuthFriendApply, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_TEXT_CHAT_MSG_REQ] = std::bind(&LogicSystem::DealChatTextMsg, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_HEART_BEAT_REQ] = std::bind(&LogicSystem::HeartBeatHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
	
	// 注册贴吧相关消息处理函数
	_fun_callbacks[ID_CREATE_TIEBA_REQ] = std::bind(&LogicSystem::CreateTiebaHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
	
	_fun_callbacks[ID_SEARCH_TIEBA_REQ] = std::bind(&LogicSystem::SearchTiebaHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
	
	_fun_callbacks[ID_JOIN_TIEBA_REQ] = std::bind(&LogicSystem::JoinTiebaHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
	
	_fun_callbacks[ID_LEAVE_TIEBA_REQ] = std::bind(&LogicSystem::LeaveTiebaHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
	
	_fun_callbacks[ID_CREATE_POST_REQ] = std::bind(&LogicSystem::CreatePostHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
	
	_fun_callbacks[ID_CREATE_REPLY_REQ] = std::bind(&LogicSystem::CreateReplyHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
	
	_fun_callbacks[ID_GET_POST_LIST_REQ] = std::bind(&LogicSystem::GetPostListHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
	
	_fun_callbacks[ID_GET_REPLY_LIST_REQ] = std::bind(&LogicSystem::GetReplyListHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
	
	_fun_callbacks[ID_DELETE_POST_REQ] = std::bind(&LogicSystem::DeletePostHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
	
	_fun_callbacks[ID_DELETE_REPLY_REQ] = std::bind(&LogicSystem::DeleteReplyHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
	
	_fun_callbacks[ID_UPDATE_TIEBA_REQ] = std::bind(&LogicSystem::UpdateTiebaHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
	
	_fun_callbacks[ID_LIKE_REQ] = std::bind(&LogicSystem::LikeHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
	
	_fun_callbacks[ID_SET_TOP_REQ] = std::bind(&LogicSystem::SetTopHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
	
	_fun_callbacks[ID_SET_ESSENCE_REQ] = std::bind(&LogicSystem::SetEssenceHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
	
	_fun_callbacks[ID_GET_TIEBA_INFO_REQ] = std::bind(&LogicSystem::GetTiebaInfoHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
	
	_fun_callbacks[ID_GET_MY_TIEBA_LIST_REQ] = std::bind(&LogicSystem::GetMyTiebaListHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
	
	_fun_callbacks[ID_GET_TIEBA_MEMBER_LIST_REQ] = std::bind(&LogicSystem::GetTiebaMemberListHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
	
	_fun_callbacks[ID_SET_FOLLOWED_TIEBA_REQ] = std::bind(&LogicSystem::SetFollowedTiebaHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
	
	_fun_callbacks[ID_GET_FOLLOWED_TIEBA_LIST_REQ] = std::bind(&LogicSystem::GetFollowedTiebaListHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
	
	_fun_callbacks[ID_SET_COLLECTED_POST_REQ] = std::bind(&LogicSystem::SetCollectedPostHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
}
void LogicSystem::LoginHandler(shared_ptr<CSession> session, const short &msg_id, const string &msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto token = root["token"].asString();
	std::cout << "user login uid is  " << uid << " user token  is "
		<< token << endl;

	Json::Value  rtvalue;
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, MSG_CHAT_LOGIN_RSP);
		});


	//从redis获取用户token是否正确
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	std::string token_value = "";
	bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
	if (!success) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return ;
	}

	if (token_value != token) {
		rtvalue["error"] = ErrorCodes::TokenInvalid;
		return ;
	}

	rtvalue["error"] = ErrorCodes::Success;


	std::string base_key = USER_BASE_INFO + uid_str;
	auto user_info = std::make_shared<UserInfo>();
	bool b_base = GetBaseInfo(base_key, uid, user_info);
	if (!b_base) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}
	rtvalue["uid"] = uid;
	rtvalue["pwd"] = user_info->pwd;
	rtvalue["name"] = user_info->name;
	rtvalue["email"] = user_info->email;
	rtvalue["nick"] = user_info->nick;
	rtvalue["desc"] = user_info->desc;
	rtvalue["sex"] = user_info->sex;
	rtvalue["icon"] = user_info->icon;

	//从数据库获取申请列表
	std::vector<std::shared_ptr<ApplyInfo>> apply_list;
	auto b_apply = GetFriendApplyInfo(uid, apply_list);
	if (b_apply) {
		for (auto& apply : apply_list) {
			Json::Value obj;
			obj["name"] = apply->_name;
			obj["uid"] = apply->_uid;
			obj["icon"] = apply->_icon;
			obj["nick"] = apply->_nick;
			obj["sex"] = apply->_sex;
			obj["desc"] = apply->_desc;
			obj["status"] = apply->_status;
			rtvalue["apply_list"].append(obj);
		}
	}

	//获取好友列表
	std::vector<std::shared_ptr<UserInfo>> friend_list;
	bool b_friend_list = GetFriendList(uid, friend_list);
	for (auto& friend_ele : friend_list) {
		Json::Value obj;
		obj["name"] = friend_ele->name;
		obj["uid"] = friend_ele->uid;
		obj["icon"] = friend_ele->icon;
		obj["nick"] = friend_ele->nick;
		obj["sex"] = friend_ele->sex;
		obj["desc"] = friend_ele->desc;
		obj["back"] = friend_ele->back;
		rtvalue["friend_list"].append(obj);
	}

	auto server_name = ConfigMgr::Inst().GetValue("SelfServer", "Name");
	{
		//此处添加分布式锁，让该线程独占登录
		//拼接用户ip对应的key
		auto lock_key = LOCK_PREFIX + uid_str;
		auto identifier = RedisMgr::GetInstance()->acquireLock(lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
		//利用defer解锁
		Defer defer2([this, identifier, lock_key]() {
			RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
			});
		//此处判断该用户是否在别处或者本服务器登录

		std::string uid_ip_value = "";
		auto uid_ip_key = USERIPPREFIX + uid_str;
		bool b_ip = RedisMgr::GetInstance()->Get(uid_ip_key, uid_ip_value);
		//说明用户已经登录了，此处应该踢掉之前的用户登录状态
		if (b_ip) {
			//获取当前服务器ip信息
			auto& cfg = ConfigMgr::Inst();
			auto self_name = cfg["SelfServer"]["Name"];
			//如果之前登录的服务器和当前相同，则直接在本服务器踢掉
			if (uid_ip_value == self_name) {
				//查找旧有的连接
				auto old_session = UserMgr::GetInstance()->GetSession(uid);

				//此处应该发送踢人消息
				if (old_session) {
					old_session->NotifyOffline(uid);
					//清除旧的连接
					_p_server->ClearSession(old_session->GetSessionId());
				}

			}
			else {
				//如果不是本服务器，则通知grpc通知其他服务器踢掉
				//发送通知
				KickUserReq kick_req;
				kick_req.set_uid(uid);
				ChatGrpcClient::GetInstance()->NotifyKickUser(uid_ip_value, kick_req);
			}
		}

		//session绑定用户uid
		session->SetUserId(uid);
		//为用户设置登录ip server的名字
		std::string  ipkey = USERIPPREFIX + uid_str;
		RedisMgr::GetInstance()->Set(ipkey, server_name);
		//uid和session绑定管理,方便以后踢人操作
		UserMgr::GetInstance()->SetUserSession(uid, session);
		std::string  uid_session_key = USER_SESSION_PREFIX + uid_str;
		RedisMgr::GetInstance()->Set(uid_session_key, session->GetSessionId());

	}

	return;
}

void LogicSystem::SearchInfo(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid_str = root["uid"].asString();
	std::cout << "user SearchInfo uid is  " << uid_str << endl;

	Json::Value  rtvalue;

	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_SEARCH_USER_RSP);
		});

	bool b_digit = isPureDigit(uid_str);
	if (b_digit) {
		GetUserByUid(uid_str, rtvalue);
	}
	else {
		GetUserByName(uid_str, rtvalue);
	}
	return;
}

void LogicSystem::AddFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto applyname = root["applyname"].asString();
	auto bakname = root["bakname"].asString();
	auto touid = root["touid"].asInt();
	std::cout << "user login uid is  " << uid << " applyname  is "
		<< applyname << " bakname is " << bakname << " touid is " << touid << endl;

	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_ADD_FRIEND_RSP);
		});

	//先更新数据库
	MysqlMgr::GetInstance()->AddFriendApply(uid, touid);

	//查询redis 查找touid对应的server ip
	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIPPREFIX + to_str;
	std::string to_ip_value = "";
	bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
	if (!b_ip) {
		return;
	}


	auto& cfg = ConfigMgr::Inst();
	auto self_name = cfg["SelfServer"]["Name"];


	std::string base_key = USER_BASE_INFO + std::to_string(uid);
	auto apply_info = std::make_shared<UserInfo>();
	bool b_info = GetBaseInfo(base_key, uid, apply_info);

	//直接通知对方有申请消息
	if (to_ip_value == self_name) {
		auto session = UserMgr::GetInstance()->GetSession(touid);
		if (session) {
			//在内存中则直接发送通知对方
			Json::Value  notify;
			notify["error"] = ErrorCodes::Success;
			notify["applyuid"] = uid;
			notify["name"] = applyname;
			notify["desc"] = "";
			if (b_info) {
				notify["icon"] = apply_info->icon;
				notify["sex"] = apply_info->sex;
				notify["nick"] = apply_info->nick;
			}
			std::string return_str = notify.toStyledString();
			session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);
		}

		return ;
	}

	
	AddFriendReq add_req;
	add_req.set_applyuid(uid);
	add_req.set_touid(touid);
	add_req.set_name(applyname);
	add_req.set_desc("");
	if (b_info) {
		add_req.set_icon(apply_info->icon);
		add_req.set_sex(apply_info->sex);
		add_req.set_nick(apply_info->nick);
	}

	//发送通知
	ChatGrpcClient::GetInstance()->NotifyAddFriend(to_ip_value,add_req);

}

void LogicSystem::AuthFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);

	auto uid = root["fromuid"].asInt();
	auto touid = root["touid"].asInt();
	auto back_name = root["back"].asString();
	std::cout << "from " << uid << " auth friend to " << touid << std::endl;

	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	auto user_info = std::make_shared<UserInfo>();

	std::string base_key = USER_BASE_INFO + std::to_string(touid);
	bool b_info = GetBaseInfo(base_key, touid, user_info);
	if (b_info) {
		rtvalue["name"] = user_info->name;
		rtvalue["nick"] = user_info->nick;
		rtvalue["icon"] = user_info->icon;
		rtvalue["sex"] = user_info->sex;
		rtvalue["uid"] = touid;
	}
	else {
		rtvalue["error"] = ErrorCodes::UidInvalid;
	}


	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_AUTH_FRIEND_RSP);
		});

	//先更新数据库
	MysqlMgr::GetInstance()->AuthFriendApply(uid, touid);

	//更新数据库添加好友
	MysqlMgr::GetInstance()->AddFriend(uid, touid,back_name);

	//查询redis 查找touid对应的server ip
	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIPPREFIX + to_str;
	std::string to_ip_value = "";
	bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
	if (!b_ip) {
		return;
	}

	auto& cfg = ConfigMgr::Inst();
	auto self_name = cfg["SelfServer"]["Name"];
	//直接通知对方有认证通过消息
	if (to_ip_value == self_name) {
		auto session = UserMgr::GetInstance()->GetSession(touid);
		if (session) {
			//在内存中则直接发送通知对方
			Json::Value  notify;
			notify["error"] = ErrorCodes::Success;
			notify["fromuid"] = uid;
			notify["touid"] = touid;
			std::string base_key = USER_BASE_INFO + std::to_string(uid);
			auto user_info = std::make_shared<UserInfo>();
			bool b_info = GetBaseInfo(base_key, uid, user_info);
			if (b_info) {
				notify["name"] = user_info->name;
				notify["nick"] = user_info->nick;
				notify["icon"] = user_info->icon;
				notify["sex"] = user_info->sex;
			}
			else {
				notify["error"] = ErrorCodes::UidInvalid;
			}


			std::string return_str = notify.toStyledString();
			session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
		}

		return ;
	}


	AuthFriendReq auth_req;
	auth_req.set_fromuid(uid);
	auth_req.set_touid(touid);

	//发送通知
	ChatGrpcClient::GetInstance()->NotifyAuthFriend(to_ip_value, auth_req);
}

void LogicSystem::DealChatTextMsg(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);

	auto uid = root["fromuid"].asInt();
	auto touid = root["touid"].asInt();

	const Json::Value  arrays = root["text_array"];
	
	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["text_array"] = arrays;
	rtvalue["fromuid"] = uid;
	rtvalue["touid"] = touid;

	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_TEXT_CHAT_MSG_RSP);
		});


	//查询redis 查找touid对应的server ip
	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIPPREFIX + to_str;
	std::string to_ip_value = "";
	bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
	if (!b_ip) {
		return;
	}

	auto& cfg = ConfigMgr::Inst();
	auto self_name = cfg["SelfServer"]["Name"];
	//直接通知对方有认证通过消息
	if (to_ip_value == self_name) {
		auto session = UserMgr::GetInstance()->GetSession(touid);
		if (session) {
			//在内存中则直接发送通知对方
			std::string return_str = rtvalue.toStyledString();
			session->Send(return_str, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
		}

		return ;
	}


	TextChatMsgReq text_msg_req;
	text_msg_req.set_fromuid(uid);
	text_msg_req.set_touid(touid);
	for (const auto& txt_obj : arrays) {
		auto content = txt_obj["content"].asString();
		auto msgid = txt_obj["msgid"].asString();
		std::cout << "content is " << content << std::endl;
		std::cout << "msgid is " << msgid << std::endl;
		auto *text_msg = text_msg_req.add_textmsgs();
		text_msg->set_msgid(msgid);
		text_msg->set_msgcontent(content);
	}


	//发送通知 todo...
	ChatGrpcClient::GetInstance()->NotifyTextChatMsg(to_ip_value, text_msg_req, rtvalue);
}

void LogicSystem::HeartBeatHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["fromuid"].asInt();
	std::cout << "receive heart beat msg, uid is " << uid << std::endl;
	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	session->Send(rtvalue.toStyledString(), ID_HEARTBEAT_RSP);
}

bool LogicSystem::isPureDigit(const std::string& str)
{
	for (char c : str) {
		if (!std::isdigit(c)) {
			return false;
		}
	}
	return true;
}

void LogicSystem::GetUserByUid(std::string uid_str, Json::Value& rtvalue)
{
	rtvalue["error"] = ErrorCodes::Success;

	std::string base_key = USER_BASE_INFO + uid_str;

	//优先查redis中查询用户信息
	std::string info_str = "";
	bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
	if (b_base) {
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		auto uid = root["uid"].asInt();
		auto name = root["name"].asString();
		auto pwd = root["pwd"].asString();
		auto email = root["email"].asString();
		auto nick = root["nick"].asString();
		auto desc = root["desc"].asString();
		auto sex = root["sex"].asInt();
		auto icon = root["icon"].asString();
		std::cout << "user  uid is  " << uid << " name  is "
			<< name << " pwd is " << pwd << " email is " << email <<" icon is " << icon << endl;

		rtvalue["uid"] = uid;
		rtvalue["pwd"] = pwd;
		rtvalue["name"] = name;
		rtvalue["email"] = email;
		rtvalue["nick"] = nick;
		rtvalue["desc"] = desc;
		rtvalue["sex"] = sex;
		rtvalue["icon"] = icon;
		return;
	}

	auto uid = std::stoi(uid_str);
	//redis中没有则查询mysql
	//查询数据库
	std::shared_ptr<UserInfo> user_info = nullptr;
	user_info = MysqlMgr::GetInstance()->GetUser(uid);
	if (user_info == nullptr) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}

	//将数据库内容写入redis缓存
	Json::Value redis_root;
	redis_root["uid"] = user_info->uid;
	redis_root["pwd"] = user_info->pwd;
	redis_root["name"] = user_info->name;
	redis_root["email"] = user_info->email;
	redis_root["nick"] = user_info->nick;
	redis_root["desc"] = user_info->desc;
	redis_root["sex"] = user_info->sex;
	redis_root["icon"] = user_info->icon;

	RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());

	//返回数据
	rtvalue["uid"] = user_info->uid;
	rtvalue["pwd"] = user_info->pwd;
	rtvalue["name"] = user_info->name;
	rtvalue["email"] = user_info->email;
	rtvalue["nick"] = user_info->nick;
	rtvalue["desc"] = user_info->desc;
	rtvalue["sex"] = user_info->sex;
	rtvalue["icon"] = user_info->icon;
}

void LogicSystem::GetUserByName(std::string name, Json::Value& rtvalue)
{
	rtvalue["error"] = ErrorCodes::Success;

	std::string base_key = NAME_INFO + name;

	//优先查redis中查询用户信息
	std::string info_str = "";
	bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
	if (b_base) {
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		auto uid = root["uid"].asInt();
		auto name = root["name"].asString();
		auto pwd = root["pwd"].asString();
		auto email = root["email"].asString();
		auto nick = root["nick"].asString();
		auto desc = root["desc"].asString();
		auto sex = root["sex"].asInt();
		std::cout << "user  uid is  " << uid << " name  is "
			<< name << " pwd is " << pwd << " email is " << email << endl;

		rtvalue["uid"] = uid;
		rtvalue["pwd"] = pwd;
		rtvalue["name"] = name;
		rtvalue["email"] = email;
		rtvalue["nick"] = nick;
		rtvalue["desc"] = desc;
		rtvalue["sex"] = sex;
		return;
	}

	//redis中没有则查询mysql
	//查询数据库
	std::shared_ptr<UserInfo> user_info = nullptr;
	user_info = MysqlMgr::GetInstance()->GetUser(name);
	if (user_info == nullptr) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}

	//将数据库内容写入redis缓存
	Json::Value redis_root;
	redis_root["uid"] = user_info->uid;
	redis_root["pwd"] = user_info->pwd;
	redis_root["name"] = user_info->name;
	redis_root["email"] = user_info->email;
	redis_root["nick"] = user_info->nick;
	redis_root["desc"] = user_info->desc;
	redis_root["sex"] = user_info->sex;

	RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
	
	//返回数据
	rtvalue["uid"] = user_info->uid;
	rtvalue["pwd"] = user_info->pwd;
	rtvalue["name"] = user_info->name;
	rtvalue["email"] = user_info->email;
	rtvalue["nick"] = user_info->nick;
	rtvalue["desc"] = user_info->desc;
	rtvalue["sex"] = user_info->sex;
}

bool LogicSystem::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
{
	//优先查redis中查询用户信息
	std::string info_str = "";
	bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
	if (b_base) {
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		userinfo->uid = root["uid"].asInt();
		userinfo->name = root["name"].asString();
		userinfo->pwd = root["pwd"].asString();
		userinfo->email = root["email"].asString();
		userinfo->nick = root["nick"].asString();
		userinfo->desc = root["desc"].asString();
		userinfo->sex = root["sex"].asInt();
		userinfo->icon = root["icon"].asString();
		std::cout << "user login uid is  " << userinfo->uid << " name  is "
			<< userinfo->name << " pwd is " << userinfo->pwd << " email is " << userinfo->email << endl;
	}
	else {
		//redis中没有则查询mysql
		//查询数据库
		std::shared_ptr<UserInfo> user_info = nullptr;
		user_info = MysqlMgr::GetInstance()->GetUser(uid);
		if (user_info == nullptr) {
			return false;
		}

		userinfo = user_info;

		//将数据库内容写入redis缓存
		Json::Value redis_root;
		redis_root["uid"] = uid;
		redis_root["pwd"] = userinfo->pwd;
		redis_root["name"] = userinfo->name;
		redis_root["email"] = userinfo->email;
		redis_root["nick"] = userinfo->nick;
		redis_root["desc"] = userinfo->desc;
		redis_root["sex"] = userinfo->sex;
		redis_root["icon"] = userinfo->icon;
		RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
	}

	return true;
}

bool LogicSystem::GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>> &list) {
	//从mysql获取好友申请列表
	return MysqlMgr::GetInstance()->GetApplyList(to_uid, list, 0, 10);
}

bool LogicSystem::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_list) {
	//从mysql获取好友列表
	return MysqlMgr::GetInstance()->GetFriendList(self_id, user_list);
}

// ==================== 贴吧相关消息处理函数实现 ====================

// 创建贴吧
void LogicSystem::CreateTiebaHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	CreateTiebaReq req;
	req.ParseFromString(msg_data);
	
	std::cout << "CreateTieba: uid=" << req.uid() << ", name=" << req.tieba_name() << std::endl;
	
	CreateTiebaRsp rsp;
	Defer defer([this, &rsp, session]() {
		std::string return_str;
		rsp.SerializeToString(&return_str);
		session->Send(return_str, ID_CREATE_TIEBA_RSP);
	});
	
	// 检查贴吧名称是否有效
	if (req.tieba_name().empty() || req.tieba_name().length() > 50) {
		rsp.set_error(ErrorCodes::TiebaNameInvalid);
		return;
	}
	
	// 创建贴吧
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	int tieba_id = tieba_dao.CreateTieba(req.uid(), req.tieba_name(), req.tieba_desc(), req.tieba_icon());
	
	if (tieba_id == 0) {
		rsp.set_error(ErrorCodes::TiebaExist);
		return;
	} else if (tieba_id < 0) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return;
	}
	
	rsp.set_error(ErrorCodes::Success);
	rsp.set_tieba_id(tieba_id);
	rsp.set_tieba_name(req.tieba_name());
}

// 搜索贴吧
void LogicSystem::SearchTiebaHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	SearchTiebaReq req;
	req.ParseFromString(msg_data);
	
	std::cout << "SearchTieba: keyword=" << req.keyword() << ", offset=" << req.offset() << ", limit=" << req.limit() << std::endl;
	
	SearchTiebaRsp rsp;
	Defer defer([this, &rsp, session]() {
		std::string return_str;
		rsp.SerializeToString(&return_str);
		session->Send(return_str, ID_SEARCH_TIEBA_RSP);
	});
	
	int limit = req.limit();
	if (limit <= 0 || limit > 100) {
		limit = 20;
	}
	
	std::vector<std::shared_ptr<TiebaInfo>> tieba_list;
	int total = 0;
	
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.SearchTieba(req.keyword(), req.offset(), limit, tieba_list, total);
	
	if (!success) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return;
	}
	
	rsp.set_error(ErrorCodes::Success);
	rsp.set_total(total);
	
	for (auto& tieba : tieba_list) {
		TiebaInfo* tieba_info = rsp.add_tieba_list();
		tieba_info->set_tieba_id(tieba->tieba_id);
		tieba_info->set_tieba_name(tieba->tieba_name);
		tieba_info->set_tieba_desc(tieba->tieba_desc);
		tieba_info->set_tieba_icon(tieba->tieba_icon);
		tieba_info->set_owner_uid(tieba->owner_uid);
		tieba_info->set_member_count(tieba->member_count);
		tieba_info->set_post_count(tieba->post_count);
		tieba_info->set_create_time(tieba->create_time);
	}
}

// 加入贴吧
void LogicSystem::JoinTiebaHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	JoinTiebaReq req;
	req.ParseFromString(msg_data);
	
	std::cout << "JoinTieba: uid=" << req.uid() << ", tieba_id=" << req.tieba_id() << std::endl;
	
	JoinTiebaRsp rsp;
	Defer defer([this, &rsp, session]() {
		std::string return_str;
		rsp.SerializeToString(&return_str);
		session->Send(return_str, ID_JOIN_TIEBA_RSP);
	});
	
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	
	// 检查贴吧是否存在
	if (!tieba_dao.TiebaExists(req.tieba_id())) {
		rsp.set_error(ErrorCodes::TiebaNotExist);
		return;
	}
	
	// 加入贴吧
	bool success = tieba_dao.JoinTieba(req.uid(), req.tieba_id());
	
	if (!success) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return;
	}
	
	rsp.set_error(ErrorCodes::Success);
	rsp.set_tieba_id(req.tieba_id());
}

// 退出贴吧
void LogicSystem::LeaveTiebaHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	LeaveTiebaReq req;
	req.ParseFromString(msg_data);
	
	std::cout << "LeaveTieba: uid=" << req.uid() << ", tieba_id=" << req.tieba_id() << std::endl;
	
	LeaveTiebaRsp rsp;
	Defer defer([this, &rsp, session]() {
		std::string return_str;
		rsp.SerializeToString(&return_str);
		session->Send(return_str, ID_LEAVE_TIEBA_RSP);
	});
	
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	
	// 退出贴吧
	bool success = tieba_dao.LeaveTieba(req.uid(), req.tieba_id());
	
	if (!success) {
		rsp.set_error(ErrorCodes::PermissionDenied);
		return;
	}
	
	rsp.set_error(ErrorCodes::Success);
	rsp.set_tieba_id(req.tieba_id());
}

// 发帖
void LogicSystem::CreatePostHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	CreatePostReq req;
	req.ParseFromString(msg_data);
	
	std::cout << "CreatePost: uid=" << req.uid() << ", tieba_id=" << req.tieba_id() << ", title=" << req.title() << std::endl;
	
	CreatePostRsp rsp;
	Defer defer([this, &rsp, session]() {
		std::string return_str;
		rsp.SerializeToString(&return_str);
		session->Send(return_str, ID_CREATE_POST_RSP);
	});
	
	// 检查标题和内容
	if (req.title().empty() || req.title().length() > 200) {
		rsp.set_error(ErrorCodes::Error_Json);
		return;
	}
	
	if (req.content().empty() || req.content().length() > 10000) {
		rsp.set_error(ErrorCodes::Error_Json);
		return;
	}
	
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	int post_id = tieba_dao.CreatePost(req.uid(), req.tieba_id(), req.title(), req.content());
	
	if (post_id < 0) {
		rsp.set_error(ErrorCodes::NotTiebaMember);
		return;
	}
	
	rsp.set_error(ErrorCodes::Success);
	rsp.set_post_id(post_id);
	rsp.set_tieba_id(req.tieba_id());
}

// 回复
void LogicSystem::CreateReplyHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	CreateReplyReq req;
	req.ParseFromString(msg_data);
	
	std::cout << "CreateReply: uid=" << req.uid() << ", post_id=" << req.post_id() << std::endl;
	
	CreateReplyRsp rsp;
	Defer defer([this, &rsp, session]() {
		std::string return_str;
		rsp.SerializeToString(&return_str);
		session->Send(return_str, ID_CREATE_REPLY_RSP);
	});
	
	// 检查内容
	if (req.content().empty() || req.content().length() > 5000) {
		rsp.set_error(ErrorCodes::Error_Json);
		return;
	}
	
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	int reply_id = tieba_dao.CreateReply(req.uid(), req.post_id(), req.content(), req.reply_to_uid(), req.reply_to_reply_id());
	
	if (reply_id < 0) {
		rsp.set_error(ErrorCodes::PostNotExist);
		return;
	}
	
	rsp.set_error(ErrorCodes::Success);
	rsp.set_reply_id(reply_id);
	rsp.set_post_id(req.post_id());
}

// 获取帖子列表
void LogicSystem::GetPostListHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	GetPostListReq req;
	req.ParseFromString(msg_data);
	
	std::cout << "GetPostList: tieba_id=" << req.tieba_id() << ", offset=" << req.offset() << ", limit=" << req.limit() << std::endl;
	
	GetPostListRsp rsp;
	Defer defer([this, &rsp, session]() {
		std::string return_str;
		rsp.SerializeToString(&return_str);
		session->Send(return_str, ID_GET_POST_LIST_RSP);
	});
	
	int limit = req.limit();
	if (limit <= 0 || limit > 100) {
		limit = 20;
	}
	
	std::vector<std::shared_ptr<PostInfo>> post_list;
	int total = 0;
	
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.GetPostList(req.uid(), req.tieba_id(), req.offset(), limit, req.sort_type(), post_list, total);
	
	if (!success) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return;
	}
	
	rsp.set_error(ErrorCodes::Success);
	rsp.set_total(total);
	
	for (auto& post : post_list) {
		PostInfo* post_info = rsp.add_post_list();
		post_info->set_post_id(post->post_id);
		post_info->set_tieba_id(post->tieba_id);
		post_info->set_uid(post->uid);
		post_info->set_user_name(post->user_name);
		post_info->set_user_icon(post->user_icon);
		post_info->set_title(post->title);
		post_info->set_content(post->content);
		post_info->set_reply_count(post->reply_count);
		post_info->set_like_count(post->like_count);
		post_info->set_is_top(post->is_top);
		post_info->set_is_essence(post->is_essence);
		post_info->set_create_time(post->create_time);
		post_info->set_is_liked(post->is_liked);
	}
}

// 获取回复列表
void LogicSystem::GetReplyListHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	GetReplyListReq req;
	req.ParseFromString(msg_data);
	
	std::cout << "GetReplyList: post_id=" << req.post_id() << ", offset=" << req.offset() << ", limit=" << req.limit() << std::endl;
	
	GetReplyListRsp rsp;
	Defer defer([this, &rsp, session]() {
		std::string return_str;
		rsp.SerializeToString(&return_str);
		session->Send(return_str, ID_GET_REPLY_LIST_RSP);
	});
	
	int limit = req.limit();
	if (limit <= 0 || limit > 100) {
		limit = 20;
	}
	
	// 获取帖子信息
	std::shared_ptr<PostInfo> post_info;
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool b_post = tieba_dao.GetPostInfo(req.uid(), req.post_id(), post_info);
	if (!b_post) {
		rsp.set_error(ErrorCodes::PostNotExist);
		return;
	}
	
	// 获取回复列表
	std::vector<std::shared_ptr<ReplyInfo>> reply_list;
	int total = 0;
	bool success = tieba_dao.GetReplyList(req.uid(), req.post_id(), req.offset(), limit, reply_list, total);
	
	if (!success) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return;
	}
	
	rsp.set_error(ErrorCodes::Success);
	rsp.set_total(total);
	
	// 添加帖子信息
	PostInfo* rsp_post_info = rsp.mutable_post_info();
	rsp_post_info->set_post_id(post_info->post_id);
	rsp_post_info->set_tieba_id(post_info->tieba_id);
	rsp_post_info->set_uid(post_info->uid);
	rsp_post_info->set_user_name(post_info->user_name);
	rsp_post_info->set_user_icon(post_info->user_icon);
	rsp_post_info->set_title(post_info->title);
	rsp_post_info->set_content(post_info->content);
	rsp_post_info->set_reply_count(post_info->reply_count);
	rsp_post_info->set_like_count(post_info->like_count);
	rsp_post_info->set_is_top(post_info->is_top);
	rsp_post_info->set_is_essence(post_info->is_essence);
	rsp_post_info->set_create_time(post_info->create_time);
	rsp_post_info->set_is_liked(post_info->is_liked);
	
	// 添加回复列表
	for (auto& reply : reply_list) {
		ReplyInfo* reply_info = rsp.add_reply_list();
		reply_info->set_reply_id(reply->reply_id);
		reply_info->set_post_id(reply->post_id);
		reply_info->set_uid(reply->uid);
		reply_info->set_user_name(reply->user_name);
		reply_info->set_user_icon(reply->user_icon);
		reply_info->set_content(reply->content);
		reply_info->set_reply_to_uid(reply->reply_to_uid);
		reply_info->set_reply_to_user_name(reply->reply_to_user_name);
		reply_info->set_reply_to_reply_id(reply->reply_to_reply_id);
		reply_info->set_like_count(reply->like_count);
		reply_info->set_create_time(reply->create_time);
		reply_info->set_is_liked(reply->is_liked);
	}
}

// 删除帖子
void LogicSystem::DeletePostHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	DeletePostReq req;
	req.ParseFromString(msg_data);
	
	std::cout << "DeletePost: uid=" << req.uid() << ", post_id=" << req.post_id() << ", tieba_id=" << req.tieba_id() << std::endl;
	
	DeletePostRsp rsp;
	Defer defer([this, &rsp, session]() {
		std::string return_str;
		rsp.SerializeToString(&return_str);
		session->Send(return_str, ID_DELETE_POST_RSP);
	});
	
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.DeletePost(req.uid(), req.post_id(), req.tieba_id());
	
	if (!success) {
		rsp.set_error(ErrorCodes::PermissionDenied);
		return;
	}
	
	rsp.set_error(ErrorCodes::Success);
	rsp.set_post_id(req.post_id());
}

// 删除回复
void LogicSystem::DeleteReplyHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	DeleteReplyReq req;
	req.ParseFromString(msg_data);
	
	std::cout << "DeleteReply: uid=" << req.uid() << ", reply_id=" << req.reply_id() << ", post_id=" << req.post_id() << std::endl;
	
	DeleteReplyRsp rsp;
	Defer defer([this, &rsp, session]() {
		std::string return_str;
		rsp.SerializeToString(&return_str);
		session->Send(return_str, ID_DELETE_REPLY_RSP);
	});
	
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.DeleteReply(req.uid(), req.reply_id(), req.post_id());
	
	if (!success) {
		rsp.set_error(ErrorCodes::PermissionDenied);
		return;
	}
	
	rsp.set_error(ErrorCodes::Success);
	rsp.set_reply_id(req.reply_id());
}

// 点赞/取消点赞
void LogicSystem::LikeHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	LikeReq req;
	req.ParseFromString(msg_data);
	
	std::cout << "Like: uid=" << req.uid() << ", target_type=" << req.target_type() << ", target_id=" << req.target_id() << ", is_like=" << req.is_like() << std::endl;
	
	LikeRsp rsp;
	Defer defer([this, &rsp, session]() {
		std::string return_str;
		rsp.SerializeToString(&return_str);
		session->Send(return_str, ID_LIKE_RSP);
	});
	
	int like_count = 0;
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.Like(req.uid(), req.target_type(), req.target_id(), req.is_like(), like_count);
	
	if (!success) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return;
	}
	
	rsp.set_error(ErrorCodes::Success);
	rsp.set_target_type(req.target_type());
	rsp.set_target_id(req.target_id());
	rsp.set_is_like(req.is_like());
	rsp.set_like_count(like_count);
}

// 置顶
void LogicSystem::SetTopHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	SetTopReq req;
	req.ParseFromString(msg_data);
	
	std::cout << "SetTop: uid=" << req.uid() << ", post_id=" << req.post_id() << ", tieba_id=" << req.tieba_id() << ", is_top=" << req.is_top() << std::endl;
	
	SetTopRsp rsp;
	Defer defer([this, &rsp, session]() {
		std::string return_str;
		rsp.SerializeToString(&return_str);
		session->Send(return_str, ID_SET_TOP_RSP);
	});
	
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.SetTop(req.uid(), req.post_id(), req.tieba_id(), req.is_top());
	
	if (!success) {
		rsp.set_error(ErrorCodes::PermissionDenied);
		return;
	}
	
	rsp.set_error(ErrorCodes::Success);
	rsp.set_post_id(req.post_id());
	rsp.set_is_top(req.is_top());
}

// 加精
void LogicSystem::SetEssenceHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	SetEssenceReq req;
	req.ParseFromString(msg_data);
	
	std::cout << "SetEssence: uid=" << req.uid() << ", post_id=" << req.post_id() << ", tieba_id=" << req.tieba_id() << ", is_essence=" << req.is_essence() << std::endl;
	
	SetEssenceRsp rsp;
	Defer defer([this, &rsp, session]() {
		std::string return_str;
		rsp.SerializeToString(&return_str);
		session->Send(return_str, ID_SET_ESSENCE_RSP);
	});
	
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.SetEssence(req.uid(), req.post_id(), req.tieba_id(), req.is_essence());
	
	if (!success) {
		rsp.set_error(ErrorCodes::PermissionDenied);
		return;
	}
	
	rsp.set_error(ErrorCodes::Success);
	rsp.set_post_id(req.post_id());
	rsp.set_is_essence(req.is_essence());
}

// 获取贴吧信息
void LogicSystem::GetTiebaInfoHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	GetTiebaInfoReq req;
	req.ParseFromString(msg_data);
	
	std::cout << "GetTiebaInfo: uid=" << req.uid() << ", tieba_id=" << req.tieba_id() << std::endl;
	
	GetTiebaInfoRsp rsp;
	Defer defer([this, &rsp, session]() {
		std::string return_str;
		rsp.SerializeToString(&return_str);
		session->Send(return_str, ID_GET_TIEBA_INFO_RSP);
	});
	
	std::shared_ptr<TiebaInfo> tieba_info;
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.GetTiebaInfo(req.tieba_id(), tieba_info);
	
	if (!success) {
		rsp.set_error(ErrorCodes::TiebaNotExist);
		return;
	}
	
	rsp.set_error(ErrorCodes::Success);
	
	// 添加贴吧信息
	TiebaInfo* tieba_obj = rsp.mutable_tieba_info();
	tieba_obj->set_tieba_id(tieba_info->tieba_id);
	tieba_obj->set_tieba_name(tieba_info->tieba_name);
	tieba_obj->set_tieba_desc(tieba_info->tieba_desc);
	tieba_obj->set_tieba_icon(tieba_info->tieba_icon);
	tieba_obj->set_owner_uid(tieba_info->owner_uid);
	tieba_obj->set_member_count(tieba_info->member_count);
	tieba_obj->set_post_count(tieba_info->post_count);
	tieba_obj->set_create_time(tieba_info->create_time);
	
	// 检查用户是否为成员
	int role = 0;
	bool is_member = tieba_dao.IsTiebaMember(req.uid(), req.tieba_id(), role);
	rsp.set_is_member(is_member);
	rsp.set_member_role(role);
}

// 获取我的贴吧列表
void LogicSystem::GetMyTiebaListHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	GetMyTiebaListReq req;
	req.ParseFromString(msg_data);
	
	std::cout << "GetMyTiebaList: uid=" << req.uid() << std::endl;
	
	GetMyTiebaListRsp rsp;
	Defer defer([this, &rsp, session]() {
		std::string return_str;
		rsp.SerializeToString(&return_str);
		session->Send(return_str, ID_GET_MY_TIEBA_LIST_RSP);
	});
	
	std::vector<std::shared_ptr<TiebaInfo>> tieba_list;
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.GetMyTiebaList(req.uid(), tieba_list);
	
	if (!success) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return;
	}
	
	rsp.set_error(ErrorCodes::Success);
	
	for (auto& tieba : tieba_list) {
		TiebaInfo* tieba_obj = rsp.add_tieba_list();
		tieba_obj->set_tieba_id(tieba->tieba_id);
		tieba_obj->set_tieba_name(tieba->tieba_name);
		tieba_obj->set_tieba_desc(tieba->tieba_desc);
		tieba_obj->set_tieba_icon(tieba->tieba_icon);
		tieba_obj->set_owner_uid(tieba->owner_uid);
		tieba_obj->set_member_count(tieba->member_count);
		tieba_obj->set_post_count(tieba->post_count);
		tieba_obj->set_create_time(tieba->create_time);
	}
}

// 更新贴吧信息
void LogicSystem::UpdateTiebaHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	UpdateTiebaReq req;
	req.ParseFromString(msg_data);
	
	std::cout << "UpdateTieba: uid=" << req.uid() << ", tieba_id=" << req.tieba_id() << std::endl;
	
	UpdateTiebaRsp rsp;
	Defer defer([this, &rsp, session]() {
		std::string return_str;
		rsp.SerializeToString(&return_str);
		session->Send(return_str, ID_UPDATE_TIEBA_RSP);
	});
	
	// 更新贴吧信息
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.UpdateTiebaInfo(req.uid(), req.tieba_id(), req.desc(), req.icon(), req.new_owner_id());
	
	if (!success) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return;
	}
	
	rsp.set_error(ErrorCodes::Success);
}

// 获取贴吧成员列表
void LogicSystem::GetTiebaMemberListHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	GetTiebaMemberListReq req;
	req.ParseFromString(msg_data);
	
	std::cout << "GetTiebaMemberList: tieba_id=" << req.tieba_id() << ", offset=" << req.offset() << ", limit=" << req.limit() << std::endl;
	
	GetTiebaMemberListRsp rsp;
	Defer defer([this, &rsp, session]() {
		std::string return_str;
		rsp.SerializeToString(&return_str);
		session->Send(return_str, ID_GET_TIEBA_MEMBER_LIST_RSP);
	});
	
	int limit = req.limit();
	if (limit <= 0 || limit > 100) {
		limit = 20;
	}
	
	std::vector<std::shared_ptr<TiebaMemberInfo>> member_list;
	int total = 0;
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.GetTiebaMemberList(req.uid(), req.tieba_id(), req.offset(), limit, member_list, total);
	
	if (!success) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return;
	}
	
	rsp.set_error(ErrorCodes::Success);
	rsp.set_total(total);
	
	for (auto& member : member_list) {
		TiebaMemberInfo* member_obj = rsp.add_member_list();
		member_obj->set_uid(member->uid);
		member_obj->set_user_name(member->user_name);
		member_obj->set_user_icon(member->user_icon);
		member_obj->set_role(member->role);
		member_obj->set_join_time(member->join_time);
	}
}

// 关注/取关贴吧
void LogicSystem::SetFollowedTiebaHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	SetFollowedTiebaReq req;
	req.ParseFromString(msg_data);
	
	std::cout << "SetFollowedTieba: uid=" << req.uid() << ", tieba_id=" << req.tieba_id() 
		<< ", is_followed=" << req.is_followed() << std::endl;
	
	SetFollowedTiebaRsp rsp;
	Defer defer([this, &rsp, session]() {
		std::string return_str;
		rsp.SerializeToString(&return_str);
		session->Send(return_str, ID_SET_FOLLOWED_TIEBA_RSP);
	});
	
	// 关注/取关贴吧
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.SetFollowedTieba(req.uid(), req.tieba_id(), req.is_followed());
	
	if (!success) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return;
	}
	
	rsp.set_error(ErrorCodes::Success);
}

// 获取用户关注的贴吧列表
void LogicSystem::GetFollowedTiebaListHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	GetFollowedTiebaListReq req;
	req.ParseFromString(msg_data);
	
	std::cout << "GetFollowedTiebaList: uid=" << req.uid() << ", offset=" << req.offset() 
		<< ", limit=" << req.limit() << std::endl;
	
	GetFollowedTiebaListRsp rsp;
	Defer defer([this, &rsp, session]() {
		std::string return_str;
		rsp.SerializeToString(&return_str);
		session->Send(return_str, ID_GET_FOLLOWED_TIEBA_LIST_RSP);
	});
	
	if (req.limit() <= 0 || req.limit() > 100) {
		// 使用默认值
		const_cast<GetFollowedTiebaListReq*>(&req)->set_limit(20);
	}
	
	std::vector<std::shared_ptr<TiebaInfo>> tieba_list;
	int total = 0;
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.GetFollowedTiebaList(req.uid(), req.offset(), req.limit(), tieba_list, total);
	
	if (!success) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return;
	}
	
	rsp.set_error(ErrorCodes::Success);
	rsp.set_total(total);
	
	for (auto& tieba : tieba_list) {
		TiebaInfo* tieba_info = rsp.add_tieba_list();
		tieba_info->set_tieba_id(tieba->tieba_id);
		tieba_info->set_tieba_name(tieba->tieba_name);
		tieba_info->set_tieba_desc(tieba->tieba_desc);
		tieba_info->set_tieba_icon(tieba->tieba_icon);
		tieba_info->set_owner_uid(tieba->owner_uid);
		tieba_info->set_member_count(tieba->member_count);
		tieba_info->set_post_count(tieba->post_count);
		tieba_info->set_create_time(tieba->create_time);
	}
}

// 收藏/取消收藏帖子
void LogicSystem::SetCollectedPostHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	SetCollectedPostReq req;
	req.ParseFromString(msg_data);
	
	std::cout << "SetCollectedPost: uid=" << req.uid() << ", post_id=" << req.post_id() 
		<< ", is_collected=" << req.is_collected() << std::endl;
	
	SetCollectedPostRsp rsp;
	Defer defer([this, &rsp, session]() {
		std::string return_str;
		rsp.SerializeToString(&return_str);
		session->Send(return_str, ID_SET_COLLECTED_POST_RSP);
	});
	
	// 收藏/取消收藏帖子
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.SetCollectedPost(req.uid(), req.post_id(), req.is_collected());
	
	if (!success) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return;
	}
	
	rsp.set_error(ErrorCodes::Success);
	rsp.set_post_id(req.post_id());
	rsp.set_is_collected(req.is_collected());
}