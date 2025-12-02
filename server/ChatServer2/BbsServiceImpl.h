#pragma once
#include "bbs.grpc.pb.h"
#include "MysqlMgr.h"
#include "RedisMgr.h"
#include "const.h"
#include <memory>

class BbsServiceImpl final : public bbs::BbsService::Service {
public:
  BbsServiceImpl() = default;
  ~BbsServiceImpl() override = default;

  grpc::Status GetForums(grpc::ServerContext*, const bbs::ForumsReq*, bbs::ForumsResp*) override;
  grpc::Status GetForumDetail(grpc::ServerContext*, const bbs::ForumDetailReq*, bbs::ForumDetailResp*) override;
  grpc::Status FollowForum(grpc::ServerContext*, const bbs::FollowForumReq*, bbs::FollowForumResp*) override;
  grpc::Status UnfollowForum(grpc::ServerContext*, const bbs::FollowForumReq*, bbs::FollowForumResp*) override;
  grpc::Status GetFollowedForums(grpc::ServerContext*, const bbs::FollowedForumsReq*, bbs::FollowedForumsResp*) override;

  grpc::Status ListPosts(grpc::ServerContext*, const bbs::PostsReq*, bbs::PostsResp*) override;
  grpc::Status GetPostDetail(grpc::ServerContext*, const bbs::PostDetailReq*, bbs::PostDetailResp*) override;
  grpc::Status CreatePost(grpc::ServerContext*, const bbs::CreatePostReq*, bbs::CreatePostResp*) override;
  grpc::Status UpdatePost(grpc::ServerContext*, const bbs::UpdatePostReq*, bbs::UpdatePostResp*) override;
  grpc::Status DeletePost(grpc::ServerContext*, const bbs::DeletePostReq*, bbs::DeletePostResp*) override;
  grpc::Status LikePost(grpc::ServerContext*, const bbs::LikePostReq*, bbs::LikePostResp*) override;
  grpc::Status UnlikePost(grpc::ServerContext*, const bbs::LikePostReq*, bbs::LikePostResp*) override;
  grpc::Status CollectPost(grpc::ServerContext*, const bbs::CollectPostReq*, bbs::CollectPostResp*) override;
  grpc::Status UncollectPost(grpc::ServerContext*, const bbs::CollectPostReq*, bbs::CollectPostResp*) override;
  grpc::Status SetTopPost(grpc::ServerContext*, const bbs::SetTopReq*, bbs::SetTopResp*) override;
  grpc::Status SetEssencePost(grpc::ServerContext*, const bbs::SetEssenceReq*, bbs::SetEssenceResp*) override;

  grpc::Status ListReplies(grpc::ServerContext*, const bbs::RepliesReq*, bbs::RepliesResp*) override;
  grpc::Status CreateReply(grpc::ServerContext*, const bbs::CreateReplyReq*, bbs::CreateReplyResp*) override;
  grpc::Status DeleteReply(grpc::ServerContext*, const bbs::DeleteReplyReq*, bbs::DeleteReplyResp*) override;
  grpc::Status LikeReply(grpc::ServerContext*, const bbs::LikeReplyReq*, bbs::LikeReplyResp*) override;
  grpc::Status UnlikeReply(grpc::ServerContext*, const bbs::LikeReplyReq*, bbs::LikeReplyResp*) override;

  grpc::Status Search(grpc::ServerContext*, const bbs::SearchReq*, bbs::SearchResp*) override;

  grpc::Status MyPosts(grpc::ServerContext*, const bbs::MyListReq*, bbs::MyPostsResp*) override;
  grpc::Status MyReplies(grpc::ServerContext*, const bbs::MyListReq*, bbs::MyRepliesResp*) override;
  grpc::Status MyCollections(grpc::ServerContext*, const bbs::MyListReq*, bbs::MyCollectionsResp*) override;
  grpc::Status MyFollowers(grpc::ServerContext*, const bbs::MyListReq*, bbs::MyUsersResp*) override;
  grpc::Status MyFollowing(grpc::ServerContext*, const bbs::MyListReq*, bbs::MyUsersResp*) override;

private:
  bool CheckToken(int uid, const std::string& token);
};
