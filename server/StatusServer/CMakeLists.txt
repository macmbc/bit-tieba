set(STATUS_SERVER_SOURCES
  AsioIOServicePool.cpp
  ChatGrpcClient.cpp
  ConfigMgr.cpp
  DistLock.cpp
  MysqlDao.cpp
  MysqlMgr.cpp
  RedisMgr.cpp
  StatusServer.cpp
  StatusServiceImpl.cpp
  message.pb.cc
  message.grpc.pb.cc
)

add_executable(statusserver ${STATUS_SERVER_SOURCES})
set_target_properties(statusserver PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(statusserver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(statusserver PRIVATE llfc_common)

if(APPLE)
  add_custom_command(TARGET statusserver POST_BUILD
    COMMAND install_name_tool -change libssl.3.dylib "@rpath/libssl.3.dylib" $<TARGET_FILE:statusserver>
  )
endif()
