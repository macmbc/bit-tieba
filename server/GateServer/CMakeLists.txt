set(GATE_SERVER_SOURCES
  AsioIOServicePool.cpp
  CServer.cpp
  ConfigMgr.cpp
  GateServer.cpp
  HttpConnection.cpp
  LogicSystem.cpp
  MysqlDao.cpp
  MysqlMgr.cpp
  RedisMgr.cpp
  StatusGrpcClient.cpp
  VerifyGrpcClient.cpp
  message.pb.cc
  message.grpc.pb.cc
)

add_executable(gateserver ${GATE_SERVER_SOURCES})
set_target_properties(gateserver PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(gateserver PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  /usr/local/include
)
target_link_libraries(gateserver PRIVATE llfc_common)

target_include_directories(llfc_common INTERFACE
  ${Boost_INCLUDE_DIRS}
  ${HIREDIS_INCLUDE_DIR}
  /usr/local/include
  ${MYSQLCPPCONN_INCLUDE_DIR}
)

if(APPLE)
  add_custom_command(TARGET gateserver POST_BUILD
    COMMAND install_name_tool -change libssl.3.dylib "@rpath/libssl.3.dylib" $<TARGET_FILE:gateserver>
  )
endif()
