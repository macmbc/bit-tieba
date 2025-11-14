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
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto tieba_name = root["tieba_name"].asString();
	auto tieba_desc = root["tieba_desc"].asString();
	auto tieba_icon = root["tieba_icon"].asString();
	
	std::cout << "CreateTieba: uid=" << uid << ", name=" << tieba_name << std::endl;
	
	Json::Value rtvalue;
	
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_CREATE_TIEBA_RSP);
	});
	
	// 检查贴吧名称是否有效
	if (tieba_name.empty() || tieba_name.length() > 50) {
		rtvalue["error"] = ErrorCodes::TiebaNameInvalid;
		return;
	}
	
	// 创建贴吧
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	int tieba_id = tieba_dao.CreateTieba(uid, tieba_name, tieba_desc, tieba_icon);
	
	if (tieba_id == 0) {
		rtvalue["error"] = ErrorCodes::TiebaExist;
		return;
	} else if (tieba_id < 0) {
		rtvalue["error"] = ErrorCodes::RPCFailed;
		return;
	}
	
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["tieba_id"] = tieba_id;
	rtvalue["tieba_name"] = tieba_name;
}

// 搜索贴吧
void LogicSystem::SearchTiebaHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto keyword = root["keyword"].asString();
	auto offset = root["offset"].asInt();
	auto limit = root["limit"].asInt();
	
	std::cout << "SearchTieba: keyword=" << keyword << ", offset=" << offset << ", limit=" << limit << std::endl;
	
	Json::Value rtvalue;
	
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_SEARCH_TIEBA_RSP);
	});
	
	if (limit <= 0 || limit > 100) {
		limit = 20;
	}
	
	std::vector<std::shared_ptr<TiebaInfo>> tieba_list;
	int total = 0;
	
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.SearchTieba(keyword, offset, limit, tieba_list, total);
	
	if (!success) {
		rtvalue["error"] = ErrorCodes::RPCFailed;
		return;
	}
	
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["total"] = total;
	
	for (auto& tieba : tieba_list) {
		Json::Value tieba_obj;
		tieba_obj["tieba_id"] = tieba->tieba_id;
		tieba_obj["tieba_name"] = tieba->tieba_name;
		tieba_obj["tieba_desc"] = tieba->tieba_desc;
		tieba_obj["tieba_icon"] = tieba->tieba_icon;
		tieba_obj["owner_uid"] = tieba->owner_uid;
		tieba_obj["member_count"] = tieba->member_count;
		tieba_obj["post_count"] = tieba->post_count;
		tieba_obj["create_time"] = tieba->create_time;
		rtvalue["tieba_list"].append(tieba_obj);
	}
}

// 加入贴吧
void LogicSystem::JoinTiebaHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto tieba_id = root["tieba_id"].asInt();
	
	std::cout << "JoinTieba: uid=" << uid << ", tieba_id=" << tieba_id << std::endl;
	
	Json::Value rtvalue;
	
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_JOIN_TIEBA_RSP);
	});
	
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	
	// 检查贴吧是否存在
	if (!tieba_dao.TiebaExists(tieba_id)) {
		rtvalue["error"] = ErrorCodes::TiebaNotExist;
		return;
	}
	
	// 加入贴吧
	bool success = tieba_dao.JoinTieba(uid, tieba_id);
	
	if (!success) {
		rtvalue["error"] = ErrorCodes::RPCFailed;
		return;
	}
	
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["tieba_id"] = tieba_id;
}

// 退出贴吧
void LogicSystem::LeaveTiebaHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto tieba_id = root["tieba_id"].asInt();
	
	std::cout << "LeaveTieba: uid=" << uid << ", tieba_id=" << tieba_id << std::endl;
	
	Json::Value rtvalue;
	
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_LEAVE_TIEBA_RSP);
	});
	
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	
	// 退出贴吧
	bool success = tieba_dao.LeaveTieba(uid, tieba_id);
	
	if (!success) {
		rtvalue["error"] = ErrorCodes::PermissionDenied;
		return;
	}
	
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["tieba_id"] = tieba_id;
}

// 发帖
void LogicSystem::CreatePostHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto tieba_id = root["tieba_id"].asInt();
	auto title = root["title"].asString();
	auto content = root["content"].asString();
	
	std::cout << "CreatePost: uid=" << uid << ", tieba_id=" << tieba_id << ", title=" << title << std::endl;
	
	Json::Value rtvalue;
	
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_CREATE_POST_RSP);
	});
	
	// 检查标题和内容
	if (title.empty() || title.length() > 200) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}
	
	if (content.empty() || content.length() > 10000) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}
	
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	int post_id = tieba_dao.CreatePost(uid, tieba_id, title, content);
	
	if (post_id < 0) {
		rtvalue["error"] = ErrorCodes::NotTiebaMember;
		return;
	}
	
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["post_id"] = post_id;
	rtvalue["tieba_id"] = tieba_id;
}

