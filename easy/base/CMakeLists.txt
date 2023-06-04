set(base_SRCS
  LogLevel.cc
  LogAppender.cc
  LogEvent.cc
  LogFormatter.cc
  Logger.cc
  Coroutine.cc
  Thread.cc
  Mutex.cc
  common.cc
  )

add_library(easy_base ${base_SRCS})

target_link_libraries(easy_base pthread)
