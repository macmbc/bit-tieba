#pragma once
#include <string>

// 贴吧相关数据结构
struct TiebaInfo {
	int tieba_id;
	std::string tieba_name;
	std::string tieba_desc;
	std::string tieba_icon;
	int owner_uid;
	int member_count;
	int post_count;
	std::string create_time;
};

