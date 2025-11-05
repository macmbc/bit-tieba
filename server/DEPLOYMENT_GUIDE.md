# 后端部署指南

## 环境概览
- **服务器**：阿里云宝塔版 CentOS（无公网），源码位于 `/opt/2025-11-bitServer/bit-tieba`.
- **编译工具链**：CMake ≥ 3.20、Ninja、GCC 10（C++17）。
- **主要依赖**：Boost ≥ 1.78、hiredis、MySQL Connector/C++ 9.5、jsoncpp 1.9.5、protobuf/gRPC 25.x、OpenSSL、zlib、c-ares、RE2。

## 0. 准备源码
1. 本地仓库同步远端：根据需要选择 `git pull --no-rebase`（合并）或 `git pull --rebase`。
2. 处理合并提交时在编辑器内 `:wq` 保存退出即可。
3. 通过 scp、压缩包或宝塔面板将项目上传到服务器。


## 1.服务器设置


2. 安装编译工具链
```
sudo dnf groupinstall -y "Development Tools"   # 等价于 Debian 的 build-essential
sudo dnf install -y cmake ninja-build git pkgconf-pkg-config
```

3. 安装第三方依赖
```
sudo dnf install boost-devel jsoncpp-devel hiredis-devel openssl-devel \
    protobuf protobuf-compiler protobuf-devel \
    mariadb mariadb-devel
```

4. 安装boost

```
sudo dnf install -y boost1.78-devel
```

5. hiredis
```
sudo dnf install -y hiredis hiredis-devel

sudo dnf install -y \
  gcc gcc-c++ make ninja-build cmake pkgconf-pkg-config \
  boost1.78-devel \
  jsoncpp jsoncpp-devel \
  hiredis hiredis-devel \
  openssl-devel \
  c-ares-devel \
  protobuf protobuf-compiler protobuf-devel \
  mariadb mariadb-devel
```

6. 本地上传grpc源码到/usr/local/src/ 执行：

```
cd /usr/local/src
mkdir -p grpc-1.72.2 && tar xzf grpc-1.72.2-full.tar.gz -C grpc-1.72.2 --strip-components=1
cd grpc-1.72.2
# 子模块已经打包好了，无需再执行 git submodule
mkdir -p cmake/build && cd cmake/build
sudo cmake ../.. \
  -DgRPC_INSTALL=ON \
  -DgRPC_BUILD_TESTS=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_STANDARD=17 \
  -DgRPC_SSL_PROVIDER=package \
  -DgRPC_ZLIB_PROVIDER=package \
  -DgRPC_CARES_PROVIDER=package \
  -DgRPC_ABSL_PROVIDER=module \
  -DCMAKE_INSTALL_PREFIX=/usr/local

sudo cmake --build . --target install
```

7. (可选)重新生成mspb

```
cd /opt/2025-11-bitServer/bit-tieba/server  # 按你的实际路径修改
for svc in GateServer StatusServer ChatServer ChatServer2; do
  cd "$svc"
  rm -f message.pb.* message.grpc.pb.*
  protoc --proto_path=. \
    --cpp_out=. \
    --grpc_out=. \
    --plugin=protoc-gen-grpc=$(command -v grpc_cpp_plugin) \
    message.proto
  cd ..
done
```

8. 本地上传编译jsoncpp

```
cd /usr/local/src
cp /root/uploads/jsoncpp-1.9.5.tar.gz .
tar xf jsoncpp-1.9.5.tar.gz
cd jsoncpp-1.9.5
cmake -S . -B build \
  -DJSONCPP_WITH_TESTS=OFF \
  -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF \
  -DBUILD_SHARED_LIBS=ON \
  -DCMAKE_CXX_STANDARD=17 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build --target all -- -j$(nproc)
sudo cmake --install build
```

9. 上传mysql

```
cd /opt
tar xf mysql-connector-c++-9.5.0-linux-glibc2.17-x86-64bit.tar.gz
mv mysql-connector-c++-9.5.0-linux-glibc2.17-x86-64bit mysql-connector-c++-9.5.0
```

10. 编译

```
rm -rf build
cmake -S . -B build -G Ninja \
  -DMYSQL_API=jdbc \
  -DMYSQLCPPCONN_INCLUDE_DIR=/opt/mysql-connector-c++-9.5.0/include \
  -DMYSQLCPPCONN_LIB=/opt/mysql-connector-c++-9.5.0/lib64/libmysqlcppconn.so \
  -DCMAKE_PREFIX_PATH="/usr/local;/usr/local/lib64;/usr" \
  -DBoost_INCLUDE_DIR=/usr/include/boost1.78 \
  -DJsonCpp_DIR=/usr/local/lib64/cmake/jsoncpp \
  -DProtobuf_DIR=/usr/local/lib64/cmake/protobuf \
  -DgRPC_DIR=/usr/local/lib64/cmake/grpc
cmake --build build -- -j1
```


## 2. 安装/上传依赖
服务器无法访问外网，需要本地下载后上传。



