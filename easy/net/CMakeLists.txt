set(net_SRCS
  Address.cc
  Socket.cc
  Buffer.cc
  TcpServer.cc
  )

add_library(easy_net ${net_SRCS})

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

target_link_libraries(easy_net easy_base ${OPENSSL_LIBRARIES})
