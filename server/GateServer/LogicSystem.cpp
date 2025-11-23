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

	// 帖子列表
	RegGet("/posts", [](std::shared_ptr<HttpConnection> connection) {
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		int forum_id = 0;
		int page = 1;
		int limit = 20;
		std::string sort = "time";
		try {
			if (connection->_get_params.find("forum_id") != connection->_get_params.end()) {
				forum_id = std::stoi(connection->_get_params["forum_id"]);
			}
			if (connection->_get_params.find("page") != connection->_get_params.end()) {
				page = std::stoi(connection->_get_params["page"]);
			}
			if (connection->_get_params.find("limit") != connection->_get_params.end()) {
				limit = std::stoi(connection->_get_params["limit"]);
			}
			if (connection->_get_params.find("sort") != connection->_get_params.end()) {
				sort = connection->_get_params["sort"];
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

		ForumInfo f;
		if (!MysqlMgr::GetInstance()->GetForum(forum_id, f)) {
			root["error"] = ErrorCodes::ForumNotFound;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (page <= 0) page = 1;
		if (limit <= 0) limit = 20;

		std::vector<PostSummary> posts;
		bool ok = MysqlMgr::GetInstance()->ListPosts(forum_id, page, limit, sort, posts);
		if (!ok) {
			root["error"] = ErrorCodes::DbError;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		root["error"] = ErrorCodes::Success;
		for (auto& p : posts) {
			Json::Value item;
			item["post_id"] = p.post_id;
			item["forum_id"] = p.forum_id;
			item["uid"] = p.uid;
			item["author"] = p.author;
			item["title"] = p.title;
			item["created_at"] = p.created_at;
			item["reply_count"] = p.reply_cnt;
			item["like_count"] = p.like_cnt;
			item["is_top"] = p.is_top;
			item["is_essence"] = p.is_essence;
			item["content_preview"] = p.content_preview;
			root["posts"].append(item);
		}
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	// 帖子详情
	RegGet("/post/detail", [](std::shared_ptr<HttpConnection> connection) {
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		int post_id = 0;
		int uid = 0;
		std::string token;
		try {
			if (connection->_get_params.find("post_id") != connection->_get_params.end()) {
				post_id = std::stoi(connection->_get_params["post_id"]);
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

		if (post_id <= 0) {
			root["error"] = ErrorCodes::PostNotFound;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		PostDetail detail;
		if (!MysqlMgr::GetInstance()->GetPostDetail(post_id, detail)) {
			root["error"] = ErrorCodes::PostNotFound;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		bool is_liked = false;
		bool is_collected = false;
		if (uid > 0 && !token.empty() && CheckTokenValid(uid, token)) {
			MysqlMgr::GetInstance()->IsLiked(uid, post_id, is_liked);
			MysqlMgr::GetInstance()->IsCollected(uid, post_id, is_collected);
		}

		root["error"] = ErrorCodes::Success;
		root["post"]["post_id"] = detail.post_id;
		root["post"]["forum_id"] = detail.forum_id;
		root["post"]["uid"] = detail.uid;
		root["post"]["author"] = detail.author;
		root["post"]["title"] = detail.title;
		root["post"]["content"] = detail.content;
		root["post"]["created_at"] = detail.created_at;
		root["post"]["reply_count"] = detail.reply_cnt;
		root["post"]["like_count"] = detail.like_cnt;
		root["post"]["collect_count"] = detail.collect_cnt;
		root["post"]["is_top"] = detail.is_top;
		root["post"]["is_essence"] = detail.is_essence;
		root["post"]["is_liked"] = is_liked;
		root["post"]["is_collected"] = is_collected;
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	// 创建帖子
	RegPost("/post/create", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		if (!reader.parse(body_str, src_root)) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		auto uid = src_root.get("uid", 0).asInt();
		auto token = src_root.get("token", "").asString();
		auto forum_id = src_root.get("forum_id", 0).asInt();
		auto title = src_root.get("title", "").asString();
		auto content = src_root.get("content", "").asString();

		if (uid <= 0 || forum_id <= 0 || token.empty() || title.empty() || content.empty()) {
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

		ForumInfo f;
		if (!MysqlMgr::GetInstance()->GetForum(forum_id, f)) {
			root["error"] = ErrorCodes::ForumNotFound;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		int post_id = 0;
		bool ok = MysqlMgr::GetInstance()->CreatePost(uid, forum_id, title, content, post_id);
		if (!ok) {
			root["error"] = ErrorCodes::DbError;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		root["error"] = ErrorCodes::Success;
		root["post_id"] = post_id;
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	// 更新帖子
	RegPost("/post/update", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		if (!reader.parse(body_str, src_root)) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		auto uid = src_root.get("uid", 0).asInt();
		auto token = src_root.get("token", "").asString();
		auto post_id = src_root.get("post_id", 0).asInt();
		auto title = src_root.get("title", "").asString();
		auto content = src_root.get("content", "").asString();

		if (uid <= 0 || post_id <= 0 || token.empty() || title.empty() || content.empty()) {
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

		int owner_uid = 0;
		int forum_id = 0;
		if (!MysqlMgr::GetInstance()->GetPostOwner(post_id, owner_uid, forum_id)) {
			root["error"] = ErrorCodes::PostNotFound;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (uid != owner_uid) {
			root["error"] = ErrorCodes::NoPermission;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		bool ok = MysqlMgr::GetInstance()->UpdatePost(post_id, uid, title, content);
		if (!ok) {
			root["error"] = ErrorCodes::DbError;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		root["error"] = ErrorCodes::Success;
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	// 删除帖子
	RegPost("/post/delete", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		if (!reader.parse(body_str, src_root)) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		auto uid = src_root.get("uid", 0).asInt();
		auto token = src_root.get("token", "").asString();
		auto post_id = src_root.get("post_id", 0).asInt();

		if (uid <= 0 || post_id <= 0 || token.empty()) {
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

		int owner_uid = 0;
		int forum_id = 0;
		if (!MysqlMgr::GetInstance()->GetPostOwner(post_id, owner_uid, forum_id)) {
			root["error"] = ErrorCodes::PostNotFound;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		ForumInfo finfo;
		if (!MysqlMgr::GetInstance()->GetForum(forum_id, finfo)) {
			root["error"] = ErrorCodes::ForumNotFound;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (uid != owner_uid && uid != finfo.owner_uid) {
			root["error"] = ErrorCodes::NoPermission;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		bool ok = MysqlMgr::GetInstance()->DeletePost(post_id, forum_id);
		if (!ok) {
			root["error"] = ErrorCodes::DbError;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		root["error"] = ErrorCodes::Success;
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	// 点赞帖子
	RegPost("/like_post", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		if (!reader.parse(body_str, src_root)) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		auto uid = src_root.get("uid", 0).asInt();
		auto token = src_root.get("token", "").asString();
		auto post_id = src_root.get("post_id", 0).asInt();

		if (uid <= 0 || post_id <= 0 || token.empty()) {
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

		int ret = MysqlMgr::GetInstance()->LikePost(uid, post_id);
		if (ret == 0) root["error"] = ErrorCodes::Success;
		else if (ret == 1) root["error"] = ErrorCodes::AlreadyLiked;
		else if (ret == 2) root["error"] = ErrorCodes::PostNotFound;
		else root["error"] = ErrorCodes::DbError;
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	// 取消点赞
	RegPost("/unlike_post", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		if (!reader.parse(body_str, src_root)) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		auto uid = src_root.get("uid", 0).asInt();
		auto token = src_root.get("token", "").asString();
		auto post_id = src_root.get("post_id", 0).asInt();

		if (uid <= 0 || post_id <= 0 || token.empty()) {
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

		int ret = MysqlMgr::GetInstance()->UnlikePost(uid, post_id);
		if (ret == 0) root["error"] = ErrorCodes::Success;
		else if (ret == 1) root["error"] = ErrorCodes::NotLiked;
		else if (ret == 2) root["error"] = ErrorCodes::PostNotFound;
		else root["error"] = ErrorCodes::DbError;
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	// 收藏帖子
	RegPost("/collect_post", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		if (!reader.parse(body_str, src_root)) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		auto uid = src_root.get("uid", 0).asInt();
		auto token = src_root.get("token", "").asString();
		auto post_id = src_root.get("post_id", 0).asInt();

		if (uid <= 0 || post_id <= 0 || token.empty()) {
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

		int ret = MysqlMgr::GetInstance()->CollectPost(uid, post_id);
		if (ret == 0) root["error"] = ErrorCodes::Success;
		else if (ret == 1) root["error"] = ErrorCodes::AlreadyCollected;
		else if (ret == 2) root["error"] = ErrorCodes::PostNotFound;
		else root["error"] = ErrorCodes::DbError;
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	// 取消收藏
	RegPost("/uncollect_post", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		if (!reader.parse(body_str, src_root)) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		auto uid = src_root.get("uid", 0).asInt();
		auto token = src_root.get("token", "").asString();
		auto post_id = src_root.get("post_id", 0).asInt();

		if (uid <= 0 || post_id <= 0 || token.empty()) {
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

		int ret = MysqlMgr::GetInstance()->UncollectPost(uid, post_id);
		if (ret == 0) root["error"] = ErrorCodes::Success;
		else if (ret == 1) root["error"] = ErrorCodes::NotCollected;
		else if (ret == 2) root["error"] = ErrorCodes::PostNotFound;
		else root["error"] = ErrorCodes::DbError;
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	// 设置置顶
	RegPost("/set_top_post", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		if (!reader.parse(body_str, src_root)) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		auto uid = src_root.get("uid", 0).asInt();
		auto token = src_root.get("token", "").asString();
		auto post_id = src_root.get("post_id", 0).asInt();
		bool is_top = src_root.get("is_top", true).asBool();

		if (uid <= 0 || post_id <= 0 || token.empty()) {
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

		int owner_uid = 0;
		int forum_id = 0;
		if (!MysqlMgr::GetInstance()->GetPostOwner(post_id, owner_uid, forum_id)) {
			root["error"] = ErrorCodes::PostNotFound;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		ForumInfo finfo;
		if (!MysqlMgr::GetInstance()->GetForum(forum_id, finfo)) {
			root["error"] = ErrorCodes::ForumNotFound;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (uid != finfo.owner_uid) {
			root["error"] = ErrorCodes::NoPermission;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		bool ok = MysqlMgr::GetInstance()->SetTop(post_id, is_top);
		root["error"] = ok ? ErrorCodes::Success : ErrorCodes::DbError;
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	// 设置精华
	RegPost("/set_essence_post", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		if (!reader.parse(body_str, src_root)) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		auto uid = src_root.get("uid", 0).asInt();
		auto token = src_root.get("token", "").asString();
		auto post_id = src_root.get("post_id", 0).asInt();
		bool is_essence = src_root.get("is_essence", true).asBool();

		if (uid <= 0 || post_id <= 0 || token.empty()) {
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

		int owner_uid = 0;
		int forum_id = 0;
		if (!MysqlMgr::GetInstance()->GetPostOwner(post_id, owner_uid, forum_id)) {
			root["error"] = ErrorCodes::PostNotFound;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		ForumInfo finfo;
		if (!MysqlMgr::GetInstance()->GetForum(forum_id, finfo)) {
			root["error"] = ErrorCodes::ForumNotFound;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (uid != finfo.owner_uid) {
			root["error"] = ErrorCodes::NoPermission;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		bool ok = MysqlMgr::GetInstance()->SetEssence(post_id, is_essence);
		root["error"] = ok ? ErrorCodes::Success : ErrorCodes::DbError;
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	// 获取回复列表
	RegGet("/post/replies", [](std::shared_ptr<HttpConnection> connection) {
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		int post_id = 0;
		int page = 1;
		int limit = 20;
		std::string sort = "time";
		int uid = 0;
		std::string token;
		try {
			if (connection->_get_params.find("post_id") != connection->_get_params.end()) {
				post_id = std::stoi(connection->_get_params["post_id"]);
			}
			if (connection->_get_params.find("page") != connection->_get_params.end()) {
				page = std::stoi(connection->_get_params["page"]);
			}
			if (connection->_get_params.find("limit") != connection->_get_params.end()) {
				limit = std::stoi(connection->_get_params["limit"]);
			}
			if (connection->_get_params.find("sort") != connection->_get_params.end()) {
				sort = connection->_get_params["sort"];
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

		if (post_id <= 0) {
			root["error"] = ErrorCodes::PostNotFound;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		PostDetail detail;
		if (!MysqlMgr::GetInstance()->GetPostDetail(post_id, detail)) {
			root["error"] = ErrorCodes::PostNotFound;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (page <= 0) page = 1;
		if (limit <= 0) limit = 20;

		std::vector<ReplyInfo> tops;
		bool ok = MysqlMgr::GetInstance()->ListTopReplies(post_id, page, limit, sort, tops);
		if (!ok) {
			root["error"] = ErrorCodes::DbError;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		std::vector<long long> root_ids;
		for (auto& t : tops) {
			root_ids.push_back(t.reply_id);
		}
		std::unordered_map<long long, std::vector<ReplyInfo>> children;
		MysqlMgr::GetInstance()->ListChildrenReplies(post_id, root_ids, children);

		bool authed = (uid > 0 && !token.empty() && CheckTokenValid(uid, token));
		std::unordered_set<long long> liked_set;
		if (authed) {
			std::vector<long long> all_ids;
			for (auto& t : tops) all_ids.push_back(t.reply_id);
			for (auto& kv : children) {
				for (auto& c : kv.second) all_ids.push_back(c.reply_id);
			}
			MysqlMgr::GetInstance()->BatchReplyLiked(uid, all_ids, liked_set);
		}

		root["error"] = ErrorCodes::Success;
		for (auto& t : tops) {
			Json::Value item;
			item["reply_id"] = Json::Int64(t.reply_id);
			item["post_id"] = t.post_id;
			item["uid"] = t.uid;
			item["content"] = t.content;
			item["parent_reply_id"] = Json::Int64(t.parent_reply_id);
			item["floor"] = t.floor;
			item["like_count"] = t.like_cnt;
			item["created_at"] = t.created_at;
			item["is_liked"] = authed && liked_set.count(t.reply_id) > 0;
			for (auto& c : children[t.reply_id]) {
				Json::Value sub;
				sub["reply_id"] = Json::Int64(c.reply_id);
				sub["post_id"] = c.post_id;
				sub["uid"] = c.uid;
				sub["content"] = c.content;
				sub["parent_reply_id"] = Json::Int64(c.parent_reply_id);
				sub["root_reply_id"] = Json::Int64(c.root_reply_id);
				sub["floor"] = c.floor;
				sub["like_count"] = c.like_cnt;
				sub["created_at"] = c.created_at;
				sub["is_liked"] = authed && liked_set.count(c.reply_id) > 0;
				item["sub_replies"].append(sub);
			}
			root["replies"].append(item);
		}

		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	// 创建回复
	RegPost("/reply", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		if (!reader.parse(body_str, src_root)) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		auto uid = src_root.get("uid", 0).asInt();
		auto token = src_root.get("token", "").asString();
		auto post_id = src_root.get("post_id", 0).asInt();
		auto content = src_root.get("content", "").asString();
		long long parent_reply_id = src_root.get("parent_reply_id", 0).asInt64();

		if (uid <= 0 || post_id <= 0 || token.empty() || content.empty()) {
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

		PostDetail detail;
		if (!MysqlMgr::GetInstance()->GetPostDetail(post_id, detail)) {
			root["error"] = ErrorCodes::PostNotFound;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		long long reply_id = 0;
		long long root_reply_id = 0;
		int floor = 0;
		bool ok = MysqlMgr::GetInstance()->CreateReply(uid, post_id, content, parent_reply_id, reply_id, root_reply_id, floor);
		if (!ok) {
			root["error"] = ErrorCodes::DbError;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		root["error"] = ErrorCodes::Success;
		root["reply_id"] = Json::Int64(reply_id);
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	// 删除回复
	RegPost("/reply/delete", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		if (!reader.parse(body_str, src_root)) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		auto uid = src_root.get("uid", 0).asInt();
		auto token = src_root.get("token", "").asString();
		long long reply_id = src_root.get("reply_id", 0).asInt64();

		if (uid <= 0 || reply_id <= 0 || token.empty()) {
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

		int owner_uid = 0;
		int post_id = 0;
		long long parent_reply_id = 0;
		long long root_reply_id = 0;
		if (!MysqlMgr::GetInstance()->GetReplyOwner(reply_id, owner_uid, post_id, parent_reply_id, root_reply_id)) {
			root["error"] = ErrorCodes::ReplyNotFound;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		int post_owner = 0;
		int forum_id = 0;
		if (!MysqlMgr::GetInstance()->GetPostOwner(post_id, post_owner, forum_id)) {
			root["error"] = ErrorCodes::PostNotFound;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		ForumInfo finfo;
		if (!MysqlMgr::GetInstance()->GetForum(forum_id, finfo)) {
			root["error"] = ErrorCodes::ForumNotFound;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (uid != owner_uid && uid != post_owner && uid != finfo.owner_uid) {
			root["error"] = ErrorCodes::NoPermission;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		bool ok = MysqlMgr::GetInstance()->DeleteReply(reply_id, post_id);
		root["error"] = ok ? ErrorCodes::Success : ErrorCodes::DbError;
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	// 点赞回复
	RegPost("/like_reply", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		if (!reader.parse(body_str, src_root)) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		auto uid = src_root.get("uid", 0).asInt();
		auto token = src_root.get("token", "").asString();
		long long reply_id = src_root.get("reply_id", 0).asInt64();

		if (uid <= 0 || reply_id <= 0 || token.empty()) {
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

		int ret = MysqlMgr::GetInstance()->LikeReply(uid, reply_id);
		if (ret == 0) root["error"] = ErrorCodes::Success;
		else if (ret == 1) root["error"] = ErrorCodes::AlreadyLikedReply;
		else if (ret == 2) root["error"] = ErrorCodes::ReplyNotFound;
		else root["error"] = ErrorCodes::DbError;
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	// 取消点赞回复
	RegPost("/unlike_reply", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		if (!reader.parse(body_str, src_root)) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		auto uid = src_root.get("uid", 0).asInt();
		auto token = src_root.get("token", "").asString();
		long long reply_id = src_root.get("reply_id", 0).asInt64();

		if (uid <= 0 || reply_id <= 0 || token.empty()) {
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

		int ret = MysqlMgr::GetInstance()->UnlikeReply(uid, reply_id);
		if (ret == 0) root["error"] = ErrorCodes::Success;
		else if (ret == 1) root["error"] = ErrorCodes::NotLikedReply;
		else if (ret == 2) root["error"] = ErrorCodes::ReplyNotFound;
		else root["error"] = ErrorCodes::DbError;
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	// 搜索帖子/贴吧/用户
	RegGet("/search", [](std::shared_ptr<HttpConnection> connection) {
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		std::string keyword;
		std::string type = "post";
		int page = 1;
		int limit = 20;
		int forum_id = 0;
		try {
			if (connection->_get_params.find("keyword") != connection->_get_params.end()) {
				keyword = connection->_get_params["keyword"];
			}
			if (connection->_get_params.find("type") != connection->_get_params.end()) {
				type = connection->_get_params["type"];
			}
			if (connection->_get_params.find("page") != connection->_get_params.end()) {
				page = std::stoi(connection->_get_params["page"]);
			}
			if (connection->_get_params.find("limit") != connection->_get_params.end()) {
				limit = std::stoi(connection->_get_params["limit"]);
			}
			if (connection->_get_params.find("forum_id") != connection->_get_params.end()) {
				forum_id = std::stoi(connection->_get_params["forum_id"]);
			}
		}
		catch (...) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (keyword.empty()) {
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		if (page <= 0) page = 1;
		if (limit <= 0) limit = 20;

		if (type == "post") {
			std::vector<PostSummary> posts;
			bool ok = MysqlMgr::GetInstance()->SearchPosts(keyword, forum_id, page, limit, posts);
			if (!ok) {
				root["error"] = ErrorCodes::DbError;
			}
			else {
				root["error"] = ErrorCodes::Success;
				for (auto& p : posts) {
					Json::Value item;
					item["post_id"] = p.post_id;
					item["forum_id"] = p.forum_id;
					item["uid"] = p.uid;
					item["author"] = p.author;
					item["title"] = p.title;
					item["created_at"] = p.created_at;
					item["reply_count"] = p.reply_cnt;
					item["like_count"] = p.like_cnt;
					item["is_top"] = p.is_top;
					item["is_essence"] = p.is_essence;
					item["content_preview"] = p.content_preview;
					root["results"].append(item);
				}
			}
		}
		else if (type == "forum") {
			std::vector<ForumInfo> forums;
			bool ok = MysqlMgr::GetInstance()->SearchForums(keyword, page, limit, forums);
			if (!ok) {
				root["error"] = ErrorCodes::DbError;
			}
			else {
				root["error"] = ErrorCodes::Success;
				for (auto& f : forums) {
					Json::Value item;
					item["forum_id"] = f.forum_id;
					item["name"] = f.name;
					item["avatar"] = f.avatar;
					item["description"] = f.description;
					item["follower_count"] = f.follower_count;
					item["post_count"] = f.post_count;
					root["results"].append(item);
				}
			}
		}
		else if (type == "user") {
			std::vector<UserBrief> users;
			bool ok = MysqlMgr::GetInstance()->SearchUsers(keyword, page, limit, users);
			if (!ok) {
				root["error"] = ErrorCodes::DbError;
			}
			else {
				root["error"] = ErrorCodes::Success;
				for (auto& u : users) {
					Json::Value item;
					item["uid"] = u.uid;
					item["name"] = u.name;
					item["nick"] = u.nick;
					item["email"] = u.email;
					item["icon"] = u.icon;
					item["sex"] = u.sex;
					root["results"].append(item);
				}
			}
		}
		else {
			root["error"] = ErrorCodes::Error_Json;
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
