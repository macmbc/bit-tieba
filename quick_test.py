#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
快速测试单个功能
使用方法: python quick_test.py <功能名称>

可用功能:
  create_tieba    - 创建贴吧
  search_tieba    - 搜索贴吧
  join_tieba      - 加入贴吧
  leave_tieba     - 退出贴吧
  create_post     - 发帖
  create_reply    - 回复
  get_post_list   - 获取帖子列表
  get_reply_list  - 获取回复列表
  like            - 点赞
  set_top         - 置顶
  set_essence     - 加精
  delete_post     - 删除帖子
  delete_reply    - 删除回复
  get_tieba_info  - 获取贴吧信息
  get_my_tiebas   - 获取我的贴吧列表
  get_members     - 获取成员列表

示例:
  python quick_test.py create_tieba
  python quick_test.py create_post 1002 1
  python quick_test.py like 1001 1 1
"""

import sys
import socket
import struct
import json

class TiebaTestClient:
    """贴吧测试客户端"""
    
    def __init__(self, host='127.0.0.1', port=8090):
        self.host = host
        self.port = port
        self.sock = None
        
    def connect(self):
        """连接到服务器"""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((self.host, self.port))
            print(f"✓ 已连接到服务器 {self.host}:{self.port}\n")
            return True
        except Exception as e:
            print(f"✗ 连接失败: {e}")
            return False
    
    def send_message(self, msg_id, data):
        """
        发送消息到服务器
        
        消息格式:
        [2字节消息ID][2字节数据长度][JSON数据]
        """
        # 将字典转换为JSON字符串
        json_data = json.dumps(data, ensure_ascii=False).encode('utf-8')
        data_len = len(json_data)
        
        # 构造消息包（网络字节序，大端）
        message = struct.pack('!H', msg_id)        # 2字节消息ID
        message += struct.pack('!H', data_len)      # 2字节数据长度
        message += json_data                        # JSON数据
        
        # 发送
        self.sock.sendall(message)
        
        print(f">>> 发送消息 [ID:{msg_id}] <<<")
        print(json.dumps(data, indent=2, ensure_ascii=False))
    
    def receive_response(self):
        """接收服务器响应"""
        try:
            # 读取消息头（2+2=4字节）
            header = self.sock.recv(4)
            if len(header) < 4:
                print("✗ 接收消息头失败")
                return None
            
            # 解析消息头
            msg_id = struct.unpack('!H', header[0:2])[0]
            data_len = struct.unpack('!H', header[2:4])[0]
            
            # 读取JSON数据
            json_data = b''
            remaining = data_len
            while remaining > 0:
                chunk = self.sock.recv(min(remaining, 4096))
                if not chunk:
                    break
                json_data += chunk
                remaining -= len(chunk)
            
            # 解析JSON
            response = json.loads(json_data.decode('utf-8'))
            
            print(f"\n<<< 接收响应 [ID:{msg_id}] <<<")
            print(json.dumps(response, indent=2, ensure_ascii=False))
            
            return response
            
        except Exception as e:
            print(f"✗ 接收响应失败: {e}")
            return None
    
    def close(self):
        """关闭连接"""
        if self.sock:
            self.sock.close()

def quick_create_tieba(uid=1001):
    """快速创建贴吧"""
    client = TiebaTestClient()
    client.connect()
    
    data = {
        "uid": uid,
        "tieba_name": f"测试贴吧_{uid}",
        "tieba_desc": "快速测试创建的贴吧",
        "tieba_icon": "test.png"
    }
    
    print(f"创建贴吧: {data['tieba_name']}")
    client.send_message(1025, data)
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"\n✓ 成功！贴吧ID: {response.get('tieba_id')}")
    else:
        print(f"\n✗ 失败！错误码: {response.get('error') if response else 'None'}")
    
    client.close()
    return response


def quick_search_tieba(keyword="测试"):
    """快速搜索贴吧"""
    client = TiebaTestClient()
    client.connect()
    
    data = {
        "keyword": keyword,
        "offset": 0,
        "limit": 10
    }
    
    print(f"搜索贴吧: 关键词='{keyword}'")
    client.send_message(1027, data)
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"\n✓ 成功！找到 {response.get('total')} 个贴吧")
        for tieba in response.get('tieba_list', []):
            print(f"  - [{tieba['tieba_id']}] {tieba['tieba_name']}")
    else:
        print(f"\n✗ 失败！")
    
    client.close()
    return response


def quick_join_tieba(uid=1002, tieba_id=1):
    """快速加入贴吧"""
    client = TiebaTestClient()
    client.connect()
    
    data = {
        "uid": uid,
        "tieba_id": tieba_id
    }
    
    print(f"加入贴吧: 用户{uid} 加入贴吧{tieba_id}")
    client.send_message(1029, data)
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"\n✓ 成功！")
    else:
        print(f"\n✗ 失败！错误码: {response.get('error') if response else 'None'}")
    
    client.close()
    return response


def quick_leave_tieba(uid=1002, tieba_id=1):
    """快速退出贴吧"""
    client = TiebaTestClient()
    client.connect()
    
    data = {
        "uid": uid,
        "tieba_id": tieba_id
    }
    
    print(f"退出贴吧: 用户{uid} 退出贴吧{tieba_id}")
    client.send_message(1031, data)
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"\n✓ 成功！")
    else:
        print(f"\n✗ 失败！错误码: {response.get('error') if response else 'None'}")
    
    client.close()
    return response


def quick_create_post(uid=1002, tieba_id=1):
    """快速发帖"""
    client = TiebaTestClient()
    client.connect()
    
    data = {
        "uid": uid,
        "tieba_id": tieba_id,
        "title": f"测试帖子_{uid}",
        "content": "这是一个快速测试发布的帖子内容"
    }
    
    print(f"发帖: 用户{uid} 在贴吧{tieba_id}发帖")
    client.send_message(1033, data)
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"\n✓ 成功！帖子ID: {response.get('post_id')}")
    else:
        print(f"\n✗ 失败！错误码: {response.get('error') if response else 'None'}")
    
    client.close()
    return response


def quick_create_reply(uid=1001, post_id=1):
    """快速回复"""
    client = TiebaTestClient()
    client.connect()
    
    data = {
        "uid": uid,
        "post_id": post_id,
        "content": f"用户{uid}的回复内容",
        "reply_to_uid": 0,
        "reply_to_reply_id": 0
    }
    
    print(f"回复: 用户{uid} 回复帖子{post_id}")
    client.send_message(1035, data)
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"\n✓ 成功！回复ID: {response.get('reply_id')}")
    else:
        print(f"\n✗ 失败！错误码: {response.get('error') if response else 'None'}")
    
    client.close()
    return response


def quick_get_post_list(uid=1002, tieba_id=1):
    """快速获取帖子列表"""
    client = TiebaTestClient()
    client.connect()
    
    data = {
        "uid": uid,
        "tieba_id": tieba_id,
        "offset": 0,
        "limit": 10,
        "sort_type": 0
    }
    
    print(f"获取帖子列表: 贴吧{tieba_id}")
    client.send_message(1041, data)
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"\n✓ 成功！共 {response.get('total')} 个帖子")
        for post in response.get('post_list', [])[:5]:
            print(f"  - [{post['post_id']}] {post['title']} (回复:{post['reply_count']}, 点赞:{post['like_count']})")
    else:
        print(f"\n✗ 失败！")
    
    client.close()
    return response


def quick_get_reply_list(uid=1002, post_id=1):
    """快速获取回复列表"""
    client = TiebaTestClient()
    client.connect()
    
    data = {
        "uid": uid,
        "post_id": post_id,
        "offset": 0,
        "limit": 10
    }
    
    print(f"获取回复列表: 帖子{post_id}")
    client.send_message(1043, data)
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"\n✓ 成功！共 {response.get('total')} 个回复")
        for reply in response.get('reply_list', [])[:5]:
            print(f"  - [{reply['reply_id']}] {reply['user_name']}: {reply['content'][:30]}...")
    else:
        print(f"\n✗ 失败！")
    
    client.close()
    return response


def quick_like(uid=1001, target_type=1, target_id=1, is_like=True):
    """快速点赞/取消点赞"""
    client = TiebaTestClient()
    client.connect()
    
    data = {
        "uid": uid,
        "target_type": target_type,  # 1-帖子, 2-回复
        "target_id": target_id,
        "is_like": is_like
    }
    
    action = "点赞" if is_like else "取消点赞"
    target_name = "帖子" if target_type == 1 else "回复"
    print(f"{action}: 用户{uid} {action}{target_name}{target_id}")
    
    client.send_message(1039, data)
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"\n✓ 成功！点赞数: {response.get('like_count')}")
    else:
        print(f"\n✗ 失败！错误码: {response.get('error') if response else 'None'}")
    
    client.close()
    return response


def quick_set_top(uid=1001, post_id=1, tieba_id=1, is_top=True):
    """快速置顶/取消置顶"""
    client = TiebaTestClient()
    client.connect()
    
    data = {
        "uid": uid,
        "post_id": post_id,
        "tieba_id": tieba_id,
        "is_top": is_top
    }
    
    action = "置顶" if is_top else "取消置顶"
    print(f"{action}: 用户{uid} {action}帖子{post_id}")
    
    client.send_message(1045, data)
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"\n✓ 成功！")
    else:
        print(f"\n✗ 失败！错误码: {response.get('error') if response else 'None'}")
    
    client.close()
    return response


def quick_set_essence(uid=1001, post_id=1, tieba_id=1, is_essence=True):
    """快速加精/取消加精"""
    client = TiebaTestClient()
    client.connect()
    
    data = {
        "uid": uid,
        "post_id": post_id,
        "tieba_id": tieba_id,
        "is_essence": is_essence
    }
    
    action = "加精" if is_essence else "取消加精"
    print(f"{action}: 用户{uid} {action}帖子{post_id}")
    
    client.send_message(1047, data)
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"\n✓ 成功！")
    else:
        print(f"\n✗ 失败！错误码: {response.get('error') if response else 'None'}")
    
    client.close()
    return response


def quick_delete_post(uid=1001, post_id=1, tieba_id=1):
    """快速删除帖子"""
    client = TiebaTestClient()
    client.connect()
    
    data = {
        "uid": uid,
        "post_id": post_id,
        "tieba_id": tieba_id
    }
    
    print(f"删除帖子: 用户{uid} 删除帖子{post_id}")
    client.send_message(1037, data)
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"\n✓ 成功！")
    else:
        print(f"\n✗ 失败！错误码: {response.get('error') if response else 'None'}")
    
    client.close()
    return response


def quick_delete_reply(uid=1001, reply_id=1, post_id=1):
    """快速删除回复"""
    client = TiebaTestClient()
    client.connect()
    
    data = {
        "uid": uid,
        "reply_id": reply_id,
        "post_id": post_id
    }
    
    print(f"删除回复: 用户{uid} 删除回复{reply_id}")
    client.send_message(1055, data)
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"\n✓ 成功！")
    else:
        print(f"\n✗ 失败！错误码: {response.get('error') if response else 'None'}")
    
    client.close()
    return response


def quick_get_tieba_info(uid=1002, tieba_id=1):
    """快速获取贴吧信息"""
    client = TiebaTestClient()
    client.connect()
    
    data = {
        "uid": uid,
        "tieba_id": tieba_id
    }
    
    print(f"获取贴吧信息: 贴吧{tieba_id}")
    client.send_message(1049, data)
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        info = response.get('tieba_info', {})
        print(f"\n✓ 成功！")
        print(f"  名称: {info.get('tieba_name')}")
        print(f"  成员: {info.get('member_count')}")
        print(f"  帖子: {info.get('post_count')}")
        print(f"  是否成员: {response.get('is_member')}")
        print(f"  角色: {response.get('member_role')}")
    else:
        print(f"\n✗ 失败！")
    
    client.close()
    return response


def quick_get_my_tiebas(uid=1002):
    """快速获取我的贴吧列表"""
    client = TiebaTestClient()
    client.connect()
    
    data = {
        "uid": uid
    }
    
    print(f"获取我的贴吧: 用户{uid}")
    client.send_message(1051, data)
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        tiebas = response.get('tieba_list', [])
        print(f"\n✓ 成功！加入了 {len(tiebas)} 个贴吧")
        for tieba in tiebas:
            print(f"  - [{tieba['tieba_id']}] {tieba['tieba_name']} (成员:{tieba['member_count']}, 帖子:{tieba['post_count']})")
    else:
        print(f"\n✗ 失败！")
    
    client.close()
    return response


def quick_get_members(uid=1001, tieba_id=1):
    """快速获取成员列表"""
    client = TiebaTestClient()
    client.connect()
    
    data = {
        "uid": uid,
        "tieba_id": tieba_id,
        "offset": 0,
        "limit": 20
    }
    
    print(f"获取成员列表: 贴吧{tieba_id}")
    client.send_message(1053, data)
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        members = response.get('member_list', [])
        print(f"\n✓ 成功！共 {response.get('total')} 个成员")
        role_map = {0: '成员', 1: '小吧主', 2: '吧主'}
        for member in members[:10]:
            print(f"  - [{member['uid']}] {member['user_name']} ({role_map.get(member['role'], '未知')})")
    else:
        print(f"\n✗ 失败！")
    
    client.close()
    return response


def show_help():
    """显示帮助信息"""
    print(__doc__)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        show_help()
        sys.exit(1)
    
    command = sys.argv[1].lower()
    
    try:
        if command == 'create_tieba':
            uid = int(sys.argv[2]) if len(sys.argv) > 2 else 1001
            quick_create_tieba(uid)
        
        elif command == 'search_tieba':
            keyword = sys.argv[2] if len(sys.argv) > 2 else "测试"
            quick_search_tieba(keyword)
        
        elif command == 'join_tieba':
            uid = int(sys.argv[2]) if len(sys.argv) > 2 else 1002
            tieba_id = int(sys.argv[3]) if len(sys.argv) > 3 else 1
            quick_join_tieba(uid, tieba_id)
        
        elif command == 'leave_tieba':
            uid = int(sys.argv[2]) if len(sys.argv) > 2 else 1002
            tieba_id = int(sys.argv[3]) if len(sys.argv) > 3 else 1
            quick_leave_tieba(uid, tieba_id)
        
        elif command == 'create_post':
            uid = int(sys.argv[2]) if len(sys.argv) > 2 else 1002
            tieba_id = int(sys.argv[3]) if len(sys.argv) > 3 else 1
            quick_create_post(uid, tieba_id)
        
        elif command == 'create_reply':
            uid = int(sys.argv[2]) if len(sys.argv) > 2 else 1001
            post_id = int(sys.argv[3]) if len(sys.argv) > 3 else 1
            quick_create_reply(uid, post_id)
        
        elif command == 'get_post_list':
            uid = int(sys.argv[2]) if len(sys.argv) > 2 else 1002
            tieba_id = int(sys.argv[3]) if len(sys.argv) > 3 else 1
            quick_get_post_list(uid, tieba_id)
        
        elif command == 'get_reply_list':
            uid = int(sys.argv[2]) if len(sys.argv) > 2 else 1002
            post_id = int(sys.argv[3]) if len(sys.argv) > 3 else 1
            quick_get_reply_list(uid, post_id)
        
        elif command == 'like':
            uid = int(sys.argv[2]) if len(sys.argv) > 2 else 1001
            target_type = int(sys.argv[3]) if len(sys.argv) > 3 else 1
            target_id = int(sys.argv[4]) if len(sys.argv) > 4 else 1
            quick_like(uid, target_type, target_id)
        
        elif command == 'set_top':
            uid = int(sys.argv[2]) if len(sys.argv) > 2 else 1001
            post_id = int(sys.argv[3]) if len(sys.argv) > 3 else 1
            tieba_id = int(sys.argv[4]) if len(sys.argv) > 4 else 1
            quick_set_top(uid, post_id, tieba_id)
        
        elif command == 'set_essence':
            uid = int(sys.argv[2]) if len(sys.argv) > 2 else 1001
            post_id = int(sys.argv[3]) if len(sys.argv) > 3 else 1
            tieba_id = int(sys.argv[4]) if len(sys.argv) > 4 else 1
            quick_set_essence(uid, post_id, tieba_id)
        
        elif command == 'delete_post':
            uid = int(sys.argv[2]) if len(sys.argv) > 2 else 1001
            post_id = int(sys.argv[3]) if len(sys.argv) > 3 else 1
            tieba_id = int(sys.argv[4]) if len(sys.argv) > 4 else 1
            quick_delete_post(uid, post_id, tieba_id)
        
        elif command == 'delete_reply':
            uid = int(sys.argv[2]) if len(sys.argv) > 2 else 1001
            reply_id = int(sys.argv[3]) if len(sys.argv) > 3 else 1
            post_id = int(sys.argv[4]) if len(sys.argv) > 4 else 1
            quick_delete_reply(uid, reply_id, post_id)
        
        elif command == 'get_tieba_info':
            uid = int(sys.argv[2]) if len(sys.argv) > 2 else 1002
            tieba_id = int(sys.argv[3]) if len(sys.argv) > 3 else 1
            quick_get_tieba_info(uid, tieba_id)
        
        elif command == 'get_my_tiebas':
            uid = int(sys.argv[2]) if len(sys.argv) > 2 else 1002
            quick_get_my_tiebas(uid)
        
        elif command == 'get_members':
            uid = int(sys.argv[2]) if len(sys.argv) > 2 else 1001
            tieba_id = int(sys.argv[3]) if len(sys.argv) > 3 else 1
            quick_get_members(uid, tieba_id)
        
        else:
            print(f"未知命令: {command}")
            show_help()
            sys.exit(1)
    
    except Exception as e:
        print(f"\n错误: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
