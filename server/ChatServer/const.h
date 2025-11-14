#pragma once
#include <functional>

enum ErrorCodes {
    Success = 0,
    Error_Json = 1001,   // Json 解析失败
    RPCFailed = 1002,    // RPC 调用失败
    VarifyExpired = 1003, // 验证码过期
    VarifyCodeErr = 1004, // 验证码错误
    UserExist = 1005,     // 用户已存在
    PasswdErr = 1006,     // 密码错误
    EmailNotMatch = 1007, // 邮箱不匹配
    PasswdUpFailed = 1008, // 更新密码失败
    PasswdInvalid = 1009,  // 密码无效
    TokenInvalid = 1010,   // Token 失效
    UidInvalid = 1011,     // uid 无效
    // 贴吧相关错误码
    TiebaExist = 1012,     // 贴吧已存在
    TiebaNotExist = 1013,  // 贴吧不存在
    TiebaNameInvalid = 1014, // 贴吧名称无效
    NotTiebaMember = 1015,   // 不是贴吧成员
    NotTiebaOwner = 1016,    // 不是吧主
    NotTiebaAdmin = 1017,    // 不是管理员
    PostNotExist = 1018,     // 帖子不存在
    ReplyNotExist = 1019,    // 回复不存在
    PermissionDenied = 1020,  // 权限不足
};

// Defer 辅助类
class Defer {
public:
    // 构造时传入一个回调
    Defer(std::function<void()> func) : func_(func) {}

    // 析构时执行回调函数
    ~Defer() {
        func_();
    }

private:
    std::function<void()> func_;
};

#define MAX_LENGTH  (1024 * 2)
// 包头中“总长度”字段的字节数
#define HEAD_TOTAL_LEN 4
// 包头中“消息 ID”字段的字节数
#define HEAD_ID_LEN 2
// 包头中“数据长度”字段的字节数
#define HEAD_DATA_LEN 2
#define MAX_RECVQUE  10000
#define MAX_SENDQUE 1000

enum MSG_IDS {
    MSG_CHAT_LOGIN = 1005,             // 用户登录请求
    MSG_CHAT_LOGIN_RSP = 1006,         // 用户登录响应
    ID_SEARCH_USER_REQ = 1007,         // 查找用户请求
    ID_SEARCH_USER_RSP = 1008,         // 查找用户响应
    ID_ADD_FRIEND_REQ = 1009,          // 添加好友请求
    ID_ADD_FRIEND_RSP  = 1010,         // 添加好友响应
    ID_NOTIFY_ADD_FRIEND_REQ = 1011,   // 通知用户有好友申请
    ID_AUTH_FRIEND_REQ = 1013,         // 好友验证请求
    ID_AUTH_FRIEND_RSP = 1014,         // 好友验证响应
    ID_NOTIFY_AUTH_FRIEND_REQ = 1015,  // 通知用户有验证请求
    ID_TEXT_CHAT_MSG_REQ = 1017,       // 文本聊天消息请求
    ID_TEXT_CHAT_MSG_RSP = 1018,       // 文本聊天消息响应
    ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019, // 通知用户接收文本消息
    ID_NOTIFY_OFF_LINE_REQ = 1021,     // 通知用户下线
    ID_HEART_BEAT_REQ = 1023,          // 心跳请求
    ID_HEARTBEAT_RSP = 1024,           // 心跳响应
    // 贴吧相关消息ID
    ID_CREATE_TIEBA_REQ = 1025,        // 创建贴吧请求
    ID_CREATE_TIEBA_RSP = 1026,        // 创建贴吧响应
    ID_SEARCH_TIEBA_REQ = 1027,        // 搜索贴吧请求
    ID_SEARCH_TIEBA_RSP = 1028,        // 搜索贴吧响应
    ID_JOIN_TIEBA_REQ = 1029,          // 加入贴吧请求
    ID_JOIN_TIEBA_RSP = 1030,          // 加入贴吧响应
    ID_LEAVE_TIEBA_REQ = 1031,         // 退出贴吧请求
    ID_LEAVE_TIEBA_RSP = 1032,         // 退出贴吧响应
    ID_CREATE_POST_REQ = 1033,         // 发帖请求
    ID_CREATE_POST_RSP = 1034,         // 发帖响应
    ID_CREATE_REPLY_REQ = 1035,        // 回复请求
    ID_CREATE_REPLY_RSP = 1036,        // 回复响应
    ID_DELETE_POST_REQ = 1037,         // 删除帖子请求
    ID_DELETE_POST_RSP = 1038,         // 删除帖子响应
    ID_LIKE_REQ = 1039,                // 点赞请求
    ID_LIKE_RSP = 1040,                // 点赞响应
    ID_GET_POST_LIST_REQ = 1041,       // 获取帖子列表请求
    ID_GET_POST_LIST_RSP = 1042,       // 获取帖子列表响应
    ID_GET_REPLY_LIST_REQ = 1043,      // 获取回复列表请求
    ID_GET_REPLY_LIST_RSP = 1044,      // 获取回复列表响应
    ID_SET_TOP_REQ = 1045,             // 置顶请求
    ID_SET_TOP_RSP = 1046,             // 置顶响应
    ID_SET_ESSENCE_REQ = 1047,         // 加精请求
    ID_SET_ESSENCE_RSP = 1048,         // 加精响应
    ID_GET_TIEBA_INFO_REQ = 1049,      // 获取贴吧信息请求
    ID_GET_TIEBA_INFO_RSP = 1050,      // 获取贴吧信息响应
    ID_GET_MY_TIEBA_LIST_REQ = 1051,   // 获取我的贴吧列表请求
    ID_GET_MY_TIEBA_LIST_RSP = 1052,   // 获取我的贴吧列表响应
    ID_GET_TIEBA_MEMBER_LIST_REQ = 1053, // 获取贴吧成员列表请求
    ID_GET_TIEBA_MEMBER_LIST_RSP = 1054, // 获取贴吧成员列表响应
    ID_DELETE_REPLY_REQ = 1055,        // 删除回复请求
    ID_DELETE_REPLY_RSP = 1056,        // 删除回复响应
};

#define USERIPPREFIX  "uip_"
#define USERTOKENPREFIX  "utoken_"
#define IPCOUNTPREFIX  "ipcount_"
#define USER_BASE_INFO "ubaseinfo_"
#define LOGIN_COUNT  "logincount"
#define NAME_INFO  "nameinfo_"
#define LOCK_PREFIX "lock_"
#define USER_SESSION_PREFIX "usession_"
#define LOCK_COUNT "lockcount"

// 分布式锁的持有超时时间（秒）
#define LOCK_TIME_OUT 10
// 分布式锁的获取超时时间（秒）
#define ACQUIRE_TIME_OUT 5
