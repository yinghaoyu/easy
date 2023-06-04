#ifndef __EASY_LOGGER_H__
#define __EASY_LOGGER_H__

#include <list>
#include <map>
#include <memory>

#include "LogLevel.h"
#include "Singleton.h"
#include "common.h"
#include "noncopyable.h"

namespace easy
{
class LogEvent;
class LogAppender;
class LogFormatter;

// 日志器
class Logger : public std::enable_shared_from_this<Logger>
{
  friend class LoggerManager;

 public:
  typedef std::shared_ptr<Logger> ptr;
  // typedef RWSpinlock RWMutexType;

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
  // RWMutexType m_mutex;
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
  // RWMutexType m_mutex;
  std::map<std::string, Logger::ptr> loggers_;
  Logger::ptr root_;  // 根日志器
};

typedef easy::Signleton<LoggerManager> LoggerMgr;

}  // namespace easy

#endif
