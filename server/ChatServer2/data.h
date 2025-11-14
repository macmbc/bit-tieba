#pragma once
#include <string>
struct UserInfo {
	UserInfo():name(""), pwd(""),uid(0),email(""),nick(""),desc(""),sex(0), icon(""), back("") {}
	std::string name;
	std::string pwd;
	int uid;
	std::string email;
	std::string nick;
	std::string desc;
	int sex;
	std::string icon;
	std::string back;
};

struct ApplyInfo {
	ApplyInfo(int uid, std::string name, std::string desc,
		std::string icon, std::string nick, int sex, int status)
		:_uid(uid),_name(name),_desc(desc),
		_icon(icon),_nick(nick),_sex(sex),_status(status){}

	int _uid;
	std::string _name;
	std::string _desc;
	std::string _icon;
	std::string _nick;
	int _sex;
	int _status;
};

// 贴吧相关数据结构
struct TiebaInfo
{
	int tieba_id;
	std::string tieba_name;
	std::string tieba_desc;
	std::string tieba_icon;
	int owner_uid;
	int member_count;
	int post_count;
	std::string create_time;
};

struct PostInfo
{
	int post_id;
	int tieba_id;
	int uid;
	std::string user_name;
	std::string user_icon;
	std::string title;
	std::string content;
	int reply_count;
	int like_count;
	bool is_top;
	bool is_essence;
	std::string create_time;
	bool is_liked;
};

struct ReplyInfo
{
	int reply_id;
	int post_id;
	int uid;
	std::string user_name;
	std::string user_icon;
	std::string content;
	int reply_to_uid;
	std::string reply_to_user_name;
	int reply_to_reply_id;
	int like_count;
	std::string create_time;
	bool is_liked;
};

struct TiebaMemberInfo
{
	int uid;
	std::string user_name;
	std::string user_icon;
	int role; // 0-普通成员, 1-小吧主, 2-吧主
	std::string join_time;
};
