#ifndef __EASY_LOG_SINK_H__
#define __EASY_LOG_SINK_H__

#include "easy/base/LogLevel.h"
#include "easy/base/Mutex.h"
#include "easy/base/Timestamp.h"

#include <fstream>
#include <memory>

namespace easy
{
class Logger;
class LogEvent;
class LogFormatter;

// 抽象类
class LogAppender
{
  friend class Logger;

 public:
  typedef std::shared_ptr<LogAppender> ptr;

  LogAppender() = default;
  virtual ~LogAppender() = default;

  virtual void log(std::shared_ptr<Logger> logger,
                   LogLevel::Level level,
                   std::shared_ptr<LogEvent> event) = 0;

  void setFormatter(std::shared_ptr<LogFormatter> val);

  std::shared_ptr<LogFormatter> getFormatter();

  bool shouldLog(LogLevel::Level) const;

  LogLevel::Level level() const { return level_; }

  void setLevel(LogLevel::Level val) { level_ = val; }

  virtual bool reopen() { return true; }

 protected:
  LogLevel::Level level_ = LogLevel::trace;
  bool hasFormatter_ = false;
  SpinLock lock_;
  std::shared_ptr<LogFormatter> formatter_;
};

// 输出到控制台
class ConsoleLogAppender : public LogAppender
{
 public:
  typedef std::shared_ptr<ConsoleLogAppender> ptr;

  void log(std::shared_ptr<Logger> logger,
           LogLevel::Level level,
           std::shared_ptr<LogEvent> event) override;
};

// 输出到文件
class FileLogAppender : public LogAppender
{
 public:
  typedef std::shared_ptr<FileLogAppender> ptr;

  FileLogAppender(const std::string &filename);

  void log(std::shared_ptr<Logger> logger,
           LogLevel::Level level,
           std::shared_ptr<LogEvent> event) override;

  bool reopen() override;

 private:
  std::string filename_;
  std::ofstream filestream_;
  Timestamp last_time_;

  static const int kRoutineInterval = 3;
};

}  // namespace easy

#endif
