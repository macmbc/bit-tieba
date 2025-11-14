# 贴吧系统后端 - 从零开始运行指南

本文档提供完整的从零开始部署和运行贴吧系统后端的详细步骤。适用于 Linux（推荐）、macOS 和 Windows（使用 WSL 或 MinGW）环境。

## 目录

1. [系统架构概览](#系统架构概览)
2. [系统要求](#系统要求)
3. [环境准备](#环境准备)
4. [数据库和Redis配置](#数据库和redis配置)
5. [项目配置](#项目配置)
6. [编译项目](#编译项目)
7. [运行服务](#运行服务)
8. [验证部署](#验证部署)
9. [常见问题排查](#常见问题排查)

---

## 系统架构概览

```
┌─────────────┐
│   客户端     │
└──────┬──────┘
       │ HTTP
       ▼
┌─────────────────────────────────┐
│      GateServer (8080)           │
│  HTTP网关 - 处理登录/注册/验证码  │
└──────┬────────────────┬──────────┘
       │ gRPC           │ gRPC
       ▼                ▼
┌──────────────┐  ┌──────────────┐
│ VarifyServer │  │ StatusServer │
│  (50051)     │  │  (50052)     │
│  邮件验证码   │  │  节点分配     │
└──────────────┘  └──────┬───────┘
                         │ gRPC
                         ▼
              ┌──────────────────────┐
              │   ChatServer 集群     │
              │  TCP: 8090/8091      │
              │  gRPC: 50055/50056   │
              │  实时聊天&好友业务    │
              └──────────────────────┘
```

**服务说明：**
- **GateServer**: HTTP网关，提供注册、登录、密码重置等接口
- **VarifyServer**: Node.js服务，发送邮件验证码
- **StatusServer**: gRPC服务，分配聊天服务器节点并生成登录token
- **ChatServer**: TCP长连接服务，处理实时聊天、好友管理等业务
- **ChatServer2**: 第二个聊天服务器节点（支持横向扩展）

**依赖服务：**
- MySQL 8.0+ (默认端口 3308，数据库名 `llfc`)
- Redis (默认端口 6380，密码 `123456`)

---

## 系统要求

### 硬件要求
- CPU: 2核心以上
- 内存: 4GB以上（编译时建议8GB）
- 磁盘: 10GB可用空间

### 软件要求
- 操作系统: Linux (Ubuntu 20.04+/CentOS 7+)、macOS 10.15+、或 Windows 10+ (WSL2)
- 编译器: GCC 10+ 或 Clang 12+ (支持C++17)
- CMake: 3.20+
- 其他: Git, pkg-config

---

## 环境准备

### Linux (Ubuntu/Debian)

#### 1. 更新系统包
```bash
sudo apt update
sudo apt upgrade -y
```

#### 2. 安装基础编译工具
```bash
sudo apt install -y \
  build-essential \
  cmake \
  ninja-build \
  git \
  pkg-config
```

#### 3. 安装依赖库
```bash
sudo apt install -y \
  libboost-filesystem-dev \
  libboost-system-dev \
  libjsoncpp-dev \
  libhiredis-dev \
  libssl-dev \
  mysql-client \
  libmysqlcppconn-dev \
  protobuf-compiler \
  libprotobuf-dev \
  grpc++-dev \
  libgrpc++-dev
```

**注意**: 如果系统仓库中的 gRPC 版本过低（<1.51），需要从源码编译安装。详见 [gRPC源码安装](#grpc源码安装)。

#### 4. 安装MySQL Connector/C++

**方式一：使用系统包（如果可用）**
```bash
sudo apt install -y libmysqlcppconn-dev
```

**方式二：从Oracle官网下载安装（推荐）**
1. 访问 https://dev.mysql.com/downloads/connector/cpp/
2. 下载 MySQL Connector/C++ 8.0+ (Linux x86_64)
3. 解压到 `/opt` 目录：
```bash
cd /opt
sudo tar xzf mysql-connector-c++-8.0.36-linux-glibc2.17-x86-64bit.tar.gz
sudo mv mysql-connector-c++-8.0.36-linux-glibc2.17-x86-64bit mysql-connector-c++-8.0.36
```

#### 5. 安装Node.js (用于VarifyServer)
```bash
# 使用NodeSource仓库安装Node.js 18+
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt install -y nodejs
```

验证安装：
```bash
node --version  # 应该显示 v18.x 或更高
npm --version
```

### Linux (CentOS/RHEL/Fedora)

#### 1. 安装基础工具
```bash
sudo dnf groupinstall -y "Development Tools"
sudo dnf install -y cmake ninja-build git pkgconf-pkg-config
```

#### 2. 安装依赖库
```bash
sudo dnf install -y \
  boost-devel \
  jsoncpp-devel \
  hiredis-devel \
  openssl-devel \
  mariadb mariadb-devel \
  protobuf protobuf-compiler protobuf-devel
```

#### 3. 安装Boost 1.78+
```bash
sudo dnf install -y boost1.78-devel
```

#### 4. 安装gRPC和MySQL Connector
参考Ubuntu章节，或从源码编译安装。

### macOS

#### 1. 安装Homebrew
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

#### 2. 安装依赖
```bash
brew install cmake ninja pkg-config
brew install boost jsoncpp hiredis openssl
brew install mysql-connector-c++
brew install protobuf grpc
brew install node
```

#### 3. 设置环境变量（如果使用非标准路径）
```bash
export CMAKE_PREFIX_PATH="/opt/homebrew:/usr/local"
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:/usr/local/lib/pkgconfig"
```

### Windows (使用WSL2)

#### 1. 安装WSL2和Ubuntu
```powershell
wsl --install -d Ubuntu-22.04
```

#### 2. 在WSL中按照Linux (Ubuntu)步骤操作

---

## gRPC源码安装

如果系统包管理器中的gRPC版本过低，需要从源码编译：

### 1. 下载gRPC源码
```bash
cd /usr/local/src
git clone --recurse-submodules -b v1.51.0 https://github.com/grpc/grpc.git
cd grpc
```

### 2. 编译安装
```bash
mkdir -p cmake/build
cd cmake/build
cmake ../.. \
  -DgRPC_INSTALL=ON \
  -DgRPC_BUILD_TESTS=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_STANDARD=17 \
  -DgRPC_SSL_PROVIDER=package \
  -DgRPC_ZLIB_PROVIDER=package \
  -DgRPC_CARES_PROVIDER=package \
  -DCMAKE_INSTALL_PREFIX=/usr/local

sudo cmake --build . --target install -j$(nproc)
sudo ldconfig
```

### 3. 验证安装
```bash
protoc --version
grpc_cpp_plugin --version
```

---

## 数据库和Redis配置

### 1. 安装MySQL

#### Linux
```bash
# Ubuntu/Debian
sudo apt install -y mysql-server
sudo systemctl start mysql
sudo systemctl enable mysql

# CentOS/RHEL
sudo dnf install -y mariadb-server
sudo systemctl start mariadb
sudo systemctl enable mariadb
```

#### macOS
```bash
brew install mysql
brew services start mysql
```

#### 配置MySQL
```bash
sudo mysql_secure_installation
```

创建数据库和用户：
```sql
mysql -u root -p
```

```sql
CREATE DATABASE llfc CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER 'root'@'localhost' IDENTIFIED BY '123456.';
GRANT ALL PRIVILEGES ON llfc.* TO 'root'@'localhost';
FLUSH PRIVILEGES;
```

**重要**: 如果MySQL默认端口不是3308，需要修改配置文件中的端口号，或修改MySQL配置使用3308端口。

#### 导入数据库结构
如果有SQL文件，导入数据库：
```bash
mysql -h127.0.0.1 -P3308 -uroot -p123456. llfc < sql备份/llfc.sql
```

如果没有SQL文件，需要创建以下表结构（参考`BACKEND_INTEGRATION.md`）：
- `user`: 用户基本资料
- `friend`: 好友关系
- `friend_apply`: 好友申请
- `user_id`: 自增uid序列

### 2. 安装Redis

#### Linux
```bash
# Ubuntu/Debian
sudo apt install -y redis-server

# CentOS/RHEL
sudo dnf install -y redis
```

#### macOS
```bash
brew install redis
brew services start redis
```

#### 配置Redis密码
编辑Redis配置文件：
```bash
sudo nano /etc/redis/redis.conf  # Linux
# 或
nano /opt/homebrew/etc/redis.conf  # macOS
```

找到并修改：
```
requirepass 123456
port 6380  # 如果默认端口不是6380
```

重启Redis：
```bash
# Linux
sudo systemctl restart redis

# macOS
brew services restart redis
```

验证Redis连接：
```bash
redis-cli -h 127.0.0.1 -p 6380 -a 123456
> PING
PONG
```

---

## 项目配置

### 1. 克隆项目
```bash
git clone <repository-url>
cd bit-tieba-backend/server
```

### 2. 修改配置文件

#### GateServer/config.ini
```ini
[GateServer]
Port = 8080

[VarifyServer]
Host = 127.0.0.1
Port = 50051

[StatusServer]
Host = 127.0.0.1
Port = 50052

[Mysql]
Host = 127.0.0.1
Port = 3308        # 根据实际MySQL端口修改
User = root
Passwd = 123456.   # 根据实际密码修改
Schema = llfc

[Redis]
Host = 127.0.0.1
Port = 6380        # 根据实际Redis端口修改
Passwd = 123456    # 根据实际Redis密码修改
```

#### StatusServer/config.ini
```ini
[StatusServer]
Port = 50052
Host = 0.0.0.0

[Mysql]
Host = 127.0.0.1
Port = 3308
User = root
Passwd = 123456.
Schema = llfc

[Redis]
Host = 127.0.0.1
Port = 6380
Passwd = 123456

[chatservers]
Name = chatserver1,chatserver2

[chatserver1]
Name = chatserver1
Host = 127.0.0.1
Port = 8090

[chatserver2]
Name = chatserver2
Host = 127.0.0.1
Port = 8091
```

#### ChatServer/config.ini
```ini
[GateServer]
Port = 8080

[VarifyServer]
Host = 127.0.0.1
Port = 50051

[StatusServer]
Host = 127.0.0.1
Port = 50052

[SelfServer]
Name = chatserver1
Host = 0.0.0.0
Port = 8090
RPCPort = 50055

[Mysql]
Host = 127.0.0.1
Port = 3308
User = root
Passwd = 123456.
Schema = llfc

[Redis]
Host = 127.0.0.1
Port = 6380
Passwd = 123456

[PeerServer]
Servers = chatserver2

[chatserver2]
Name = chatserver2
Host = 127.0.0.1
Port = 50056
```

#### ChatServer2/config.ini
```ini
[GateServer]
Port = 8080

[VarifyServer]
Host = 127.0.0.1
Port = 50051

[StatusServer]
Host = 127.0.0.1
Port = 50052

[SelfServer]
Name = chatserver2
Host = 0.0.0.0
Port = 8091
RPCPort = 50056

[Mysql]
Host = 127.0.0.1
Port = 3308
User = root
Passwd = 123456.
Schema = llfc

[Redis]
Host = 127.0.0.1
Port = 6380
Passwd = 123456

[PeerServer]
Servers = chatserver1

[chatserver1]
Name = chatserver1
Host = 127.0.0.1
Port = 50055
```

#### VarifyServer/config.json
```json
{
  "email": {
    "user": "your-email@example.com",      # 修改为你的邮箱
    "pass": "your-email-password"          # 修改为邮箱密码或授权码
  },
  "mysql": {
    "host": "127.0.0.1",
    "port": 3308,
    "passwd": "123456."
  },
  "redis": {
    "host": "127.0.0.1",
    "port": 6380,
    "passwd": "123456"
  }
}
```

**邮件配置说明：**
- 如果使用163邮箱，需要在邮箱设置中开启SMTP服务并生成授权码
- 如果使用Gmail，需要启用"不太安全的应用访问"或使用OAuth2
- 其他邮箱提供商请参考其SMTP配置文档

### 3. 生成Protobuf文件（可选）

如果修改了`message.proto`文件，需要重新生成：
```bash
cd server
for dir in GateServer StatusServer ChatServer ChatServer2; do
  cd "$dir"
  protoc --proto_path=. \
    --cpp_out=. \
    --grpc_out=. \
    --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
    message.proto
  cd ..
done
```

---

## 编译项目

### 1. 创建构建目录
```bash
cd server
mkdir -p build
cd build
```

### 2. 配置CMake

#### Linux (使用系统包管理器安装的依赖)
```bash
cmake -S .. -B . -G Ninja \
  -DMYSQL_API=jdbc \
  -DCMAKE_PREFIX_PATH="/usr/local;/usr"
```

#### Linux (使用自定义路径的MySQL Connector)
```bash
cmake -S .. -B . -G Ninja \
  -DMYSQL_API=jdbc \
  -DMYSQLCPPCONN_ROOT=/opt/mysql-connector-c++-8.0.36 \
  -DCMAKE_PREFIX_PATH="/opt/mysql-connector-c++-8.0.36;/usr/local;/usr"
```

#### macOS
```bash
cmake -S .. -B . -G Ninja \
  -DMYSQL_API=jdbc \
  -DCMAKE_PREFIX_PATH="/opt/homebrew;/usr/local"
```

#### 如果遇到依赖找不到的问题
可以显式指定路径：
```bash
cmake -S .. -B . -G Ninja \
  -DMYSQL_API=jdbc \
  -DMYSQLCPPCONN_INCLUDE_DIR=/opt/mysql-connector-c++-8.0.36/include \
  -DMYSQLCPPCONN_LIB=/opt/mysql-connector-c++-8.0.36/lib64/libmysqlcppconn.so \
  -DCMAKE_PREFIX_PATH="/usr/local;/usr/local/lib64;/usr" \
  -DBoost_INCLUDE_DIR=/usr/include/boost1.78 \
  -DJsonCpp_DIR=/usr/local/lib64/cmake/jsoncpp \
  -DProtobuf_DIR=/usr/local/lib64/cmake/protobuf \
  -DgRPC_DIR=/usr/local/lib64/cmake/grpc
```

### 3. 编译
```bash
# 编译所有服务
cmake --build . -- -j$(nproc)  # Linux
cmake --build . -- -j$(sysctl -n hw.ncpu)  # macOS

# 或单独编译某个服务
cmake --build . --target gateserver
cmake --build . --target statusserver
cmake --build . --target chatserver1
cmake --build . --target chatserver2
```

### 4. 验证编译结果
编译成功后，可执行文件会生成在各自的源码目录下：
```bash
ls -la GateServer/gateserver
ls -la StatusServer/statusserver
ls -la ChatServer/chatserver1
ls -la ChatServer2/chatserver2
```

---

## 运行服务

### 1. 启动顺序

服务需要按以下顺序启动：

1. **MySQL** (如果未启动)
2. **Redis** (如果未启动)
3. **VarifyServer** (Node.js服务)
4. **StatusServer** (gRPC服务)
5. **ChatServer** (TCP服务)
6. **ChatServer2** (TCP服务，可选)
7. **GateServer** (HTTP网关)

### 2. 启动MySQL和Redis

```bash
# Linux
sudo systemctl start mysql
sudo systemctl start redis

# macOS
brew services start mysql
brew services start redis
```

### 3. 启动VarifyServer

```bash
cd server/VarifyServer
npm install
npm run serve
```

应该看到输出：
```
varify server started
```

### 4. 启动StatusServer

```bash
cd server/StatusServer
./statusserver
```

### 5. 启动ChatServer

```bash
# 第一个聊天服务器
cd server/ChatServer
./chatserver1

# 第二个聊天服务器（新终端）
cd server/ChatServer2
./chatserver2
```

### 6. 启动GateServer

```bash
cd server/GateServer
./gateserver
```

### 7. 使用后台运行（生产环境推荐）

#### 使用tmux
```bash
# 安装tmux
sudo apt install tmux  # Linux
brew install tmux      # macOS

# 创建会话
tmux new -s tieba-backend

# 在tmux中启动所有服务（每个服务一个窗口）
# Ctrl+B, C 创建新窗口
# 在每个窗口中启动对应的服务
```

#### 使用systemd (Linux)

创建systemd服务文件：

`/etc/systemd/system/tieba-varify.service`
```ini
[Unit]
Description=Tieba VarifyServer
After=network.target

[Service]
Type=simple
User=your-user
WorkingDirectory=/path/to/bit-tieba-backend/server/VarifyServer
ExecStart=/usr/bin/node server.js
Restart=always

[Install]
WantedBy=multi-user.target
```

`/etc/systemd/system/tieba-status.service`
```ini
[Unit]
Description=Tieba StatusServer
After=network.target

[Service]
Type=simple
User=your-user
WorkingDirectory=/path/to/bit-tieba-backend/server/StatusServer
ExecStart=/path/to/bit-tieba-backend/server/StatusServer/statusserver
Restart=always

[Install]
WantedBy=multi-user.target
```

启动服务：
```bash
sudo systemctl daemon-reload
sudo systemctl enable tieba-varify tieba-status tieba-chat1 tieba-chat2 tieba-gate
sudo systemctl start tieba-varify tieba-status tieba-chat1 tieba-chat2 tieba-gate
```

#### 使用PM2 (Node.js服务)

```bash
npm install -g pm2
cd server/VarifyServer
pm2 start server.js --name varify-server
pm2 save
pm2 startup
```

---

## 验证部署

### 1. 检查服务端口

```bash
# 检查GateServer（使用已注册的测试接口）
curl http://127.0.0.1:8080/get_test

# 检查gRPC服务（需要grpcurl工具）
grpcurl -plaintext 127.0.0.1:50051 list
grpcurl -plaintext 127.0.0.1:50052 list
```

**注意**: 直接访问 `http://127.0.0.1:8080` 会返回 "url not found"，因为根路径 `/` 未注册路由。请使用具体的 API 路径，如 `/get_test` 或其他已注册的接口。

### 2. 测试HTTP接口

#### 获取验证码
```bash
curl -X POST http://127.0.0.1:8080/get_varifycode \
  -H "Content-Type: application/json" \
  -d '{"email":"test@example.com"}'
```

预期响应：
```json
{"error":0,"email":"test@example.com"}
```

#### 用户注册
```bash
curl -X POST http://127.0.0.1:8080/user_register \
  -H "Content-Type: application/json" \
  -d '{
    "email":"test@example.com",
    "user":"testuser",
    "passwd":"password123",
    "confirm":"password123",
    "icon":":/res/head_1.jpg",
    "varifycode":"1234"
  }'
```

#### 用户登录
```bash
curl -X POST http://127.0.0.1:8080/user_login \
  -H "Content-Type: application/json" \
  -d '{
    "email":"test@example.com",
    "passwd":"password123"
  }'
```

预期响应：
```json
{
  "error":0,
  "email":"test@example.com",
  "uid":1054,
  "token":"...",
  "host":"127.0.0.1",
  "port":"8090"
}
```

### 3. 测试TCP连接

使用telnet或nc测试ChatServer：
```bash
telnet 127.0.0.1 8090
# 或
nc 127.0.0.1 8090
```

### 4. 查看日志

检查各服务的输出日志，确认没有错误信息。

### 5. 检查Redis数据

```bash
redis-cli -h 127.0.0.1 -p 6380 -a 123456
> KEYS *
> GET code_test@example.com
> GET utoken_1054
```

---

## 常见问题排查

### 1. 编译错误

#### 错误: `protobuf not found`
**解决方案**: 安装protobuf开发库
```bash
sudo apt install libprotobuf-dev protobuf-compiler
```

#### 错误: `grpc not found`
**解决方案**: 从源码编译安装gRPC，或使用更新的系统仓库

#### 错误: `mysqlcppconn not found`
**解决方案**: 
- 确保MySQL Connector/C++已安装
- 使用`-DMYSQLCPPCONN_ROOT`指定路径
- 检查头文件路径是否正确

#### 错误: `jsoncpp string_view not found`
**解决方案**: 使用jsoncpp 1.9.5+版本，并确保使用C++17编译

### 2. 运行时错误

#### 错误: `Address already in use`
**解决方案**: 端口被占用，检查并关闭占用端口的进程
```bash
# Linux
sudo lsof -i :8080
sudo kill -9 <PID>

# 或修改配置文件使用其他端口
```

#### 错误: `Connection refused` (MySQL/Redis)
**解决方案**: 
- 检查MySQL/Redis是否启动
- 检查配置文件中的地址和端口是否正确
- 检查防火墙设置

#### 错误: `TokenInvalid` 或 `UidInvalid`
**解决方案**: 
- 检查Redis连接是否正常
- 清除Redis中的旧数据：`redis-cli FLUSHDB`
- 重新登录获取新token

### 3. 服务启动失败

#### VarifyServer无法启动
**解决方案**:
- 检查Node.js版本（需要16+）
- 运行`npm install`安装依赖
- 检查`config.json`中的邮箱配置
- 查看错误日志

#### ChatServer无法连接StatusServer
**解决方案**:
- 确保StatusServer已启动
- 检查gRPC端口(50052)是否可访问
- 检查防火墙设置

#### GateServer无法连接其他服务
**解决方案**:
- 按顺序启动服务：VarifyServer → StatusServer → ChatServer → GateServer
- 检查各服务的配置文件中的地址和端口
- 使用`netstat`或`ss`检查端口监听状态

### 4. 性能问题

#### 编译速度慢
**解决方案**:
- 使用Ninja构建系统（已配置）
- 增加并行编译线程数：`-j$(nproc)`
- 使用ccache加速编译

#### 运行时内存不足
**解决方案**:
- 减少ChatServer实例数量
- 增加系统swap空间
- 优化代码中的内存使用

### 5. 数据库问题

#### 数据库连接失败
**解决方案**:
- 检查MySQL是否运行：`sudo systemctl status mysql`
- 验证用户名和密码
- 检查数据库`llfc`是否存在
- 检查MySQL端口是否正确（默认3308）

#### 表不存在
**解决方案**:
- 导入SQL文件创建表结构
- 或手动创建所需的表（参考`BACKEND_INTEGRATION.md`）

### 6. Redis问题

#### Redis连接失败
**解决方案**:
- 检查Redis是否运行：`redis-cli ping`
- 验证密码是否正确
- 检查Redis端口（默认6380）
- 检查Redis配置文件中的`requirepass`

#### Redis键过期问题
**解决方案**:
- 验证码默认600秒过期，这是正常行为
- Token无过期时间，如需过期请修改代码

### 7. 网络问题

#### 无法从外部访问
**解决方案**:
- 检查防火墙规则
- 确保服务绑定到`0.0.0.0`而非`127.0.0.1`
- 检查云服务器的安全组设置

#### gRPC连接超时
**解决方案**:
- 检查网络延迟
- 增加gRPC超时时间（需修改代码）
- 检查服务间网络连通性

---

## 下一步

部署成功后，你可以：

1. **配置前端**: 将前端应用连接到GateServer (http://your-server:8080)
2. **扩展ChatServer**: 添加更多ChatServer实例以支持更多并发用户
3. **监控和日志**: 配置日志收集和监控系统
4. **性能优化**: 根据实际负载调整配置参数
5. **安全加固**: 配置HTTPS、添加接口限流、加强认证等

---

## 获取帮助

如果遇到问题：

1. 查看日志文件和服务输出
2. 检查配置文件是否正确
3. 参考`DEPLOYMENT_GUIDE.md`和`BACKEND_INTEGRATION.md`
4. 检查GitHub Issues（如果有）
5. 联系项目维护者

---

## 附录

### A. 服务端口列表

| 服务 | 端口 | 协议 | 说明 |
|------|------|------|------|
| GateServer | 8080 | HTTP | HTTP网关 |
| VarifyServer | 50051 | gRPC | 邮件验证码服务 |
| StatusServer | 50052 | gRPC | 节点分配服务 |
| ChatServer | 8090 | TCP | 聊天服务器1 |
| ChatServer | 50055 | gRPC | ChatServer1的gRPC端口 |
| ChatServer2 | 8091 | TCP | 聊天服务器2 |
| ChatServer2 | 50056 | gRPC | ChatServer2的gRPC端口 |
| MySQL | 3308 | TCP | 数据库（默认） |
| Redis | 6380 | TCP | 缓存（默认） |

### B. 配置文件路径

```
server/
├── GateServer/config.ini
├── StatusServer/config.ini
├── ChatServer/config.ini
├── ChatServer2/config.ini
└── VarifyServer/config.json
```

### C. 环境变量（可选）

如果依赖安装在非标准路径，可以设置：

```bash
export CMAKE_PREFIX_PATH="/opt/custom/path:/usr/local"
export LD_LIBRARY_PATH="/opt/custom/lib:$LD_LIBRARY_PATH"
export PKG_CONFIG_PATH="/opt/custom/lib/pkgconfig:$PKG_CONFIG_PATH"
```

---

**祝部署顺利！**

如有任何问题或建议，欢迎反馈。


