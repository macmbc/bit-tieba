cmake_minimum_required(VERSION 3.20)
project(llfc_backend LANGUAGES CXX)

# ---- basics ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Homebrew 常见前缀辅助
if(APPLE)
  foreach(pfx /opt/homebrew /usr/local)
    if(EXISTS "${pfx}/opt/boost@1.85")
      list(APPEND CMAKE_PREFIX_PATH "${pfx}/opt/boost@1.85")
    endif()
    if(EXISTS "${pfx}/opt/mysql-connector-c++")
      list(APPEND CMAKE_PREFIX_PATH "${pfx}/opt/mysql-connector-c++")
    endif()
    if(EXISTS "${pfx}")
      list(APPEND CMAKE_PREFIX_PATH "${pfx}")
    endif()
  endforeach()
endif()

# ---- dependencies ----
find_package(Threads REQUIRED)
find_package(Boost 1.70 REQUIRED COMPONENTS system filesystem)
# set(CMAKE_PREFIX_PATH "/usr/local/lib/cmake/jsoncpp${CMAKE_PREFIX_PATH}")
# find_package(JsonCpp CONFIG REQUIRED NAMES jsoncpp JsonCpp)
# find_package(JsonCpp REQUIRED)
set(JsonCpp_DIR "/usr/local/lib/cmake/jsoncpp")

find_package(JsonCpp CONFIG REQUIRED NAMES jsoncpp JsonCpp)
# find_package(PkgConfig REQUIRED)
# pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# if(NOT TARGET JsonCpp::JsonCpp)
#     add_library(JsonCpp::JsonCpp UNKNOWN IMPORTED)
#     set_target_properties(JsonCpp::JsonCpp PROPERTIES
#         INTERFACE_INCLUDE_DIRECTORIES "${JSONCPP_INCLUDE_DIRS}"
#         INTERFACE_LINK_LIBRARIES "${JSONCPP_LIBRARIES}"
#     )
# endif()

if(NOT TARGET protobuf::libprotobuf)
  find_package(Protobuf CONFIG REQUIRED)
endif()
find_package(OpenSSL REQUIRED)
find_package(gRPC CONFIG REQUIRED)


# hiredis（Homebrew 无官方Config，手动找）
find_path(HIREDIS_INCLUDE_DIR hiredis.h
  HINTS /opt/homebrew/include /usr/local/include
  PATH_SUFFIXES hiredis)
find_library(HIREDIS_LIBRARY hiredis
  HINTS /opt/homebrew/lib /usr/local/lib)
if(NOT HIREDIS_INCLUDE_DIR OR NOT HIREDIS_LIBRARY)
  message(FATAL_ERROR "hiredis not found. Try: brew install hiredis")
endif()

# ---- MySQL Connector/C++ : 先用官方 package，失败再回退 ----
# 选择使用的 API：jdbc（经典/旧API）或 xdevapi（新API）
set(MYSQL_API "jdbc" CACHE STRING "Which MySQL Connector/C++ API to use: jdbc or xdevapi")
set_property(CACHE MYSQL_API PROPERTY STRINGS jdbc xdevapi)

set(MYSQL_PACKAGE_FOUND OFF)
find_package(mysql-connector-c++ CONFIG QUIET)
if(TARGET mysql::concpp OR TARGET mysql::concpp-jdbc)
  set(MYSQL_PACKAGE_FOUND ON)
endif()

if(MYSQL_PACKAGE_FOUND)
  if(MYSQL_API STREQUAL "jdbc")
    if(NOT TARGET mysql::concpp-jdbc)
      message(FATAL_ERROR "mysql-connector-c++ found but target mysql::concpp-jdbc missing (package too new/without legacy JDBC?). Try MYSQL_API=xdevapi or use official tarball with JDBC enabled.")
    endif()
    set(MYSQL_TARGET mysql::concpp-jdbc)
  else()
    if(NOT TARGET mysql::concpp)
      message(FATAL_ERROR "mysql-connector-c++ found but target mysql::concpp missing.")
    endif()
    set(MYSQL_TARGET mysql::concpp)
  endif()
