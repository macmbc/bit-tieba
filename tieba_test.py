#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
贴吧功能测试客户端
使用方法: python tieba_test.py [host] [port]
示例: python tieba_test.py 127.0.0.1 8081
"""

import socket
import struct
import json
import sys
import time

class TiebaTestClient:
    """贴吧测试客户端"""
    
    def __init__(self, host='127.0.0.1', port=8081):
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
            print("\n连接已关闭")


def test_create_tieba(client, uid=1001):
    """测试1: 创建贴吧"""
    print("\n" + "="*50)
    print("测试1: 创建贴吧")
    print("="*50)
    
    data = {
        "uid": uid,
        "tieba_name": "C++学习吧",
        "tieba_desc": "C++编程学习交流社区",
        "tieba_icon": "cpp_icon.png"
    }
    
    client.send_message(1025, data)  # ID_CREATE_TIEBA_REQ
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"✓ 创建成功！贴吧ID: {response.get('tieba_id')}")
        return response.get('tieba_id')
    else:
        print(f"✗ 创建失败！错误码: {response.get('error') if response else 'None'}")
        return None


def test_search_tieba(client):
    """测试2: 搜索贴吧"""
    print("\n" + "="*50)
    print("测试2: 搜索贴吧")
    print("="*50)
    
    data = {
        "keyword": "C++",
        "offset": 0,
        "limit": 10
    }
    
    client.send_message(1027, data)  # ID_SEARCH_TIEBA_REQ
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"✓ 搜索成功！找到 {response.get('total')} 个贴吧")
        return True
    else:
        print(f"✗ 搜索失败！")
        return False


def test_join_tieba(client, uid, tieba_id):
    """测试3: 加入贴吧"""
    print("\n" + "="*50)
    print("测试3: 加入贴吧")
    print("="*50)
    
    data = {
        "uid": uid,
        "tieba_id": tieba_id
    }
    
    client.send_message(1029, data)  # ID_JOIN_TIEBA_REQ
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"✓ 加入成功！")
        return True
    else:
        print(f"✗ 加入失败！错误码: {response.get('error') if response else 'None'}")
        return False


def test_get_tieba_info(client, uid, tieba_id):
    """测试4: 获取贴吧信息"""
    print("\n" + "="*50)
    print("测试4: 获取贴吧信息")
    print("="*50)
    
    data = {
        "uid": uid,
        "tieba_id": tieba_id
    }
    
    client.send_message(1049, data)  # ID_GET_TIEBA_INFO_REQ
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"✓ 获取成功！是否成员: {response.get('is_member')}, 角色: {response.get('member_role')}")
        return True
    else:
        print(f"✗ 获取失败！")
        return False


def test_create_post(client, uid, tieba_id):
    """测试5: 发帖"""
    print("\n" + "="*50)
    print("测试5: 发帖")
    print("="*50)
    
    data = {
        "uid": uid,
        "tieba_id": tieba_id,
        "title": "如何学习C++模板编程？",
        "content": "最近在学习C++模板，感觉很难理解，有没有好的学习资料推荐？"
    }
    
    client.send_message(1033, data)  # ID_CREATE_POST_REQ
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"✓ 发帖成功！帖子ID: {response.get('post_id')}")
        return response.get('post_id')
    else:
        print(f"✗ 发帖失败！错误码: {response.get('error') if response else 'None'}")
        return None


def test_get_post_list(client, uid, tieba_id):
    """测试6: 获取帖子列表"""
    print("\n" + "="*50)
    print("测试6: 获取帖子列表")
    print("="*50)
    
    data = {
        "uid": uid,
        "tieba_id": tieba_id,
        "offset": 0,
        "limit": 20,
        "sort_type": 0
    }
    
    client.send_message(1041, data)  # ID_GET_POST_LIST_REQ
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"✓ 获取成功！共 {response.get('total')} 个帖子")
        return True
    else:
        print(f"✗ 获取失败！")
        return False


def test_create_reply(client, uid, post_id):
    """测试7: 回复帖子"""
    print("\n" + "="*50)
    print("测试7: 回复帖子")
    print("="*50)
    
    data = {
        "uid": uid,
        "post_id": post_id,
        "content": "推荐《C++ Templates: The Complete Guide》这本书，讲解很详细。",
        "reply_to_uid": 0,
        "reply_to_reply_id": 0
    }
    
    client.send_message(1035, data)  # ID_CREATE_REPLY_REQ
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"✓ 回复成功！回复ID: {response.get('reply_id')}")
        return response.get('reply_id')
    else:
        print(f"✗ 回复失败！错误码: {response.get('error') if response else 'None'}")
        return None


def test_get_reply_list(client, uid, post_id):
    """测试8: 获取回复列表"""
    print("\n" + "="*50)
    print("测试8: 获取回复列表")
    print("="*50)
    
    data = {
        "uid": uid,
        "post_id": post_id,
        "offset": 0,
        "limit": 20
    }
    
    client.send_message(1043, data)  # ID_GET_REPLY_LIST_REQ
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"✓ 获取成功！共 {response.get('total')} 个回复")
        return True
    else:
        print(f"✗ 获取失败！")
        return False


def test_like_post(client, uid, post_id):
    """测试9: 点赞帖子"""
    print("\n" + "="*50)
    print("测试9: 点赞帖子")
    print("="*50)
    
    data = {
        "uid": uid,
        "target_type": 1,  # 1-帖子, 2-回复
        "target_id": post_id,
        "is_like": True
    }
    
    client.send_message(1039, data)  # ID_LIKE_REQ
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"✓ 点赞成功！点赞数: {response.get('like_count')}")
        return True
    else:
        print(f"✗ 点赞失败！")
        return False


def test_set_top(client, uid, post_id, tieba_id):
    """测试10: 置顶帖子"""
    print("\n" + "="*50)
    print("测试10: 置顶帖子")
    print("="*50)
    
    data = {
        "uid": uid,
        "post_id": post_id,
        "tieba_id": tieba_id,
        "is_top": True
    }
    
    client.send_message(1045, data)  # ID_SET_TOP_REQ
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"✓ 置顶成功！")
        return True
    else:
        print(f"✗ 置顶失败！错误码: {response.get('error') if response else 'None'}")
        return False


def test_get_my_tieba_list(client, uid):
    """测试11: 获取我的贴吧列表"""
    print("\n" + "="*50)
    print("测试11: 获取我的贴吧列表")
    print("="*50)
    
    data = {
        "uid": uid
    }
    
    client.send_message(1051, data)  # ID_GET_MY_TIEBA_LIST_REQ
    response = client.receive_response()
    
    if response and response.get('error') == 0:
        print(f"✓ 获取成功！加入了 {len(response.get('tieba_list', []))} 个贴吧")
        return True
    else:
        print(f"✗ 获取失败！")
        return False


def run_all_tests():
    """运行所有测试"""
    # 解析命令行参数
    host = sys.argv[1] if len(sys.argv) > 1 else '127.0.0.1'
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 8081
    
    print("="*50)
    print("贴吧功能测试客户端")
    print("="*50)
    
    # 创建客户端并连接
    client = TiebaTestClient(host, port)
    if not client.connect():
        return
    
    try:
        # 测试用户ID
        owner_uid = 1001  # 贴吧创建者
        member_uid = 1002  # 普通成员
        
        # 执行测试
        tieba_id = test_create_tieba(client, owner_uid)
        if not tieba_id:
            print("\n✗ 创建贴吧失败，后续测试可能无法进行")
            return
        
        time.sleep(0.5)
        test_search_tieba(client)
        
        time.sleep(0.5)
        test_join_tieba(client, member_uid, tieba_id)
        
        time.sleep(0.5)
        test_get_tieba_info(client, member_uid, tieba_id)
        
        time.sleep(0.5)
        post_id = test_create_post(client, member_uid, tieba_id)
        
        if post_id:
            time.sleep(0.5)
            test_get_post_list(client, member_uid, tieba_id)
            
            time.sleep(0.5)
            test_create_reply(client, owner_uid, post_id)
            
            time.sleep(0.5)
            test_get_reply_list(client, member_uid, post_id)
            
            time.sleep(0.5)
            test_like_post(client, owner_uid, post_id)
            
            time.sleep(0.5)
            test_set_top(client, owner_uid, post_id, tieba_id)
        
        time.sleep(0.5)
        test_get_my_tieba_list(client, member_uid)
        
        print("\n" + "="*50)
        print("测试完成！")
        print("="*50)
        
    except KeyboardInterrupt:
        print("\n\n测试被用户中断")
    except Exception as e:
        print(f"\n✗ 测试出错: {e}")
        import traceback
        traceback.print_exc()
    finally:
        client.close()


if __name__ == '__main__':
    run_all_tests()