### 2.1 MySQL Connector/C++ 9.5
1. 本地下载 `mysql-connector-c++-9.5.0-linux-glibc2.17-x86-64bit.tar.gz`。
2. 上传到 `/opt`，执行：
   ```bash
   cd /opt
   tar xf mysql-connector-c++-9.5.0-linux-glibc2.17-x86-64bit.tar.gz
   mv mysql-connector-c++-9.5.0-linux-glibc2.17-x86-64bit mysql-connector-c++-9.5.0
   ```
   关键路径：
   - 头文件：`/opt/mysql-connector-c++-9.5.0/include`
   - 动态库：`/opt/mysql-connector-c++-9.5.0/lib64/libmysqlcppconn.so`

### 2.2 jsoncpp 1.9.5（启用 string_view）
```bash
cd /usr/local/src
tar xf jsoncpp-1.9.5.tar.gz
cd jsoncpp-1.9.5
cmake -S . -B build \
  -DJSONCPP_WITH_TESTS=OFF \
  -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF \
  -DBUILD_SHARED_LIBS=ON \
  -DBUILD_STATIC_LIBS=OFF \
  -DCMAKE_CXX_STANDARD=17 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build --target all -- -j$(nproc)
sudo cmake --install build
```
确认 `/usr/local/lib64/libjsoncpp.so` 及 `…/cmake/jsoncpp/jsoncppConfig.cmake` 存在。

### 2.3 protobuf/gRPC
- 将新版二进制或源码构建结果上传至 `/usr/local`，确保：
  ```
  protoc --version           # libprotoc 25.x
  ls /usr/local/lib64/cmake/protobuf
  ls /usr/local/lib64/cmake/grpc
  ```
- 若需要重新生成 `.pb` 文件，请使用同一套工具链。

### 2.4 其他
- Boost 1.78、OpenSSL、zlib、c-ares、RE2、hiredis 通过系统 RPM 提供即可。

## 3. 源码兼容性修正（一次性）
- `ChatServer/ChatGrpcClient.h`、`ChatServer2/ChatGrpcClient.h` 增加 `<mutex> <condition_variable> <atomic>`，将 `atomic<bool>` 修改为 `std::atomic<bool>`。
- `ChatServer/MysqlDao.h`、`ChatServer2/MysqlDao.h` 增加 `<condition_variable> <atomic> <chrono>`。
- `ConfigMgr::GetInstance()` 在未来补齐默认返回可消除警告。

## 4. 重新生成 protobuf/gRPC 代码（如版本不一致）
```bash
cd /opt/2025-11-bitServer/bit-tieba/server
for svc in GateServer StatusServer ChatServer ChatServer2; do
  cd "$svc"
  rm -f message.pb.* message.grpc.pb.*
  protoc --proto_path=. \
    --cpp_out=. \
    --grpc_out=. \
    --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin \
    message.proto
  cd ..
done
```

## 5. CMake 配置与编译
```bash
cd /opt/2025-11-bitServer/bit-tieba/server
rm -rf build
cmake -S . -B build -G Ninja \
  -DMYSQL_API=jdbc \
  -DMYSQLCPPCONN_INCLUDE_DIR=/opt/mysql-connector-c++-9.5.0/include \
  -DMYSQLCPPCONN_LIB=/opt/mysql-connector-c++-9.5.0/lib64/libmysqlcppconn.so \
  -DCMAKE_PREFIX_PATH="/usr/local;/usr/local/lib64;/usr" \
  -DBoost_INCLUDE_DIR=/usr/include/boost1.78 \
  -DJsonCpp_DIR=/usr/local/lib64/cmake/jsoncpp \
  -DProtobuf_DIR=/usr/local/lib64/cmake/protobuf \
  -DgRPC_DIR=/usr/local/lib64/cmake/grpc
cmake --build build -- -j1
```
- `-j1` 避免低内存环境下 cc1plus 被 OOM 杀掉；可按实际内存调整并行度。
- 如链接报错，核对 `…_LIB` 指向的实际文件是否存在。

## 6. 服务运行
- 调整各服务目录下的配置文件（数据库、Redis 地址等）。
- 启动顺序示例：
  ```
  ./build/GateServer/gateserver
  ./build/StatusServer/statusserver
  ./build/ChatServer/chatserver1
  ./build/ChatServer2/chatserver2
  ./build/VarifyServer/varifyserver
  ```
- 可使用 `ninja -C build <target>` 单独构建单个服务。

## 7. 常见问题速查
- **protobuf 版本不匹配**：看到 “generated by a newer version of protoc” 或 `#error` 时，升级运行时或重新生成 `.pb`。
- **jsoncpp string_view 符号缺失**：保证使用 1.9.5+ C++17 版本；不要删除安装生成的 `.a` 文件。
- **MySQL Connector 未找到**：确保头文件与库路径匹配（如 `libmysqlcppconn.so`）。
- **缺少 `<condition_variable>`**：补充头文件并使用 `std::` 前缀。
- **cc1plus 进程被 Kill**：降低并行度或增加 swap。
- **git pull 提示 divergent**：根据策略使用 `--no-rebase` 或 `--rebase`，在编辑器中保存合并信息。

## 8. 建议
- 将依赖包与安装脚本集中维护（如 `/opt/pkg-cache`），便于重复部署。
- 在仓库中追踪本指南，方便团队共享。
- 保持 mac 与服务器工具链版本一致，减少跨平台差异。
- 编写 systemd 或脚本统一启动/停止所有服务。
