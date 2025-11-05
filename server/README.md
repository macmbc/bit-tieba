# l服务端项目说明

本仓库包含 后端全部服务。当前已通过 CMake 适配，可在 Linux（推荐）或 macOS 上编译运行。

## 项目结构

```
GateServer/      # HTTP 网关，处理客户端请求并转发到 gRPC 服务
StatusServer/    # 状态服务，负责分配聊天服务器、校验登录 token
ChatServer/      # 第一个聊天服 (chatserver1)
ChatServer2/     # 第二个聊天服 (chatserver2)
VarifyServer/    # Node.js 验证码及邮件发送服务
message.proto    # gRPC / Protobuf 定义（各目录下均拷贝一份）
CMakeLists.txt   # 顶层 CMake 构建脚本
```

每个 C++ 服务目录下都包含独立的 `config.ini`，部署前需按实际环境修改。

## Linux 环境准备

以 Debian/Ubuntu 为例安装依赖（其他发行版请对应调整包名）：

```bash
sudo apt update
sudo apt install -y \
  build-essential cmake ninja-build git pkg-config \
  libboost-filesystem-dev libboost-system-dev \
  libjsoncpp-dev libhiredis-dev libssl-dev \
  mysql-client libmysqlcppconn-dev \
  protobuf-compiler libprotobuf-dev \
  grpc++-dev libgrpc++-dev
```

> 注意：某些发行版的 gRPC 或 MySQL Connector/C++ 版本较旧。若缺少：
> - 请按官方指引从源码构建 gRPC (>=1.51) 并安装到 `/usr/local`。
> - 从 Oracle 官网下载 MySQL Connector/C++ 8.x 压缩包，例如解压到 `/opt/mysql-connector-c++-8.0.36`，后续 CMake 通过 `-DMYSQLCPPCONN_ROOT` 指向该目录。

运行依赖：

* MySQL Server
* Redis Server
* Node.js ≥ 16（供 VarifyServer 使用）

## 初始配置

1. **克隆仓库**
   ```bash
   git clone <repo-url> && cd server
   ```

2. **更新配置文件**
   修改以下文件中的主机、端口、账号、密码等：
   - `GateServer/config.ini`
   - `StatusServer/config.ini`
   - `ChatServer/config.ini`
   - `ChatServer2/config.ini`

   `VarifyServer/config.json` 需配置 SMTP、Redis 等信息。

3. **（可选）重新生成 protobuf / gRPC 代码**
   若你更改了 `message.proto`，需要在每个服务目录下重新生成：
   ```bash
   for dir in GateServer StatusServer ChatServer ChatServer2; do
     pushd "$dir"
     protoc --proto_path=. \
       --cpp_out=. \
       --grpc_out=. \
       --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
       message.proto
     popd
   done
   ```

## 编译

首次运行 CMake 进行配置（根据实际路径调整 MySQL Connector）：

```bash
cmake -S . -B build -G Ninja \
  -DMYSQL_API=jdbc \
  -DMYSQLCPPCONN_ROOT=/opt/mysql-connector-c++-8.0.36 \
  -DCMAKE_PREFIX_PATH="/opt/mysql-connector-c++-8.0.36;/usr/local"
```

常用编译命令：

```bash
cmake --build build --target gateserver
cmake --build build --target statusserver
cmake --build build --target chatserver1
cmake --build build --target chatserver2
```

所有可执行文件会直接生成在各自的源码目录下：

```
GateServer/gateserver
StatusServer/statusserver
ChatServer/chatserver1
ChatServer2/chatserver2
```

### 运行时路径说明

构建脚本会自动为可执行文件写入 OpenSSL、MySQL Connector 的 RPATH，通常无需再设置 `LD_LIBRARY_PATH`。如果依赖安装在其他路径，请在执行 CMake 时通过 `CMAKE_PREFIX_PATH` 指向该路径。

## 部署与运行步骤

1. **准备基础服务**
   - 启动 MySQL，导入所需表/存储过程（默认数据库为 `llfc`）。
   - 启动 Redis，并确保密码与配置文件一致。

2. **启动邮件验证码服务**
   ```bash
   cd VarifyServer
   npm install
  npm run serve
   ```

3. **启动 gRPC 服务（建议按顺序执行）**
   ```bash
   cd StatusServer && ./statusserver
   cd ChatServer && ./chatserver1
   cd ChatServer2 && ./chatserver2
   ```

4. **启动网关**
   ```bash
   cd GateServer && ./gateserver
   ```

   GateServer 提供 HTTP 接口，内部调用 Redis/MySQL，并通过 gRPC 与其他服务交互。

5. **日志与守护**
   - 服务默认将日志输出到标准输出，可结合 tmux、screen、systemd 或 supervisord 进行守护。
   - Node.js 服务可使用 `pm2`/`forever` 等工具运行。

## 开发流程建议

* 使用 `cmake --build build --target <target> --clean-first` 强制重新编译单个服务。
* 每次修改依赖路径、编译选项后都重新运行 `cmake -S . -B build`。
* 保持源码文件统一为 UTF-8，避免出现乱码日志或注释。
* 修改 `message.proto` 后记得提交更新后的 `.pb.cc/.pb.h`。

## 常见问题排查

| 现象 | 可能原因 | 解决方案 |
| ---- | -------- | -------- |
| `source … does not match …` | 之前的 `build/` 来自其他项目 | 删除旧的 `build/` 目录后重新 CMake |
| `no type named 'thread'` | 头文件缺失 | 确认源文件包含 `<thread>/<chrono>/<condition_variable>`（仓库已修复） |
| 运行时报 `libmysqlcppconn` 找不到 | MySQL Connector 不在系统路径 | 通过 `-DMYSQLCPPCONN_ROOT` 指定路径并确保 RPATH 正确 |
| macOS 提示无法打开 `libmysqlcppconn.*` | Gatekeeper 拦截未签名库 | 在“系统设置 → 隐私与安全”中允许，或移除隔离属性 |

## 参与开发

* 提交前请检查 `git diff` 是否包含非预期的二进制或缓存文件。
* 保持注释、日志为 UTF-8 中文（避免重新引入 GBK/乱码）。
* 若新增/修改部署流程，请同步更新本文档。

祝开发顺利！
