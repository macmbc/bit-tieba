#pragma once
#include "MysqlDao.h"
#include "data.h"
#include <vector>
#include <memory>

class TiebaDao {
public:
	TiebaDao();
	~TiebaDao();

	// 创建贴吧
	int CreateTieba(int uid, const std::string& tieba_name, const std::string& tieba_desc, const std::string& tieba_icon);

	// 搜索贴吧
	bool SearchTieba(const std::string& keyword, int offset, int limit, std::vector<std::shared_ptr<TiebaInfo>>& tieba_list, int& total);

	// 获取贴吧信息
	bool GetTiebaInfo(int tieba_id, std::shared_ptr<TiebaInfo>& tieba_info);
	bool GetTiebaInfo(const std::string& tieba_name, std::shared_ptr<TiebaInfo>& tieba_info);

	// 检查贴吧是否存在
	bool TiebaExists(const std::string& tieba_name);
	bool TiebaExists(int tieba_id);

	// 加入贴吧
	bool JoinTieba(int uid, int tieba_id);

	// 退出贴吧
	bool LeaveTieba(int uid, int tieba_id);

	// 检查是否为贴吧成员
	bool IsTiebaMember(int uid, int tieba_id, int& role);  // role: 0-非成员, 1-普通成员, 2-小吧主, 3-吧主

	// 发帖
	int CreatePost(int uid, int tieba_id, const std::string& title, const std::string& content);

	// 回复
	int CreateReply(int uid, int post_id, const std::string& content, int reply_to_uid, int reply_to_reply_id);

	// 获取帖子列表
	bool GetPostList(int uid, int tieba_id, int offset, int limit, int sort_type, std::vector<std::shared_ptr<PostInfo>>& post_list, int& total);

	// 获取回复列表
	bool GetReplyList(int uid, int post_id, int offset, int limit, std::vector<std::shared_ptr<ReplyInfo>>& reply_list, int& total);

	// 获取帖子信息
	bool GetPostInfo(int uid, int post_id, std::shared_ptr<PostInfo>& post_info);

	// 获取回复信息
	bool GetReplyInfo(int uid, int reply_id, std::shared_ptr<ReplyInfo>& reply_info);

	// 删除帖子
	bool DeletePost(int uid, int post_id, int tieba_id);

	// 删除回复
	bool DeleteReply(int uid, int reply_id, int post_id);

	// 点赞/取消点赞
	bool Like(int uid, int target_type, int target_id, bool is_like, int& like_count);

	// 检查是否点赞
	bool IsLiked(int uid, int target_type, int target_id);

	// 置顶
	bool SetTop(int uid, int post_id, int tieba_id, bool is_top);

	// 加精
	bool SetEssence(int uid, int post_id, int tieba_id, bool is_essence);

	// 获取我的贴吧列表
	bool GetMyTiebaList(int uid, std::vector<std::shared_ptr<TiebaInfo>>& tieba_list);

	// 获取贴吧成员列表
	bool GetTiebaMemberList(int uid, int tieba_id, int offset, int limit, std::vector<std::shared_ptr<TiebaMemberInfo>>& member_list, int& total);

private:
	std::unique_ptr<MySqlPool> pool_;
};

