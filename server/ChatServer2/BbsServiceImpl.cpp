#include "BbsServiceImpl.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"
#include "../common/bbs/ForumDao.h"
#include "../common/bbs/PostDao.h"
#include "../common/bbs/ReplyDao.h"
#include "../common/bbs/SearchDao.h"
#include "../common/bbs/UserContentDao.h"
#include <unordered_set>

using grpc::ServerContext;

bool BbsServiceImpl::CheckToken(int uid, const std::string& token) {
  std::string token_value;
  return RedisMgr::GetInstance()->Get(USERTOKENPREFIX + std::to_string(uid), token_value) && token_value == token;
}

grpc::Status BbsServiceImpl::GetForums(ServerContext*, const bbs::ForumsReq* req, bbs::ForumsResp* resp) {
  int page = req->page() > 0 ? req->page() : 1;
  int limit = req->limit() > 0 ? req->limit() : 20;
  std::vector<ForumInfo> forums;
  if (!MysqlMgr::GetInstance()->ListForums(page, limit, forums)) {
    resp->set_error(ErrorCodes::DbError);
    return grpc::Status::OK;
  }
  resp->set_error(ErrorCodes::Success);
  for (auto& f : forums) {
    auto* item = resp->add_forums();
    item->set_forum_id(f.forum_id);
    item->set_name(f.name);
    item->set_avatar(f.avatar);
    item->set_description(f.description);
    item->set_follower_count(f.follower_count);
    item->set_post_count(f.post_count);
  }
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::GetForumDetail(ServerContext*, const bbs::ForumDetailReq* req, bbs::ForumDetailResp* resp) {
  ForumInfo info;
  if (!MysqlMgr::GetInstance()->GetForum(req->forum_id(), info)) {
    resp->set_error(ErrorCodes::ForumNotFound);
    return grpc::Status::OK;
  }
  bool is_followed = false;
  if (req->uid() > 0 && !req->token().empty() && CheckToken(req->uid(), req->token())) {
    bool b_follow = false;
    MysqlMgr::GetInstance()->IsFollowed(req->uid(), req->forum_id(), b_follow);
    is_followed = b_follow;
  }
  resp->set_error(ErrorCodes::Success);
  auto* f = resp->mutable_forum();
  f->set_forum_id(info.forum_id);
  f->set_name(info.name);
  f->set_avatar(info.avatar);
  f->set_description(info.description);
  f->set_follower_count(info.follower_count);
  f->set_post_count(info.post_count);
  f->set_is_followed(is_followed);
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::FollowForum(ServerContext*, const bbs::FollowForumReq* req, bbs::FollowForumResp* resp) {
  if (!CheckToken(req->uid(), req->token())) {
    resp->set_error(ErrorCodes::TokenInvalid);
    return grpc::Status::OK;
  }
  int ret = MysqlMgr::GetInstance()->FollowForum(req->uid(), req->forum_id());
  if (ret == 0) resp->set_error(ErrorCodes::Success);
  else if (ret == 1) resp->set_error(ErrorCodes::AlreadyFollowed);
  else if (ret == 2) resp->set_error(ErrorCodes::ForumNotFound);
  else resp->set_error(ErrorCodes::DbError);
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::UnfollowForum(ServerContext*, const bbs::FollowForumReq* req, bbs::FollowForumResp* resp) {
  if (!CheckToken(req->uid(), req->token())) {
    resp->set_error(ErrorCodes::TokenInvalid);
    return grpc::Status::OK;
  }
  int ret = MysqlMgr::GetInstance()->UnfollowForum(req->uid(), req->forum_id());
  if (ret == 0) resp->set_error(ErrorCodes::Success);
  else if (ret == 1) resp->set_error(ErrorCodes::NotFollowed);
  else if (ret == 2) resp->set_error(ErrorCodes::ForumNotFound);
  else resp->set_error(ErrorCodes::DbError);
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::GetFollowedForums(ServerContext*, const bbs::FollowedForumsReq* req, bbs::FollowedForumsResp* resp) {
  if (!CheckToken(req->uid(), req->token())) {
    resp->set_error(ErrorCodes::TokenInvalid);
    return grpc::Status::OK;
  }
  int page = req->page() > 0 ? req->page() : 1;
  int limit = req->limit() > 0 ? req->limit() : 20;
  std::vector<ForumInfo> forums;
  if (!MysqlMgr::GetInstance()->ListFollowedForums(req->uid(), page, limit, forums)) {
    resp->set_error(ErrorCodes::DbError);
    return grpc::Status::OK;
  }
  resp->set_error(ErrorCodes::Success);
  for (auto& f : forums) {
    auto* item = resp->add_forums();
    item->set_forum_id(f.forum_id);
    item->set_name(f.name);
    item->set_avatar(f.avatar);
    item->set_description(f.description);
    item->set_follower_count(f.follower_count);
    item->set_post_count(f.post_count);
  }
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::ListPosts(ServerContext*, const bbs::PostsReq* req, bbs::PostsResp* resp) {
  ForumInfo finfo;
  if (!MysqlMgr::GetInstance()->GetForum(req->forum_id(), finfo)) {
    resp->set_error(ErrorCodes::ForumNotFound);
    return grpc::Status::OK;
  }
  int page = req->page() > 0 ? req->page() : 1;
  int limit = req->limit() > 0 ? req->limit() : 20;
  std::vector<PostSummary> posts;
  if (!MysqlMgr::GetInstance()->ListPosts(req->forum_id(), page, limit, req->sort(), posts)) {
    resp->set_error(ErrorCodes::DbError);
    return grpc::Status::OK;
  }
  resp->set_error(ErrorCodes::Success);
  for (auto& p : posts) {
    auto* item = resp->add_posts();
    item->set_post_id(p.post_id);
    item->set_forum_id(p.forum_id);
    item->set_uid(p.uid);
    item->set_author(p.author);
    item->set_title(p.title);
    item->set_created_at(p.created_at);
    item->set_reply_count(p.reply_cnt);
    item->set_like_count(p.like_cnt);
    item->set_is_top(p.is_top);
    item->set_is_essence(p.is_essence);
    item->set_content_preview(p.content_preview);
    item->set_forum_name(p.forum_name);
  }
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::GetPostDetail(ServerContext*, const bbs::PostDetailReq* req, bbs::PostDetailResp* resp) {
  PostDetail detail;
  if (!MysqlMgr::GetInstance()->GetPostDetail(req->post_id(), detail)) {
    resp->set_error(ErrorCodes::PostNotFound);
    return grpc::Status::OK;
  }
  bool is_liked = false;
  bool is_collected = false;
  if (req->uid() > 0 && !req->token().empty() && CheckToken(req->uid(), req->token())) {
    MysqlMgr::GetInstance()->IsLiked(req->uid(), req->post_id(), is_liked);
    MysqlMgr::GetInstance()->IsCollected(req->uid(), req->post_id(), is_collected);
  }
  resp->set_error(ErrorCodes::Success);
  auto* p = resp->mutable_post();
  p->set_post_id(detail.post_id);
  p->set_forum_id(detail.forum_id);
  p->set_uid(detail.uid);
  p->set_author(detail.author);
  p->set_title(detail.title);
  p->set_content(detail.content);
  p->set_created_at(detail.created_at);
  p->set_reply_count(detail.reply_cnt);
  p->set_like_count(detail.like_cnt);
  p->set_collect_count(detail.collect_cnt);
  p->set_is_top(detail.is_top);
  p->set_is_essence(detail.is_essence);
  p->set_is_liked(is_liked);
  p->set_is_collected(is_collected);
  p->set_forum_name(detail.forum_name);
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::CreatePost(ServerContext*, const bbs::CreatePostReq* req, bbs::CreatePostResp* resp) {
  if (!CheckToken(req->uid(), req->token())) {
    resp->set_error(ErrorCodes::TokenInvalid);
    return grpc::Status::OK;
  }
  ForumInfo finfo;
  if (!MysqlMgr::GetInstance()->GetForum(req->forum_id(), finfo)) {
    resp->set_error(ErrorCodes::ForumNotFound);
    return grpc::Status::OK;
  }
  int post_id = 0;
  if (!MysqlMgr::GetInstance()->CreatePost(req->uid(), req->forum_id(), req->title(), req->content(), post_id)) {
    resp->set_error(ErrorCodes::DbError);
    return grpc::Status::OK;
  }
  resp->set_error(ErrorCodes::Success);
  resp->set_post_id(post_id);
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::UpdatePost(ServerContext*, const bbs::UpdatePostReq* req, bbs::UpdatePostResp* resp) {
  if (!CheckToken(req->uid(), req->token())) {
    resp->set_error(ErrorCodes::TokenInvalid);
    return grpc::Status::OK;
  }
  int owner_uid = 0;
  int forum_id = 0;
  if (!MysqlMgr::GetInstance()->GetPostOwner(req->post_id(), owner_uid, forum_id)) {
    resp->set_error(ErrorCodes::PostNotFound);
    return grpc::Status::OK;
  }
  if (req->uid() != owner_uid) {
    resp->set_error(ErrorCodes::NoPermission);
    return grpc::Status::OK;
  }
  if (!MysqlMgr::GetInstance()->UpdatePost(req->post_id(), req->uid(), req->title(), req->content())) {
    resp->set_error(ErrorCodes::DbError);
    return grpc::Status::OK;
  }
  resp->set_error(ErrorCodes::Success);
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::DeletePost(ServerContext*, const bbs::DeletePostReq* req, bbs::DeletePostResp* resp) {
  if (!CheckToken(req->uid(), req->token())) {
    resp->set_error(ErrorCodes::TokenInvalid);
    return grpc::Status::OK;
  }
  int owner_uid = 0;
  int forum_id = 0;
  if (!MysqlMgr::GetInstance()->GetPostOwner(req->post_id(), owner_uid, forum_id)) {
    resp->set_error(ErrorCodes::PostNotFound);
    return grpc::Status::OK;
  }
  ForumInfo finfo;
  if (!MysqlMgr::GetInstance()->GetForum(forum_id, finfo)) {
    resp->set_error(ErrorCodes::ForumNotFound);
    return grpc::Status::OK;
  }
  if (req->uid() != owner_uid && req->uid() != finfo.owner_uid) {
    resp->set_error(ErrorCodes::NoPermission);
    return grpc::Status::OK;
  }
  if (!MysqlMgr::GetInstance()->DeletePost(req->post_id(), forum_id)) {
    resp->set_error(ErrorCodes::DbError);
    return grpc::Status::OK;
  }
  resp->set_error(ErrorCodes::Success);
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::LikePost(ServerContext*, const bbs::LikePostReq* req, bbs::LikePostResp* resp) {
  if (!CheckToken(req->uid(), req->token())) {
    resp->set_error(ErrorCodes::TokenInvalid);
    return grpc::Status::OK;
  }
  int ret = MysqlMgr::GetInstance()->LikePost(req->uid(), req->post_id());
  if (ret == 0) resp->set_error(ErrorCodes::Success);
  else if (ret == 1) resp->set_error(ErrorCodes::AlreadyLiked);
  else if (ret == 2) resp->set_error(ErrorCodes::PostNotFound);
  else resp->set_error(ErrorCodes::DbError);
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::UnlikePost(ServerContext*, const bbs::LikePostReq* req, bbs::LikePostResp* resp) {
  if (!CheckToken(req->uid(), req->token())) {
    resp->set_error(ErrorCodes::TokenInvalid);
    return grpc::Status::OK;
  }
  int ret = MysqlMgr::GetInstance()->UnlikePost(req->uid(), req->post_id());
  if (ret == 0) resp->set_error(ErrorCodes::Success);
  else if (ret == 1) resp->set_error(ErrorCodes::NotLiked);
  else if (ret == 2) resp->set_error(ErrorCodes::PostNotFound);
  else resp->set_error(ErrorCodes::DbError);
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::CollectPost(ServerContext*, const bbs::CollectPostReq* req, bbs::CollectPostResp* resp) {
  if (!CheckToken(req->uid(), req->token())) {
    resp->set_error(ErrorCodes::TokenInvalid);
    return grpc::Status::OK;
  }
  int ret = MysqlMgr::GetInstance()->CollectPost(req->uid(), req->post_id());
  if (ret == 0) resp->set_error(ErrorCodes::Success);
  else if (ret == 1) resp->set_error(ErrorCodes::AlreadyCollected);
  else if (ret == 2) resp->set_error(ErrorCodes::PostNotFound);
  else resp->set_error(ErrorCodes::DbError);
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::UncollectPost(ServerContext*, const bbs::CollectPostReq* req, bbs::CollectPostResp* resp) {
  if (!CheckToken(req->uid(), req->token())) {
    resp->set_error(ErrorCodes::TokenInvalid);
    return grpc::Status::OK;
  }
  int ret = MysqlMgr::GetInstance()->UncollectPost(req->uid(), req->post_id());
  if (ret == 0) resp->set_error(ErrorCodes::Success);
  else if (ret == 1) resp->set_error(ErrorCodes::NotCollected);
  else if (ret == 2) resp->set_error(ErrorCodes::PostNotFound);
  else resp->set_error(ErrorCodes::DbError);
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::SetTopPost(ServerContext*, const bbs::SetTopReq* req, bbs::SetTopResp* resp) {
  if (!CheckToken(req->uid(), req->token())) {
    resp->set_error(ErrorCodes::TokenInvalid);
    return grpc::Status::OK;
  }
  int owner_uid = 0;
  int forum_id = 0;
  if (!MysqlMgr::GetInstance()->GetPostOwner(req->post_id(), owner_uid, forum_id)) {
    resp->set_error(ErrorCodes::PostNotFound);
    return grpc::Status::OK;
  }
  ForumInfo finfo;
  if (!MysqlMgr::GetInstance()->GetForum(forum_id, finfo)) {
    resp->set_error(ErrorCodes::ForumNotFound);
    return grpc::Status::OK;
  }
  if (req->uid() != finfo.owner_uid) {
    resp->set_error(ErrorCodes::NoPermission);
    return grpc::Status::OK;
  }
  if (!MysqlMgr::GetInstance()->SetTop(req->post_id(), req->is_top())) {
    resp->set_error(ErrorCodes::DbError);
    return grpc::Status::OK;
  }
  resp->set_error(ErrorCodes::Success);
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::SetEssencePost(ServerContext*, const bbs::SetEssenceReq* req, bbs::SetEssenceResp* resp) {
  if (!CheckToken(req->uid(), req->token())) {
    resp->set_error(ErrorCodes::TokenInvalid);
    return grpc::Status::OK;
  }
  int owner_uid = 0;
  int forum_id = 0;
  if (!MysqlMgr::GetInstance()->GetPostOwner(req->post_id(), owner_uid, forum_id)) {
    resp->set_error(ErrorCodes::PostNotFound);
    return grpc::Status::OK;
  }
  ForumInfo finfo;
  if (!MysqlMgr::GetInstance()->GetForum(forum_id, finfo)) {
    resp->set_error(ErrorCodes::ForumNotFound);
    return grpc::Status::OK;
  }
  if (req->uid() != finfo.owner_uid) {
    resp->set_error(ErrorCodes::NoPermission);
    return grpc::Status::OK;
  }
  if (!MysqlMgr::GetInstance()->SetEssence(req->post_id(), req->is_essence())) {
    resp->set_error(ErrorCodes::DbError);
    return grpc::Status::OK;
  }
  resp->set_error(ErrorCodes::Success);
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::ListReplies(ServerContext*, const bbs::RepliesReq* req, bbs::RepliesResp* resp) {
  PostDetail detail;
  if (!MysqlMgr::GetInstance()->GetPostDetail(req->post_id(), detail)) {
    resp->set_error(ErrorCodes::PostNotFound);
    return grpc::Status::OK;
  }
  int page = req->page() > 0 ? req->page() : 1;
  int limit = req->limit() > 0 ? req->limit() : 20;
  std::vector<ReplyInfo> tops;
  if (!MysqlMgr::GetInstance()->ListTopReplies(req->post_id(), page, limit, req->sort(), tops)) {
    resp->set_error(ErrorCodes::DbError);
    return grpc::Status::OK;
  }
  std::vector<long long> root_ids;
  for (auto& t : tops) root_ids.push_back(t.reply_id);
  std::unordered_map<long long, std::vector<ReplyInfo>> children;
  MysqlMgr::GetInstance()->ListChildrenReplies(req->post_id(), root_ids, children);

  bool authed = (req->uid() > 0 && !req->token().empty() && CheckToken(req->uid(), req->token()));
  std::unordered_set<long long> liked_set;
  if (authed) {
    std::vector<long long> all_ids;
    for (auto& t : tops) all_ids.push_back(t.reply_id);
    for (auto& kv : children) {
      for (auto& c : kv.second) all_ids.push_back(c.reply_id);
    }
    MysqlMgr::GetInstance()->BatchReplyLiked(req->uid(), all_ids, liked_set);
  }

  resp->set_error(ErrorCodes::Success);
  for (auto& t : tops) {
    auto* item = resp->add_replies();
    item->set_reply_id(t.reply_id);
    item->set_post_id(t.post_id);
    item->set_uid(t.uid);
    item->set_content(t.content);
    item->set_parent_reply_id(t.parent_reply_id);
    item->set_floor(t.floor);
    item->set_like_count(t.like_cnt);
    item->set_created_at(t.created_at);
    item->set_is_liked(authed && liked_set.count(t.reply_id) > 0);
    item->set_author(t.author);
    for (auto& c : children[t.reply_id]) {
      auto* sub = item->add_sub_replies();
      sub->set_reply_id(c.reply_id);
      sub->set_post_id(c.post_id);
      sub->set_uid(c.uid);
      sub->set_content(c.content);
      sub->set_parent_reply_id(c.parent_reply_id);
      sub->set_root_reply_id(c.root_reply_id);
      sub->set_floor(c.floor);
      sub->set_like_count(c.like_cnt);
      sub->set_created_at(c.created_at);
      sub->set_is_liked(authed && liked_set.count(c.reply_id) > 0);
      sub->set_author(c.author);
    }
  }
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::CreateReply(ServerContext*, const bbs::CreateReplyReq* req, bbs::CreateReplyResp* resp) {
  if (!CheckToken(req->uid(), req->token())) {
    resp->set_error(ErrorCodes::TokenInvalid);
    return grpc::Status::OK;
  }
  PostDetail detail;
  if (!MysqlMgr::GetInstance()->GetPostDetail(req->post_id(), detail)) {
    resp->set_error(ErrorCodes::PostNotFound);
    return grpc::Status::OK;
  }
  long long reply_id = 0;
  long long root_reply_id = 0;
  int floor = 0;
  if (!MysqlMgr::GetInstance()->CreateReply(req->uid(), req->post_id(), req->content(), req->parent_reply_id(), reply_id, root_reply_id, floor)) {
    resp->set_error(ErrorCodes::DbError);
    return grpc::Status::OK;
  }
  resp->set_error(ErrorCodes::Success);
  resp->set_reply_id(reply_id);
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::DeleteReply(ServerContext*, const bbs::DeleteReplyReq* req, bbs::DeleteReplyResp* resp) {
  if (!CheckToken(req->uid(), req->token())) {
    resp->set_error(ErrorCodes::TokenInvalid);
    return grpc::Status::OK;
  }
  int owner_uid = 0;
  int post_id = 0;
  long long parent_reply_id = 0;
  long long root_reply_id = 0;
  if (!MysqlMgr::GetInstance()->GetReplyOwner(req->reply_id(), owner_uid, post_id, parent_reply_id, root_reply_id)) {
    resp->set_error(ErrorCodes::ReplyNotFound);
    return grpc::Status::OK;
  }
  int post_owner = 0;
  int forum_id = 0;
  if (!MysqlMgr::GetInstance()->GetPostOwner(post_id, post_owner, forum_id)) {
    resp->set_error(ErrorCodes::PostNotFound);
    return grpc::Status::OK;
  }
  ForumInfo finfo;
  if (!MysqlMgr::GetInstance()->GetForum(forum_id, finfo)) {
    resp->set_error(ErrorCodes::ForumNotFound);
    return grpc::Status::OK;
  }
  if (req->uid() != owner_uid && req->uid() != post_owner && req->uid() != finfo.owner_uid) {
    resp->set_error(ErrorCodes::NoPermission);
    return grpc::Status::OK;
  }
  if (!MysqlMgr::GetInstance()->DeleteReply(req->reply_id(), post_id)) {
    resp->set_error(ErrorCodes::DbError);
    return grpc::Status::OK;
  }
  resp->set_error(ErrorCodes::Success);
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::LikeReply(ServerContext*, const bbs::LikeReplyReq* req, bbs::LikeReplyResp* resp) {
  if (!CheckToken(req->uid(), req->token())) {
    resp->set_error(ErrorCodes::TokenInvalid);
    return grpc::Status::OK;
  }
  int ret = MysqlMgr::GetInstance()->LikeReply(req->uid(), req->reply_id());
  if (ret == 0) resp->set_error(ErrorCodes::Success);
  else if (ret == 1) resp->set_error(ErrorCodes::AlreadyLikedReply);
  else if (ret == 2) resp->set_error(ErrorCodes::ReplyNotFound);
  else resp->set_error(ErrorCodes::DbError);
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::UnlikeReply(ServerContext*, const bbs::LikeReplyReq* req, bbs::LikeReplyResp* resp) {
  if (!CheckToken(req->uid(), req->token())) {
    resp->set_error(ErrorCodes::TokenInvalid);
    return grpc::Status::OK;
  }
  int ret = MysqlMgr::GetInstance()->UnlikeReply(req->uid(), req->reply_id());
  if (ret == 0) resp->set_error(ErrorCodes::Success);
  else if (ret == 1) resp->set_error(ErrorCodes::NotLikedReply);
  else if (ret == 2) resp->set_error(ErrorCodes::ReplyNotFound);
  else resp->set_error(ErrorCodes::DbError);
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::Search(ServerContext*, const bbs::SearchReq* req, bbs::SearchResp* resp) {
  if (req->keyword().empty()) {
    resp->set_error(ErrorCodes::Error_Json);
    return grpc::Status::OK;
  }
  int page = req->page() > 0 ? req->page() : 1;
  int limit = req->limit() > 0 ? req->limit() : 20;
  if (req->type() == "post" || req->type().empty()) {
    std::vector<PostSummary> posts;
    if (!MysqlMgr::GetInstance()->SearchPosts(req->keyword(), req->forum_id(), page, limit, posts)) {
      resp->set_error(ErrorCodes::DbError);
      return grpc::Status::OK;
    }
    resp->set_error(ErrorCodes::Success);
    for (auto& p : posts) {
      auto* item = resp->add_posts();
      item->set_post_id(p.post_id);
      item->set_forum_id(p.forum_id);
      item->set_uid(p.uid);
      item->set_author(p.author);
      item->set_title(p.title);
      item->set_created_at(p.created_at);
      item->set_reply_count(p.reply_cnt);
      item->set_like_count(p.like_cnt);
      item->set_is_top(p.is_top);
      item->set_is_essence(p.is_essence);
      item->set_content_preview(p.content_preview);
      item->set_forum_name(p.forum_name);
    }
  }
  else if (req->type() == "forum") {
    std::vector<ForumInfo> forums;
    if (!MysqlMgr::GetInstance()->SearchForums(req->keyword(), page, limit, forums)) {
      resp->set_error(ErrorCodes::DbError);
      return grpc::Status::OK;
    }
    resp->set_error(ErrorCodes::Success);
    for (auto& f : forums) {
      auto* item = resp->add_forums();
      item->set_forum_id(f.forum_id);
      item->set_name(f.name);
      item->set_avatar(f.avatar);
      item->set_description(f.description);
      item->set_follower_count(f.follower_count);
      item->set_post_count(f.post_count);
    }
  }
  else if (req->type() == "user") {
    std::vector<UserBrief> users;
    if (!MysqlMgr::GetInstance()->SearchUsers(req->keyword(), page, limit, users)) {
      resp->set_error(ErrorCodes::DbError);
      return grpc::Status::OK;
    }
    resp->set_error(ErrorCodes::Success);
    for (auto& u : users) {
      auto* item = resp->add_users();
      item->set_uid(u.uid);
      item->set_name(u.name);
      item->set_nick(u.nick);
      item->set_email(u.email);
      item->set_icon(u.icon);
      item->set_sex(u.sex);
    }
  }
  else {
    resp->set_error(ErrorCodes::Error_Json);
  }
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::MyPosts(ServerContext*, const bbs::MyListReq* req, bbs::MyPostsResp* resp) {
  if (!CheckToken(req->uid(), req->token())) {
    resp->set_error(ErrorCodes::TokenInvalid);
    return grpc::Status::OK;
  }
  int page = req->page() > 0 ? req->page() : 1;
  int limit = req->limit() > 0 ? req->limit() : 20;
  std::vector<PostSummary> posts;
  if (!MysqlMgr::GetInstance()->ListMyPosts(req->uid(), page, limit, posts)) {
    resp->set_error(ErrorCodes::DbError);
    return grpc::Status::OK;
  }
  resp->set_error(ErrorCodes::Success);
  for (auto& p : posts) {
    auto* item = resp->add_posts();
    item->set_post_id(p.post_id);
    item->set_forum_id(p.forum_id);
    item->set_uid(p.uid);
    item->set_author(p.author);
    item->set_title(p.title);
    item->set_created_at(p.created_at);
    item->set_reply_count(p.reply_cnt);
    item->set_like_count(p.like_cnt);
    item->set_is_top(p.is_top);
    item->set_is_essence(p.is_essence);
    item->set_content_preview(p.content_preview);
    item->set_forum_name(p.forum_name);
  }
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::MyReplies(ServerContext*, const bbs::MyListReq* req, bbs::MyRepliesResp* resp) {
  if (!CheckToken(req->uid(), req->token())) {
    resp->set_error(ErrorCodes::TokenInvalid);
    return grpc::Status::OK;
  }
  int page = req->page() > 0 ? req->page() : 1;
  int limit = req->limit() > 0 ? req->limit() : 20;
  std::vector<ReplyWithPost> replies;
  if (!MysqlMgr::GetInstance()->ListMyReplies(req->uid(), page, limit, replies)) {
    resp->set_error(ErrorCodes::DbError);
    return grpc::Status::OK;
  }
  resp->set_error(ErrorCodes::Success);
  for (auto& r : replies) {
    auto* item = resp->add_replies();
    item->set_reply_id(r.reply.reply_id);
    item->set_post_id(r.reply.post_id);
    item->set_uid(r.reply.uid);
    item->set_content(r.reply.content);
    item->set_parent_reply_id(r.reply.parent_reply_id);
    item->set_root_reply_id(r.reply.root_reply_id);
    item->set_floor(r.reply.floor);
    item->set_like_count(r.reply.like_cnt);
    item->set_created_at(r.reply.created_at);
    item->set_is_liked(false);
    item->set_author(r.reply.author);
    item->set_post_title(r.post_title);
  }
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::MyCollections(ServerContext*, const bbs::MyListReq* req, bbs::MyCollectionsResp* resp) {
  if (!CheckToken(req->uid(), req->token())) {
    resp->set_error(ErrorCodes::TokenInvalid);
    return grpc::Status::OK;
  }
  int page = req->page() > 0 ? req->page() : 1;
  int limit = req->limit() > 0 ? req->limit() : 20;
  std::vector<PostSummary> posts;
  if (!MysqlMgr::GetInstance()->ListMyCollections(req->uid(), page, limit, posts)) {
    resp->set_error(ErrorCodes::DbError);
    return grpc::Status::OK;
  }
  resp->set_error(ErrorCodes::Success);
  for (auto& p : posts) {
    auto* item = resp->add_posts();
    item->set_post_id(p.post_id);
    item->set_forum_id(p.forum_id);
    item->set_uid(p.uid);
    item->set_author(p.author);
    item->set_title(p.title);
    item->set_created_at(p.created_at);
    item->set_reply_count(p.reply_cnt);
    item->set_like_count(p.like_cnt);
    item->set_is_top(p.is_top);
    item->set_is_essence(p.is_essence);
    item->set_content_preview(p.content_preview);
    item->set_forum_name(p.forum_name);
  }
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::MyFollowers(ServerContext*, const bbs::MyListReq* req, bbs::MyUsersResp* resp) {
  if (!CheckToken(req->uid(), req->token())) {
    resp->set_error(ErrorCodes::TokenInvalid);
    return grpc::Status::OK;
  }
  int page = req->page() > 0 ? req->page() : 1;
  int limit = req->limit() > 0 ? req->limit() : 20;
  std::vector<UserBrief> users;
  if (!MysqlMgr::GetInstance()->ListMyFollowers(req->uid(), page, limit, users)) {
    resp->set_error(ErrorCodes::DbError);
    return grpc::Status::OK;
  }
  resp->set_error(ErrorCodes::Success);
  for (auto& u : users) {
    auto* item = resp->add_users();
    item->set_uid(u.uid);
    item->set_name(u.name);
    item->set_nick(u.nick);
    item->set_email(u.email);
    item->set_icon(u.icon);
    item->set_sex(u.sex);
  }
  return grpc::Status::OK;
}

grpc::Status BbsServiceImpl::MyFollowing(ServerContext*, const bbs::MyListReq* req, bbs::MyUsersResp* resp) {
  if (!CheckToken(req->uid(), req->token())) {
    resp->set_error(ErrorCodes::TokenInvalid);
    return grpc::Status::OK;
  }
  int page = req->page() > 0 ? req->page() : 1;
  int limit = req->limit() > 0 ? req->limit() : 20;
  std::vector<UserBrief> users;
  if (!MysqlMgr::GetInstance()->ListMyFollowing(req->uid(), page, limit, users)) {
    resp->set_error(ErrorCodes::DbError);
    return grpc::Status::OK;
  }
  resp->set_error(ErrorCodes::Success);
  for (auto& u : users) {
    auto* item = resp->add_users();
    item->set_uid(u.uid);
    item->set_name(u.name);
    item->set_nick(u.nick);
    item->set_email(u.email);
    item->set_icon(u.icon);
    item->set_sex(u.sex);
  }
  return grpc::Status::OK;
}
