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