// 回复
void LogicSystem::CreateReplyHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto post_id = root["post_id"].asInt();
	auto content = root["content"].asString();
	auto reply_to_uid = root["reply_to_uid"].asInt();
	auto reply_to_reply_id = root["reply_to_reply_id"].asInt();
	
	std::cout << "CreateReply: uid=" << uid << ", post_id=" << post_id << std::endl;
	
	Json::Value rtvalue;
	
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_CREATE_REPLY_RSP);
	});
	
	// 检查内容
	if (content.empty() || content.length() > 5000) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}
	
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	int reply_id = tieba_dao.CreateReply(uid, post_id, content, reply_to_uid, reply_to_reply_id);
	
	if (reply_id < 0) {
		rtvalue["error"] = ErrorCodes::PostNotExist;
		return;
	}
	
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["reply_id"] = reply_id;
	rtvalue["post_id"] = post_id;
}
// 获取帖子列表
void LogicSystem::GetPostListHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto tieba_id = root["tieba_id"].asInt();
	auto offset = root["offset"].asInt();
	auto limit = root["limit"].asInt();
	auto sort_type = root["sort_type"].asInt();
	
	std::cout << "GetPostList: tieba_id=" << tieba_id << ", offset=" << offset << ", limit=" << limit << std::endl;
	
	Json::Value rtvalue;
	
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_GET_POST_LIST_RSP);
	});
	
	if (limit <= 0 || limit > 100) {
		limit = 20;
	}
	
	std::vector<std::shared_ptr<PostInfo>> post_list;
	int total = 0;
	
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.GetPostList(uid, tieba_id, offset, limit, sort_type, post_list, total);
	
	if (!success) {
		rtvalue["error"] = ErrorCodes::RPCFailed;
		return;
	}
	
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["total"] = total;
	
	for (auto& post : post_list) {
		Json::Value post_obj;
		post_obj["post_id"] = post->post_id;
		post_obj["tieba_id"] = post->tieba_id;
		post_obj["uid"] = post->uid;
		post_obj["user_name"] = post->user_name;
		post_obj["user_icon"] = post->user_icon;
		post_obj["title"] = post->title;
		post_obj["content"] = post->content;
		post_obj["reply_count"] = post->reply_count;
		post_obj["like_count"] = post->like_count;
		post_obj["is_top"] = post->is_top;
		post_obj["is_essence"] = post->is_essence;
		post_obj["create_time"] = post->create_time;
		post_obj["is_liked"] = post->is_liked;
		rtvalue["post_list"].append(post_obj);
	}
}

// 获取回复列表
void LogicSystem::GetReplyListHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto post_id = root["post_id"].asInt();
	auto offset = root["offset"].asInt();
	auto limit = root["limit"].asInt();
	
	std::cout << "GetReplyList: post_id=" << post_id << ", offset=" << offset << ", limit=" << limit << std::endl;
	
	Json::Value rtvalue;
	
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_GET_REPLY_LIST_RSP);
	});
	
	if (limit <= 0 || limit > 100) {
		limit = 20;
	}
	
	// 获取帖子信息
	std::shared_ptr<PostInfo> post_info;
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool b_post = tieba_dao.GetPostInfo(uid, post_id, post_info);
	if (!b_post) {
		rtvalue["error"] = ErrorCodes::PostNotExist;
		return;
	}
	
	// 获取回复列表
	std::vector<std::shared_ptr<ReplyInfo>> reply_list;
	int total = 0;
	bool success = tieba_dao.GetReplyList(uid, post_id, offset, limit, reply_list, total);
	
	if (!success) {
		rtvalue["error"] = ErrorCodes::RPCFailed;
		return;
	}
	
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["total"] = total;
	
	// 添加帖子信息
	Json::Value post_obj;
	post_obj["post_id"] = post_info->post_id;
	post_obj["tieba_id"] = post_info->tieba_id;
	post_obj["uid"] = post_info->uid;
	post_obj["user_name"] = post_info->user_name;
	post_obj["user_icon"] = post_info->user_icon;
	post_obj["title"] = post_info->title;
	post_obj["content"] = post_info->content;
	post_obj["reply_count"] = post_info->reply_count;
	post_obj["like_count"] = post_info->like_count;
	post_obj["is_top"] = post_info->is_top;
	post_obj["is_essence"] = post_info->is_essence;
	post_obj["create_time"] = post_info->create_time;
	post_obj["is_liked"] = post_info->is_liked;
	rtvalue["post_info"] = post_obj;
	
	// 添加回复列表
	for (auto& reply : reply_list) {
		Json::Value reply_obj;
		reply_obj["reply_id"] = reply->reply_id;
		reply_obj["post_id"] = reply->post_id;
		reply_obj["uid"] = reply->uid;
		reply_obj["user_name"] = reply->user_name;
		reply_obj["user_icon"] = reply->user_icon;
		reply_obj["content"] = reply->content;
		reply_obj["reply_to_uid"] = reply->reply_to_uid;
		reply_obj["reply_to_user_name"] = reply->reply_to_user_name;
		reply_obj["reply_to_reply_id"] = reply->reply_to_reply_id;
		reply_obj["like_count"] = reply->like_count;
		reply_obj["create_time"] = reply->create_time;
		reply_obj["is_liked"] = reply->is_liked;
		rtvalue["reply_list"].append(reply_obj);
	}
}

