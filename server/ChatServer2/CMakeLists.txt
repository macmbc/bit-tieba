set(CHAT_SERVER2_SOURCES
  AsioIOServicePool.cpp
  CServer.cpp
  CSession.cpp
  ChatGrpcClient.cpp
  ChatServer.cpp
  ChatServiceImpl.cpp
  ConfigMgr.cpp
  DistLock.cpp
  LogicSystem.cpp
  MsgNode.cpp
  MysqlDao.cpp
  MysqlMgr.cpp
  RedisMgr.cpp
  StatusGrpcClient.cpp
  UserMgr.cpp
  message.pb.cc
  message.grpc.pb.cc
)

add_executable(chatserver2 ${CHAT_SERVER2_SOURCES})
set_target_properties(chatserver2 PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(chatserver2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(chatserver2 PRIVATE llfc_common)

if(APPLE)
  add_custom_command(TARGET chatserver2 POST_BUILD
    COMMAND install_name_tool -change libssl.3.dylib "@rpath/libssl.3.dylib" $<TARGET_FILE:chatserver2>
  )
endif()
