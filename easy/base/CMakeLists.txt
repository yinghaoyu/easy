set(base_SRCS
  LogAppender.cc
  LogEvent.cc
  LogFormatter.cc
  Logger.cc
  common.cc
  )

add_library(easy_base ${base_SRCS})

target_link_libraries(easy_base)
