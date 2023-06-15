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
  hook.cc
  FdManager.cc
  )

add_library(easy_base ${base_SRCS})

find_library(YAMLCPP yaml-cpp)

target_link_libraries(easy_base dl pthread yaml-cpp) # dl for dlsym