else()
  # ---- Fallback: 手写路径搜索（兼容多布局）----
  set(MYSQLCPPCONN_ROOT "" CACHE PATH "Root directory of MySQL Connector/C++ (optional)")

  # 头文件：三选一即可
  set(_mysql_inc_hints
      /opt/homebrew/opt/mysql-connector-c++/include
      /usr/local/opt/mysql-connector-c++/include
      /opt/homebrew/include
      /usr/local/include
  )
  if(MYSQLCPPCONN_ROOT)
    list(APPEND _mysql_inc_hints
      "${MYSQLCPPCONN_ROOT}/include"
      "${MYSQLCPPCONN_ROOT}/include/mysql"
      "${MYSQLCPPCONN_ROOT}/include/mysqlx"
      "${MYSQLCPPCONN_ROOT}")  # 以防 include 就在根下
  endif()

  find_path(MYSQLCPPCONN_INCLUDE_DIR
    NAMES mysql/jdbc.h jdbc/mysql_driver.h mysqlx/xdevapi.h
    HINTS ${_mysql_inc_hints}
  )

  # 库名：mysqlcppconn8（新）或 mysqlcppconn（旧）
  set(_mysql_lib_hints
      /opt/homebrew/opt/mysql-connector-c++/lib
      /usr/local/opt/mysql-connector-c++/lib
      /opt/homebrew/lib
      /usr/local/lib
  )
  if(MYSQLCPPCONN_ROOT)
    list(APPEND _mysql_lib_hints
      "${MYSQLCPPCONN_ROOT}/lib"
      "${MYSQLCPPCONN_ROOT}/lib64"
    )
  endif()

  find_library(MYSQLCPPCONN_LIB
    NAMES mysqlcppconn8 mysqlcppconn
    HINTS ${__mysql_lib_hints} ${_mysql_lib_hints}
  )

  if(NOT MYSQLCPPCONN_INCLUDE_DIR OR NOT MYSQLCPPCONN_LIB)
    message(FATAL_ERROR
      "MySQL Connector/C++ headers or library not found.\n"
      "Solutions:\n"
      "  1) brew install mysql-connector-c++  并确保其在 CMAKE_PREFIX_PATH 中\n"
      "  2) 或下载官方 8/9.x 压缩包，解压后 -DMYSQLCPPCONN_ROOT=<解压目录>\n"
      "     (需包含 include/mysql 或 include/jdbc 或 include/mysqlx 及 lib/libmysqlcppconn*.dylib)\n"
    )
  endif()
endif()

# ---- 公共接口目标 ----
add_library(llfc_common INTERFACE)
target_compile_features(llfc_common INTERFACE cxx_std_17)
target_include_directories(llfc_common INTERFACE
  ${Boost_INCLUDE_DIRS}
  ${HIREDIS_INCLUDE_DIR}
  /usr/local/include
)

if(MYSQL_PACKAGE_FOUND)
  # 使用导出的目标自动带 include 与 link
  target_link_libraries(llfc_common INTERFACE ${MYSQL_TARGET})
else()
  # 回退路径：手动 include + link
  target_include_directories(llfc_common INTERFACE ${MYSQLCPPCONN_INCLUDE_DIR})
  target_link_libraries(llfc_common INTERFACE ${MYSQLCPPCONN_LIB})
endif()

target_link_libraries(llfc_common INTERFACE
  Threads::Threads
  Boost::system
  Boost::filesystem
  JsonCpp::JsonCpp
  ${HIREDIS_LIBRARY}
  gRPC::grpc++
  gRPC::grpc
  protobuf::libprotobuf
  OpenSSL::SSL
  OpenSSL::Crypto
)

target_include_directories(llfc_common INTERFACE
  ${Boost_INCLUDE_DIRS}
  ${HIREDIS_INCLUDE_DIR}
  /usr/local/include
  ${MYSQLCPPCONN_INCLUDE_DIR}
)

# ---- rpath for macOS ----
if(APPLE)
  set(_extra_rpaths "@executable_path/../lib")

  get_target_property(_openssl_lib OpenSSL::SSL IMPORTED_LOCATION_RELEASE)
  if(NOT _openssl_lib)
    get_target_property(_openssl_lib OpenSSL::SSL IMPORTED_LOCATION)
  endif()
  if(NOT _openssl_lib AND DEFINED OPENSSL_SSL_LIBRARY)
    set(_openssl_lib "${OPENSSL_SSL_LIBRARY}")
  endif()

  if(_openssl_lib AND NOT _openssl_lib STREQUAL "OPENSSL_SSL_LIBRARY-NOTFOUND")
    get_filename_component(_openssl_dir "${_openssl_lib}" DIRECTORY)
    list(APPEND _extra_rpaths "${_openssl_dir}")
  endif()

  if(MYSQL_PACKAGE_FOUND)
    get_target_property(_mysql_lib ${MYSQL_TARGET} IMPORTED_LOCATION_RELEASE)
    if(NOT _mysql_lib)
      get_target_property(_mysql_lib ${MYSQL_TARGET} IMPORTED_LOCATION)
    endif()
  else()
    set(_mysql_lib "${MYSQLCPPCONN_LIB}")
  endif()

  if(_mysql_lib AND NOT _mysql_lib STREQUAL "MYSQLCPPCONN_LIB-NOTFOUND")
    get_filename_component(_mysql_dir "${_mysql_lib}" DIRECTORY)
    list(APPEND _extra_rpaths "${_mysql_dir}")
  endif()

  list(REMOVE_DUPLICATES _extra_rpaths)
  list(APPEND CMAKE_BUILD_RPATH ${_extra_rpaths})
  list(APPEND CMAKE_INSTALL_RPATH ${_extra_rpaths})
endif()

# ---- subdirs (按源码存在时启用) ----
add_subdirectory(GateServer)
add_subdirectory(StatusServer)
add_subdirectory(ChatServer)
add_subdirectory(ChatServer2)
