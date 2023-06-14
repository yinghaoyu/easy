set(base_SRCS
  LogLevel.cc
  LogAppender.cc
  LogEvent.cc
  LogFormatter.cc
  Logger.cc
  Coroutine.cc
  Thread.cc
  Mutex.cc
  Scheduler.cc
  Timestamp.cc
  Timer.cc
  IOManager.cc
  common.cc
  FileUtil.cc
  Config.cc
  Env.cc
  )

add_library(easy_base ${base_SRCS})

target_link_libraries(easy_base pthread)
