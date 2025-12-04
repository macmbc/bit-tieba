SERVER="http://39.102.149.84:8080"
LOG_FILE="error_test_$(date +%Y%m%d_%H%M%S).log"

echo "=== HTTP接口错误码测试 ===" | tee -a $LOG_FILE
echo "测试时间: $(date)" | tee -a $LOG_FILE
echo "==========================" | tee -a $LOG_FILE

# 记录函数
log_test() {
    echo "[$(date '+%H:%M:%S')] $1" | tee -a $LOG_FILE
}

test_error_case() {
    local test_name="$1"
    local method="$2"
    local endpoint="$3"
    local data="$4"
    local expected_code="$5"
    
    log_test "测试: $test_name (期望: $expected_code)"
    
    if [ "$method" = "GET" ]; then
        response=$(curl -s "$SERVER$endpoint")
    else
        response=$(curl -s -X $method -H "Content-Type: application/json" -d "$data" "$SERVER$endpoint")
    fi
    
    # 解析错误码
    actual_code=$(echo "$response" | python3 -c "
import sys, json
try:
    data = json.load(sys.stdin)
    print(data.get('error', -1))
except:
    print(-2)
" 2>/dev/null)
    
    log_test "响应: $response"
    
    if [ "$actual_code" = "$expected_code" ]; then
        log_test "✅ 通过: 期望 $expected_code, 实际 $actual_code"
        return 0
    else
        log_test "❌ 失败: 期望 $expected_code, 实际 $actual_code"
        return 1
    fi
}

echo ""
echo "1. 测试JSON解析错误 (1001)" | tee -a $LOG_FILE
echo "==========================================" | tee -a $LOG_FILE

# 1.1 无效的JSON格式
test_error_case "无效JSON格式" "POST" "/user_login" '{"invalid json' "1001"

# 1.2 缺少引号
test_error_case "缺少引号" "POST" "/user_login" '{email: "test@test.com", passwd: "123"}' "1001"

# 1.3 多余的逗号
test_error_case "多余逗号" "POST" "/user_login" '{"email": "test@test.com", "passwd": "123",}' "1001"

echo ""
echo "2. 测试用户登录相关错误" | tee -a $LOG_FILE
echo "==========================================" | tee -a $LOG_FILE

# 2.1 不存在的邮箱（应该是1009或1011）
test_error_case "不存在的邮箱" "POST" "/user_login" '{"email": "nonexistent@example.com", "passwd": "123456"}' "1009"

# 2.2 错误密码
test_error_case "错误密码" "POST" "/user_login" '{"email": "18165031775@qq.com", "passwd": "wrongpassword"}' "1009"

# 2.3 缺少必填字段
test_error_case "缺少email" "POST" "/user_login" '{"passwd": "123456"}' "1009"
test_error_case "缺少password" "POST" "/user_login" '{"email": "test@test.com"}' "1009"

# 2.4 空字段
test_error_case "空邮箱" "POST" "/user_login" '{"email": "", "passwd": "123456"}' "1009"
test_error_case "空密码" "POST" "/user_login" '{"email": "test@test.com", "passwd": ""}' "1009"

echo ""
echo "3. 测试贴吧相关错误" | tee -a $LOG_FILE
echo "==========================================" | tee -a $LOG_FILE

# 先登录获取一个有效token用于测试
log_test "登录获取有效token..."
LOGIN_RESPONSE=$(curl -s -X POST "$SERVER/user_login" \
  -H "Content-Type: application/json" \
  -d '{"email": "18165031775@qq.com", "passwd": "123456"}')

VALID_TOKEN=$(echo "$LOGIN_RESPONSE" | python3 -c "
import sys, json
try:
    data = json.load(sys.stdin)
    print(data.get('token', ''))
except:
    print('')
")

VALID_UID=$(echo "$LOGIN_RESPONSE" | python3 -c "
import sys, json
try:
    data = json.load(sys.stdin)
    print(data.get('uid', ''))
except:
    print('')
")

if [ -n "$VALID_TOKEN" ]; then
    log_test "获取到有效token: ${VALID_TOKEN:0:20}..."
    
    # 3.1 不存在的贴吧 (1012)
    test_error_case "不存在的贴吧ID" "GET" "/forum/detail?forum_id=999999&uid=$VALID_UID&token=$VALID_TOKEN" "" "1012"
    
    # 3.2 无效的贴吧ID格式
    test_error_case "非数字贴吧ID" "GET" "/forum/detail?forum_id=abc&uid=$VALID_UID&token=$VALID_TOKEN" "" "1001"
    
    # 3.3 负数的贴吧ID
    test_error_case "负数贴吧ID" "GET" "/forum/detail?forum_id=-1&uid=$VALID_UID&token=$VALID_TOKEN" "" "1012"
    
    # 3.4 重复关注（先关注，再关注一次）
    log_test "测试重复关注..."
    # 先关注一个存在的贴吧
    curl -s -X POST "$SERVER/follow_forum" \
      -H "Content-Type: application/json" \
      -d "{\"uid\": $VALID_UID, \"token\": \"$VALID_TOKEN\", \"forum_id\": 1}" > /dev/null
    
    # 再关注一次（应该成功，但我们可以检查数据库是否处理了重复）
    test_error_case "重复关注" "POST" "/follow_forum" "{\"uid\": $VALID_UID, \"token\": \"$VALID_TOKEN\", \"forum_id\": 1}" "1013"
    
    # 3.5 关注不存在的贴吧
    test_error_case "关注不存在的贴吧" "POST" "/follow_forum" "{\"uid\": $VALID_UID, \"token\": \"$VALID_TOKEN\", \"forum_id\":999 }" "1012"
    
    # 3.6 取消关注未关注的贴吧
    test_error_case "取消未关注的贴吧" "POST" "/unfollow_forum" "{\"uid\": $VALID_UID, \"token\": \"$VALID_TOKEN\", \"forum_id\":3 }" "1014"
    
else
    log_test "❌ 无法获取有效token，跳过贴吧测试"
fi

echo ""
echo "4. 测试Token相关错误 (1010)" | tee -a $LOG_FILE
echo "==========================================" | tee -a $LOG_FILE

# 4.1 无效Token
test_error_case "无效Token" "POST" "/follow_forum" '{"uid": 1001, "token": "invalid_token_123", "forum_id": 1}' "1010"

# 4.2 空Token
test_error_case "空Token" "POST" "/follow_forum" '{"uid": 1001, "token": "", "forum_id": 1}' "1001"

# 4.3 Token与UID不匹配
if [ -n "$VALID_TOKEN" ]; then
    test_error_case "Token与UID不匹配" "POST" "/follow_forum" "{\"uid\": 999999, \"token\": \"$VALID_TOKEN\", \"forum_id\": 1}" "1010"
fi

# 4.4 过期的Token（需要模拟token过期）
# 暂时跳过，因为需要知道token过期机制

echo ""
echo "5. 测试帖子相关错误" | tee -a $LOG_FILE
echo "==========================================" | tee -a $LOG_FILE

if [ -n "$VALID_TOKEN" ]; then
    # 5.1 获取不存在的帖子详情 (1016)
    test_error_case "不存在的帖子" "GET" "/post/detail?post_id=999999&uid=$VALID_UID&token=$VALID_TOKEN" "" "1016"
    
    # 5.2 在不存在贴吧发帖
    test_error_case "在不存在贴吧发帖" "POST" "/post/create" "{\"uid\": $VALID_UID, \"token\": \"$VALID_TOKEN\", \"forum_id\": 999999, \"title\": \"测试\", \"content\": \"测试内容\"}" "1012"
    
    # 5.3 更新不存在的帖子
    test_error_case "更新不存在的帖子" "POST" "/post/update" "{\"uid\": $VALID_UID, \"token\": \"$VALID_TOKEN\", \"post_id\": 999999, \"title\": \"新标题\", \"content\": \"新内容\"}" "1016"
    
    # 5.4 删除不存在的帖子
    test_error_case "删除不存在的帖子" "POST" "/post/delete" "{\"uid\": $VALID_UID, \"token\": \"$VALID_TOKEN\", \"post_id\": 999999}" "1016"
    
    # 5.5 无权限操作（非作者删除帖子）
    # 先创建一个帖子
    log_test "创建测试帖子用于权限测试..."
    CREATE_RESPONSE=$(curl -s -X POST "$SERVER/post/create" \
      -H "Content-Type: application/json" \
      -d "{\"uid\": $VALID_UID, \"token\": \"$VALID_TOKEN\", \"forum_id\": 1, \"title\": \"权限测试帖子\", \"content\": \"用于测试权限\"}")
    
    TEST_POST_ID=$(echo "$CREATE_RESPONSE" | python3 -c "
import sys, json
try:
    data = json.load(sys.stdin)
    print(data.get('post_id', 0))
except:
    print(0)
")
    
    if [ "$TEST_POST_ID" -gt 0 ]; then
        log_test "创建了测试帖子 ID: $TEST_POST_ID"
        
        # 使用另一个"用户"尝试删除（模拟无权限）
        test_error_case "无权限删除帖子" "POST" "/post/delete" "{\"uid\": 999999, \"token\": \"invalid_token\", \"post_id\": $TEST_POST_ID}" "1010"
        
        # 清理：删除测试帖子
        curl -s -X POST "$SERVER/post/delete" \
          -H "Content-Type: application/json" \
          -d "{\"uid\": $VALID_UID, \"token\": \"$VALID_TOKEN\", \"post_id\": $TEST_POST_ID}" > /dev/null
    fi
    
    # 5.6 置顶/加精权限测试（非贴吧owner）
    # 先获取一个帖子ID
    POSTS_RESPONSE=$(curl -s "$SERVER/posts?forum_id=1&page=1&limit=1")
    SAMPLE_POST_ID=$(echo "$POSTS_RESPONSE" | python3 -c "
import sys, json
try:
    data = json.load(sys.stdin)
    posts = data.get('posts', [])
    if posts:
        print(posts[0].get('post_id', 0))
    else:
        print(0)
except:
    print(0)
")
    
    if [ "$SAMPLE_POST_ID" -gt 0 ]; then
        test_error_case "非贴吧owner置顶" "POST" "/set_top_post" "{\"uid\": $VALID_UID, \"token\": \"$VALID_TOKEN\", \"post_id\": $SAMPLE_POST_ID, \"is_top\": true}" "1017"
        test_error_case "非贴吧owner加精" "POST" "/set_essence_post" "{\"uid\": $VALID_UID, \"token\": \"$VALID_TOKEN\", \"post_id\": $SAMPLE_POST_ID, \"is_essence\": true}" "1017"
    fi
fi

echo ""
echo "6. 测试回复相关错误" | tee -a $LOG_FILE
echo "==========================================" | tee -a $LOG_FILE

if [ -n "$VALID_TOKEN" ]; then
    # 6.1 回复不存在的帖子
    test_error_case "回复不存在的帖子" "POST" "/reply" "{\"uid\": $VALID_UID, \"token\": \"$VALID_TOKEN\", \"post_id\": 999999, \"content\": \"测试回复\", \"parent_reply_id\": 0}" "1016"
    
    # 6.2 回复不存在的父回复
    if [ "$SAMPLE_POST_ID" -gt 0 ]; then
        test_error_case "回复不存在的父回复" "POST" "/reply" "{\"uid\": $VALID_UID, \"token\": \"$VALID_TOKEN\", \"post_id\": $SAMPLE_POST_ID, \"content\": \"测试回复\", \"parent_reply_id\": 999}" "1015"
    fi
    
    # 6.3 删除不存在的回复
    test_error_case "删除不存在的回复" "POST" "/reply/delete" "{\"uid\": $VALID_UID, \"token\": \"$VALID_TOKEN\", \"reply_id\": 150}" "1022"
fi

echo ""
echo "7. 测试搜索功能错误" | tee -a $LOG_FILE
echo "==========================================" | tee -a $LOG_FILE

# 7.1 空关键词搜索
test_error_case "空关键词搜索" "GET" "/search?keyword=&type=post&page=1&limit=10" "" "1001"  # 返回错误

# 7.2 无效的搜索类型
test_error_case "无效搜索类型" "GET" "/search?keyword=test&type=invalid&page=1&limit=10" "" "1001"  # 返回错误

echo ""
echo "8. 测试用户内容管理错误" | tee -a $LOG_FILE
echo "==========================================" | tee -a $LOG_FILE

# 8.1 无效Token查看用户内容
test_error_case "无效Token查看我的帖子" "GET" "/my/posts?uid=1001&token=invalid_token&page=1&limit=10" "" "1010"

# 8.2 无权限查看他人内容（如果能测的话）
# 这个可能需要具体实现支持

echo ""
echo "9. 测试注册相关错误" | tee -a $LOG_FILE
echo "==========================================" | tee -a $LOG_FILE

# 9.1 用户已存在 (1005)
# 需要知道一个已存在的用户名和邮箱
test_error_case "用户名已存在" "POST" "/user_register" '{"email": "secondtonone1@163.com", "user": "tc", "passwd": "123456", "confirm": "123456", "icon": "", "varifycode": "123456"}' "1003"

test_error_case "邮箱已存在" "POST" "/user_register" '{"email": "18165031775@qq.com", "user": "newusersss", "passwd": "123456", "confirm": "123456", "icon": "", "varifycode": "123456"}' "1003"

# 9.2 两次密码不一致 (1006)
test_error_case "密码不一致" "POST" "/user_register" '{"email": "testregister@example.com", "user": "testregister", "passwd": "123456", "confirm": "654321", "icon": "", "varifycode": "123456"}' "1006"

# 9.3 验证码错误 (1004)
test_error_case "验证码错误" "POST" "/user_register" '{"email": "testregister@example.com", "user": "testregister", "passwd": "123456", "confirm": "123456", "icon": "", "varifycode": "wrongcode"}' "1003"

# 9.4 验证码过期 (1003)
# 需要有一个过期的验证码，暂时跳过

echo ""
echo "10. 边界值和异常输入测试" | tee -a $LOG_FILE
echo "==========================================" | tee -a $LOG_FILE

# 10.1 超长输入
LONG_STRING=$(printf '%*s' 10000 | tr ' ' 'x')
test_error_case "超长标题" "POST" "/post/create" "{\"uid\": $VALID_UID, \"token\": \"$VALID_TOKEN\", \"forum_id\": 1, \"title\": \"$LONG_STRING\", \"content\": \"测试内容\"}" "1015"  # 可能成功或失败

# 10.2 SQL注入尝试（应该被参数化查询阻止）
test_error_case "SQL注入标题" "POST" "/post/create" "{\"uid\": $VALID_UID, \"token\": \"$VALID_TOKEN\", \"forum_id\": 1, \"title\": \"test'); DROP TABLE forum; --\", \"content\": \"测试内容\"}" "0"

# 10.3 XSS尝试
test_error_case "XSS内容" "POST" "/post/create" "{\"uid\": $VALID_UID, \"token\": \"$VALID_TOKEN\", \"forum_id\": 1, \"title\": \"测试标题\", \"content\": \"<script>alert('xss')</script>\"}" "0"

# 10.4 特殊字符
test_error_case "特殊字符" "POST" "/post/create" "{\"uid\": $VALID_UID, \"token\": \"$VALID_TOKEN\", \"forum_id\": 1, \"title\": \"测试!@#$%^&*()标题\", \"content\": \"内容!@#$%^&*()\"}" "0"

echo ""
echo "11. 测试分页参数错误" | tee -a $LOG_FILE
echo "==========================================" | tee -a $LOG_FILE

# 11.1 无效的分页参数
test_error_case "负数的页码" "GET" "/forums?page=-1&limit=10" "" "0"
test_error_case "零页码" "GET" "/forums?page=0&limit=10" "" "0"
test_error_case "负数的每页数量" "GET" "/forums?page=1&limit=-1" "" "0"
test_error_case "零每页数量" "GET" "/forums?page=1&limit=0" "" "0"
test_error_case "超大每页数量" "GET" "/forums?page=1&limit=10000" "" "0"

# 11.2 非数字参数
test_error_case "非数字页码" "GET" "/forums?page=abc&limit=10" "" "1001"
test_error_case "非数字每页数量" "GET" "/forums?page=1&limit=abc" "" "1001"

echo ""
echo "=== 测试总结 ===" | tee -a $LOG_FILE
echo "详细日志已保存到: $LOG_FILE" | tee -a $LOG_FILE
echo "==========================" | tee -a $LOG_FILE

# 显示错误码统计
echo ""
echo "📊 测试的错误码分布:"
grep -o '"error":[0-9]*' "$LOG_FILE" | sed 's/"error"://' | sort | uniq -c | sort -rn | tee -a $LOG_FILE

echo ""
echo "建议:"
echo "1. 检查未覆盖的错误码"
echo "2. 重点关注返回码不是期望值的测试用例"
echo "3. 查看日志文件获取详细响应信息"
