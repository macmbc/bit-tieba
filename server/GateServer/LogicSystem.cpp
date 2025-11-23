#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VerifyGrpcClient.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"
#include "StatusGrpcClient.h"
#include <cstdlib>

static bool CheckTokenValid(int uid, const std::string& token) {
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	std::string token_value = "";
	bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
	if (!success) {
		return false;
	}
	return token_value == token;
}

LogicSystem::LogicSystem() {
	RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
		beast::ostream(connection->_response.body()) << "receive get_test req " << std::endl;
		int i = 0;
		for (auto& elem : connection->_get_params) {
			i++;
			beast::ostream(connection->_response.body()) << "param" << i << " key is " << elem.first;
			beast::ostream(connection->_response.body()) << ", " <<  " value is " << elem.second << std::endl;
		}

		connection->_response.set(http::field::content_type, "text/plain");
	});

	RegPost("/test_procedure", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (!src_root.isMember("email")) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		int uid = 0;
		std::string name = "";
		MysqlMgr::GetInstance()->TestProcedure(email, uid, name);
		cout << "email is " << email << endl;
		root["error"] = ErrorCodes::Success;
		root["email"] = src_root["email"];
		root["name"] = name;
		root["uid"] = uid;
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		
	});

	RegPost("/get_varifycode", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (!src_root.isMember("email")) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		GetVarifyRsp rsp = VerifyGrpcClient::GetInstance()->GetVarifyCode(email);
		cout << "email is " << email << endl;
		root["error"] = rsp.error();
		root["email"] = src_root["email"];
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
	});
	//day11 注册用户逻辑
	RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();
		auto confirm = src_root["confirm"].asString();
		auto icon = src_root["icon"].asString();

		if (pwd != confirm) {
			std::cout << "password err " << std::endl;
			root["error"] = ErrorCodes::PasswdErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//先查找redis中email对应的验证码是否合理
		std::string  varify_code;
		bool b_get_varify = RedisMgr::GetInstance()->Get(CODEPREFIX+src_root["email"].asString(), varify_code);
		if (!b_get_varify) {
			std::cout << " get varify code expired" << std::endl;
			root["error"] = ErrorCodes::VarifyExpired;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (varify_code != src_root["varifycode"].asString()) {
			std::cout << " varify code error" << std::endl;
			root["error"] = ErrorCodes::VarifyCodeErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//查找数据库判断用户是否存在
		int uid = MysqlMgr::GetInstance()->RegUser(name, email, pwd, icon);
		if (uid == 0 || uid == -1) {
			std::cout << " user or email exist" << std::endl;
			root["error"] = ErrorCodes::UserExist;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		root["error"] = 0;
		root["uid"] = uid;
		root["email"] = email;
		root ["user"]= name;
		root["passwd"] = pwd;
		root["confirm"] = confirm;
		root["icon"] = icon;
		root["varifycode"] = src_root["varifycode"].asString();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	//重置回调逻辑
	RegPost("/reset_pwd", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();

		//先查找redis中email对应的验证码是否合理
		std::string  varify_code;
		bool b_get_varify = RedisMgr::GetInstance()->Get(CODEPREFIX + src_root["email"].asString(), varify_code);
		if (!b_get_varify) {
			std::cout << " get varify code expired" << std::endl;
			root["error"] = ErrorCodes::VarifyExpired;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (varify_code != src_root["varifycode"].asString()) {
			std::cout << " varify code error" << std::endl;
			root["error"] = ErrorCodes::VarifyCodeErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//查询数据库判断用户名和邮箱是否匹配
		bool email_valid = MysqlMgr::GetInstance()->CheckEmail(name, email);
		if (!email_valid) {
			std::cout << " user email not match" << std::endl;
			root["error"] = ErrorCodes::EmailNotMatch;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//更新密码为最新密码
		bool b_up = MysqlMgr::GetInstance()->UpdatePwd(name, pwd);
		if (!b_up) {
			std::cout << " update pwd failed" << std::endl;
			root["error"] = ErrorCodes::PasswdUpFailed;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		std::cout << "succeed to update password" << pwd << std::endl;
		root["error"] = 0;
		root["email"] = email;
		root["user"] = name;
		root["passwd"] = pwd;
		root["varifycode"] = src_root["varifycode"].asString();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	//用户登录逻辑
	RegPost("/user_login", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		auto pwd = src_root["passwd"].asString();
		UserInfo userInfo;
		//查询数据库判断用户名和密码是否匹配
		bool pwd_valid = MysqlMgr::GetInstance()->CheckPwd(email, pwd, userInfo);
		if (!pwd_valid) {
			std::cout << " user pwd not match" << std::endl;
			root["error"] = ErrorCodes::PasswdInvalid;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//查询StatusServer找到合适的连接
		auto reply = StatusGrpcClient::GetInstance()->GetChatServer(userInfo.uid);
		if (reply.error()) {
			std::cout << " grpc get chat server failed, error is " << reply.error()<< std::endl;
			root["error"] = ErrorCodes::RPCFailed;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		std::cout << "succeed to load userinfo uid is " << userInfo.uid << std::endl;
		root["error"] = 0;
		root["email"] = email;
		root["uid"] = userInfo.uid;
		root["token"] = reply.token();
		root["host"] = reply.host();
		root["port"] = reply.port();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	// 获取贴吧列表
	RegGet("/forums", [](std::shared_ptr<HttpConnection> connection) {
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		int page = 1;
		int limit = 20;
		try {
			if (connection->_get_params.find("page") != connection->_get_params.end()) {
				page = std::stoi(connection->_get_params["page"]);
			}
			if (connection->_get_params.find("limit") != connection->_get_params.end()) {
				limit = std::stoi(connection->_get_params["limit"]);
			}
		}
		catch (...) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (page <= 0) page = 1;
		if (limit <= 0) limit = 20;

		std::vector<ForumInfo> forums;
		bool ok = MysqlMgr::GetInstance()->ListForums(page, limit, forums);
		if (!ok) {
			root["error"] = ErrorCodes::DbError;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		root["error"] = ErrorCodes::Success;
		for (auto& f : forums) {
			Json::Value item;
			item["forum_id"] = f.forum_id;
			item["name"] = f.name;
			item["avatar"] = f.avatar;
			item["description"] = f.description;
			item["follower_count"] = f.follower_count;
			item["post_count"] = f.post_count;
			root["forums"].append(item);
		}
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	// 贴吧详情
	RegGet("/forum/detail", [](std::shared_ptr<HttpConnection> connection) {
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		int forum_id = 0;
		int uid = 0;
		std::string token = "";
		try {
			if (connection->_get_params.find("forum_id") != connection->_get_params.end()) {
				forum_id = std::stoi(connection->_get_params["forum_id"]);
			}
			if (connection->_get_params.find("uid") != connection->_get_params.end()) {
				uid = std::stoi(connection->_get_params["uid"]);
			}
			if (connection->_get_params.find("token") != connection->_get_params.end()) {
				token = connection->_get_params["token"];
			}
		}
		catch (...) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (forum_id <= 0) {
			root["error"] = ErrorCodes::ForumNotFound;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		ForumInfo info;
		bool ok = MysqlMgr::GetInstance()->GetForum(forum_id, info);
		if (!ok) {
			root["error"] = ErrorCodes::ForumNotFound;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		bool is_followed = false;
		if (uid > 0 && !token.empty() && CheckTokenValid(uid, token)) {
			bool b_follow = false;
			if (MysqlMgr::GetInstance()->IsFollowed(uid, forum_id, b_follow)) {
				is_followed = b_follow;
			}
		}

		root["error"] = ErrorCodes::Success;
		root["forum"]["forum_id"] = info.forum_id;
		root["forum"]["name"] = info.name;
		root["forum"]["avatar"] = info.avatar;
		root["forum"]["description"] = info.description;
		root["forum"]["follower_count"] = info.follower_count;
		root["forum"]["post_count"] = info.post_count;
		root["forum"]["is_followed"] = is_followed;
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	// 关注贴吧
	RegPost("/follow_forum", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto uid = src_root.get("uid", 0).asInt();
		auto token = src_root.get("token", "").asString();
		auto forum_id = src_root.get("forum_id", 0).asInt();

		if (uid <= 0 || forum_id <= 0 || token.empty()) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (!CheckTokenValid(uid, token)) {
			root["error"] = ErrorCodes::TokenInvalid;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		int ret = MysqlMgr::GetInstance()->FollowForum(uid, forum_id);
		if (ret == 2) {
			root["error"] = ErrorCodes::ForumNotFound;
		}
		else if (ret == 1) {
			root["error"] = ErrorCodes::AlreadyFollowed;
		}
		else if (ret == 0) {
			root["error"] = ErrorCodes::Success;
		}
		else {
			root["error"] = ErrorCodes::DbError;
		}
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	// 取消关注贴吧
	RegPost("/unfollow_forum", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto uid = src_root.get("uid", 0).asInt();
		auto token = src_root.get("token", "").asString();
		auto forum_id = src_root.get("forum_id", 0).asInt();

		if (uid <= 0 || forum_id <= 0 || token.empty()) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (!CheckTokenValid(uid, token)) {
			root["error"] = ErrorCodes::TokenInvalid;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		int ret = MysqlMgr::GetInstance()->UnfollowForum(uid, forum_id);
		if (ret == 2) {
			root["error"] = ErrorCodes::ForumNotFound;
		}
		else if (ret == 1) {
			root["error"] = ErrorCodes::NotFollowed;
		}
		else if (ret == 0) {
			root["error"] = ErrorCodes::Success;
		}
		else {
			root["error"] = ErrorCodes::DbError;
		}
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	// 获取已关注贴吧
	RegGet("/followed_forums", [](std::shared_ptr<HttpConnection> connection) {
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		int uid = 0;
		std::string token;
		int page = 1;
		int limit = 20;
		try {
			if (connection->_get_params.find("uid") != connection->_get_params.end()) {
				uid = std::stoi(connection->_get_params["uid"]);
			}
			if (connection->_get_params.find("token") != connection->_get_params.end()) {
				token = connection->_get_params["token"];
			}
			if (connection->_get_params.find("page") != connection->_get_params.end()) {
				page = std::stoi(connection->_get_params["page"]);
			}
			if (connection->_get_params.find("limit") != connection->_get_params.end()) {
				limit = std::stoi(connection->_get_params["limit"]);
			}
		}
		catch (...) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (uid <= 0 || token.empty()) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (!CheckTokenValid(uid, token)) {
			root["error"] = ErrorCodes::TokenInvalid;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (page <= 0) page = 1;
		if (limit <= 0) limit = 20;

		std::vector<ForumInfo> forums;
		bool ok = MysqlMgr::GetInstance()->ListFollowedForums(uid, page, limit, forums);
		if (!ok) {
			root["error"] = ErrorCodes::DbError;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		root["error"] = ErrorCodes::Success;
		for (auto& f : forums) {
			Json::Value item;
			item["forum_id"] = f.forum_id;
			item["name"] = f.name;
			item["avatar"] = f.avatar;
			item["description"] = f.description;
			item["follower_count"] = f.follower_count;
			item["post_count"] = f.post_count;
			root["forums"].append(item);
		}
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});
}

void LogicSystem::RegGet(const std::string& url, HttpHandler handler) {
	_get_handlers.insert(std::make_pair(url, std::move(handler)));
}

void LogicSystem::RegPost(const std::string& url, HttpHandler handler) {
	_post_handlers.insert(std::make_pair(url, std::move(handler)));
}

LogicSystem::~LogicSystem() {

}

bool LogicSystem::HandleGet(const std::string& path, std::shared_ptr<HttpConnection> con) {
	if (_get_handlers.find(path) == _get_handlers.end()) {
		return false;
	}

	_get_handlers[path](con);
	return true;
}

bool LogicSystem::HandlePost(const std::string& path, std::shared_ptr<HttpConnection> con) {
	if (_post_handlers.find(path) == _post_handlers.end()) {
		return false;
	}

	_post_handlers[path](con);
	return true;
}
