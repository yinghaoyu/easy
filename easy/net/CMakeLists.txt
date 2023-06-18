set(net_SRCS
  Address.cc
  )

add_library(easy_net ${net_SRCS})

target_link_libraries(easy_net easy_base)
