#ifndef __EASY_LOG_EVENT_H__
#define __EASY_LOG_EVENT_H__

#include "LogLevel.h"

#include <memory>
#include <sstream>

namespace easy
{
class Logger;

// 日志事件
class LogEvent
{
 public:
  typedef std::shared_ptr<LogEvent> ptr;

  explicit LogEvent(std::shared_ptr<Logger> logger,
                    LogLevel::Level level,
                    const char *file,
                    int32_t line,
                    uint32_t elapse,
                    uint32_t thread_id,
                    uint32_t coroutine_id,
                    int64_t time,
                    const std::string &thread_name);

  ~LogEvent() = default;

  const char *fileName() const { return fileName_; }
  int32_t line() const { return line_; }
  uint32_t elapse() const { return elapse_; }
  uint32_t threadId() const { return threadId_; }
  uint32_t coroutineId() const { return coroutineId_; }
  int64_t timestamp() const { return timestamp_; }
  const std::string &threadName() const { return threadName_; }
  std::string content() const { return contentSS_.str(); }
  std::shared_ptr<Logger> logger() const { return logger_; }
  LogLevel::Level level() const { return level_; }

  std::stringstream &contentSS() { return contentSS_; }
  void format(const char *fmt, ...);
  void format(const char *fmt, va_list al);

 private:
  inline const char *slash_walk(const char *str, const char *last_slash)
  {
    if (*str == '\0')
    {
      return last_slash;
    }
    else if (*str == '/')
    {
      return slash_walk(str + 1, str + 1);
    }
    else
    {
      return slash_walk(str + 1, last_slash);
    }
  }

 private:
  const char *fileName_ = nullptr;  // 文件名
  int32_t line_ = 0;                // 行号
  uint32_t elapse_ = 0;             // 程序启动开始到现在的毫秒数
  uint32_t threadId_ = 0;           // 线程id
  uint32_t coroutineId_ = 0;        // 协程id
  int64_t timestamp_ = 0;           // 时间戳
  std::string threadName_;          // 线程名
  std::stringstream contentSS_;     // 日志内容流
  std::shared_ptr<Logger> logger_;  // 日志事件归属的日志器
  LogLevel::Level level_;
};

class LogEventRAII
{
 public:
  LogEventRAII(LogEvent::ptr e);
  ~LogEventRAII();
  LogEvent::ptr event() const { return event_; }
  std::stringstream &getSS();

 private:
  LogEvent::ptr event_;
};

}  // namespace easy

#endif
