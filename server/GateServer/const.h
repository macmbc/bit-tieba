#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include <unordered_map>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "Singleton.h"
#include <assert.h>
#include <queue>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/exception.h>
#include <iostream>
#include <functional>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <string>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

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

#define CODEPREFIX  "code_"