// 删除帖子
void LogicSystem::DeletePostHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto post_id = root["post_id"].asInt();
	auto tieba_id = root["tieba_id"].asInt();
	
	std::cout << "DeletePost: uid=" << uid << ", post_id=" << post_id << ", tieba_id=" << tieba_id << std::endl;
	
	Json::Value rtvalue;
	
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_DELETE_POST_RSP);
	});
	
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.DeletePost(uid, post_id, tieba_id);
	
	if (!success) {
		rtvalue["error"] = ErrorCodes::PermissionDenied;
		return;
	}
	
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["post_id"] = post_id;
}

// 删除回复
void LogicSystem::DeleteReplyHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto reply_id = root["reply_id"].asInt();
	auto post_id = root["post_id"].asInt();
	
	std::cout << "DeleteReply: uid=" << uid << ", reply_id=" << reply_id << ", post_id=" << post_id << std::endl;
	
	Json::Value rtvalue;
	
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_DELETE_REPLY_RSP);
	});
	
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.DeleteReply(uid, reply_id, post_id);
	
	if (!success) {
		rtvalue["error"] = ErrorCodes::PermissionDenied;
		return;
	}
	
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["reply_id"] = reply_id;
}

// 点赞/取消点赞
void LogicSystem::LikeHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto target_type = root["target_type"].asInt();
	auto target_id = root["target_id"].asInt();
	auto is_like = root["is_like"].asBool();
	
	std::cout << "Like: uid=" << uid << ", target_type=" << target_type << ", target_id=" << target_id << ", is_like=" << is_like << std::endl;
	
	Json::Value rtvalue;
	
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_LIKE_RSP);
	});
	
	int like_count = 0;
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.Like(uid, target_type, target_id, is_like, like_count);
	
	if (!success) {
		rtvalue["error"] = ErrorCodes::RPCFailed;
		return;
	}
	
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["target_type"] = target_type;
	rtvalue["target_id"] = target_id;
	rtvalue["is_like"] = is_like;
	rtvalue["like_count"] = like_count;
}

// 置顶
void LogicSystem::SetTopHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto post_id = root["post_id"].asInt();
	auto tieba_id = root["tieba_id"].asInt();
	auto is_top = root["is_top"].asBool();
	
	std::cout << "SetTop: uid=" << uid << ", post_id=" << post_id << ", tieba_id=" << tieba_id << ", is_top=" << is_top << std::endl;
	
	Json::Value rtvalue;
	
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_SET_TOP_RSP);
	});
	
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.SetTop(uid, post_id, tieba_id, is_top);
	
	if (!success) {
		rtvalue["error"] = ErrorCodes::PermissionDenied;
		return;
	}
	
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["post_id"] = post_id;
	rtvalue["is_top"] = is_top;
}

// 加精
void LogicSystem::SetEssenceHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto post_id = root["post_id"].asInt();
	auto tieba_id = root["tieba_id"].asInt();
	auto is_essence = root["is_essence"].asBool();
	
	std::cout << "SetEssence: uid=" << uid << ", post_id=" << post_id << ", tieba_id=" << tieba_id << ", is_essence=" << is_essence << std::endl;
	
	Json::Value rtvalue;
	
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_SET_ESSENCE_RSP);
	});
	
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.SetEssence(uid, post_id, tieba_id, is_essence);
	
	if (!success) {
		rtvalue["error"] = ErrorCodes::PermissionDenied;
		return;
	}
	
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["post_id"] = post_id;
	rtvalue["is_essence"] = is_essence;
}

