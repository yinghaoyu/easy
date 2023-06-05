#ifndef __EASY_LOGGER_H__
#define __EASY_LOGGER_H__

#include <list>
#include <map>
#include <memory>

#include "Coroutine.h"
#include "LogEvent.h"
#include "LogLevel.h"
#include "Singleton.h"
#include "Thread.h"
#include "common.h"
#include "noncopyable.h"

#define EASY_LOG_LEVEL(obj, lvl)                                    \
  if (obj->level() <= lvl)                                          \
  easy::LogEventRAII(std::make_shared<easy::LogEvent>(              \
                         obj, lvl, __FILE__, __LINE__, 0,           \
                         easy::Thread::GetCurrentThreadId(),    \
                         easy::Coroutine::CurrentCoroutineId(),     \
                         easy::util::timestamp(),                   \
                         easy::Thread::GetCurrentThreadName())) \
      .getSS()

#define EASY_LOG_TRACE(obj) EASY_LOG_LEVEL(obj, easy::LogLevel::trace)
#define EASY_LOG_DEBUG(obj) EASY_LOG_LEVEL(obj, easy::LogLevel::debug)
#define EASY_LOG_INFO(obj) EASY_LOG_LEVEL(obj, easy::LogLevel::info)
#define EASY_LOG_WARN(obj) EASY_LOG_LEVEL(obj, easy::LogLevel::warn)
#define EASY_LOG_ERROR(obj) EASY_LOG_LEVEL(obj, easy::LogLevel::error)
#define EASY_LOG_FATAL(obj) EASY_LOG_LEVEL(obj, easy::LogLevel::fatal)

#define EASY_LOG_FMT_LEVEL(obj, lvl, fmt, ...)                      \
  if (obj->level() <= lvl)                                          \
  easy::LogEventRAII(std::make_shared<easy::LogEvent>(              \
                         obj, lvl, __FILE__, __LINE__, 0,           \
                         easy::Thread::GetCurrentThreadId(),    \
                         easy::Coroutine::CurrentCoroutineId(),     \
                         easy::util::timestamp(),                   \
                         easy::Thread::GetCurrentThreadName())) \
      .event()                                                      \
      ->format(fmt, __VA_ARGS__)

#define EASY_LOG_FMT_TRACE(obj, fmt, ...) \
  EASY_LOG_FMT_LEVEL(obj, easy::LogLevel::trace, fmt, __VA_ARGS__)
#define EASY_LOG_FMT_DEBUG(obj, fmt, ...) \
  EASY_LOG_FMT_LEVEL(obj, easy::LogLevel::debug, fmt, __VA_ARGS__)
#define EASY_LOG_FMT_INFO(obj, fmt, ...) \
  EASY_LOG_FMT_LEVEL(obj, easy::LogLevel::info, fmt, __VA_ARGS__)
#define EASY_LOG_FMT_WARN(obj, fmt, ...) \
  EASY_LOG_FMT_LEVEL(obj, easy::LogLevel::warn, fmt, __VA_ARGS__)
#define EASY_LOG_FMT_ERROR(obj, fmt, ...) \
  EASY_LOG_FMT_LEVEL(obj, easy::LogLevel::error, fmt, __VA_ARGS__)
#define EASY_LOG_FMT_FATAL(obj, fmt, ...) \
  EASY_LOG_FMT_LEVEL(obj, easy::LogLevel::fatal, fmt, __VA_ARGS__)

#define EASY_LOG_ROOT() easy::LoggerMgr::GetInstance()->getLogger("root")
#define EASY_LOG_NAME(name) easy::LoggerMgr::GetInstance()->getLogger(name)

namespace easy
{
class LogAppender;
class LogFormatter;

// 日志器
class Logger : public std::enable_shared_from_this<Logger>
{
  friend class LoggerManager;

 public:
  typedef std::shared_ptr<Logger> ptr;

  explicit Logger(const std::string &name = "root");
  void log(LogLevel::Level level, std::shared_ptr<LogEvent> event);

  void trace(std::shared_ptr<LogEvent> event);
  void debug(std::shared_ptr<LogEvent> event);
  void info(std::shared_ptr<LogEvent> event);
  void warn(std::shared_ptr<LogEvent> event);
  void error(std::shared_ptr<LogEvent> event);
  void fatal(std::shared_ptr<LogEvent> event);

  void addAppender(std::shared_ptr<LogAppender> sink);
  void deleteAppender(std::shared_ptr<LogAppender> sink);
  void clearAppender();
  LogLevel::Level level() const { return level_; }
  void setLevel(LogLevel::Level val) { level_ = val; }

  const std::string &name() const { return name_; }

  void setFormatter(std::shared_ptr<LogFormatter> val);
  void setFormatter(const std::string &val);
  std::shared_ptr<LogFormatter> formatter();

  bool reopen();

 private:
  std::string name_;       // 日志名称
  LogLevel::Level level_;  // 日志级别
  RWMutex mutex_;
  std::list<std::shared_ptr<LogAppender>> appenders_;  // appenders 集合
  std::shared_ptr<LogFormatter> formatter_;            // 默认的 formatter
  Logger::ptr root_;                                   // 根日志器
};

class LoggerManager : noncopyable
{
 public:
  LoggerManager();
  Logger::ptr getLogger(const std::string &name);

  bool reopen();

 private:
  RWMutex mutex_;
  std::map<std::string, Logger::ptr> loggers_;
  Logger::ptr root_;  // 根日志器
};

typedef easy::Signleton<LoggerManager> LoggerMgr;

}  // namespace easy

#endif