// 获取贴吧信息
void LogicSystem::GetTiebaInfoHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto tieba_id = root["tieba_id"].asInt();
	
	std::cout << "GetTiebaInfo: uid=" << uid << ", tieba_id=" << tieba_id << std::endl;
	
	Json::Value rtvalue;
	
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_GET_TIEBA_INFO_RSP);
	});
	
	std::shared_ptr<TiebaInfo> tieba_info;
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.GetTiebaInfo(tieba_id, tieba_info);
	
	if (!success) {
		rtvalue["error"] = ErrorCodes::TiebaNotExist;
		return;
	}
	
	rtvalue["error"] = ErrorCodes::Success;
	
	// 添加贴吧信息
	Json::Value tieba_obj;
	tieba_obj["tieba_id"] = tieba_info->tieba_id;
	tieba_obj["tieba_name"] = tieba_info->tieba_name;
	tieba_obj["tieba_desc"] = tieba_info->tieba_desc;
	tieba_obj["tieba_icon"] = tieba_info->tieba_icon;
	tieba_obj["owner_uid"] = tieba_info->owner_uid;
	tieba_obj["member_count"] = tieba_info->member_count;
	tieba_obj["post_count"] = tieba_info->post_count;
	tieba_obj["create_time"] = tieba_info->create_time;
	rtvalue["tieba_info"] = tieba_obj;
	
	// 检查用户是否为成员
	int role = 0;
	bool is_member = tieba_dao.IsTiebaMember(uid, tieba_id, role);
	rtvalue["is_member"] = is_member;
	rtvalue["member_role"] = role;
}

// 获取我的贴吧列表
void LogicSystem::GetMyTiebaListHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	
	std::cout << "GetMyTiebaList: uid=" << uid << std::endl;
	
	Json::Value rtvalue;
	
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_GET_MY_TIEBA_LIST_RSP);
	});
	
	std::vector<std::shared_ptr<TiebaInfo>> tieba_list;
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.GetMyTiebaList(uid, tieba_list);
	
	if (!success) {
		rtvalue["error"] = ErrorCodes::RPCFailed;
		return;
	}
	
	rtvalue["error"] = ErrorCodes::Success;
	
	for (auto& tieba : tieba_list) {
		Json::Value tieba_obj;
		tieba_obj["tieba_id"] = tieba->tieba_id;
		tieba_obj["tieba_name"] = tieba->tieba_name;
		tieba_obj["tieba_desc"] = tieba->tieba_desc;
		tieba_obj["tieba_icon"] = tieba->tieba_icon;
		tieba_obj["owner_uid"] = tieba->owner_uid;
		tieba_obj["member_count"] = tieba->member_count;
		tieba_obj["post_count"] = tieba->post_count;
		tieba_obj["create_time"] = tieba->create_time;
		rtvalue["tieba_list"].append(tieba_obj);
	}
}

// 获取贴吧成员列表
void LogicSystem::GetTiebaMemberListHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto tieba_id = root["tieba_id"].asInt();
	auto offset = root["offset"].asInt();
	auto limit = root["limit"].asInt();
	
	std::cout << "GetTiebaMemberList: tieba_id=" << tieba_id << ", offset=" << offset << ", limit=" << limit << std::endl;
	
	Json::Value rtvalue;
	
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_GET_TIEBA_MEMBER_LIST_RSP);
	});
	
	if (limit <= 0 || limit > 100) {
		limit = 20;
	}
	
	std::vector<std::shared_ptr<TiebaMemberInfo>> member_list;
	int total = 0;
	auto& tieba_dao = MysqlMgr::GetInstance()->GetTiebaDao();
	bool success = tieba_dao.GetTiebaMemberList(uid, tieba_id, offset, limit, member_list, total);
	
	if (!success) {
		rtvalue["error"] = ErrorCodes::RPCFailed;
		return;
	}
	
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["total"] = total;
	
	for (auto& member : member_list) {
		Json::Value member_obj;
		member_obj["uid"] = member->uid;
		member_obj["user_name"] = member->user_name;
		member_obj["user_icon"] = member->user_icon;
		member_obj["role"] = member->role;
		member_obj["join_time"] = member->join_time;
		rtvalue["member_list"].append(member_obj);
	}
}
