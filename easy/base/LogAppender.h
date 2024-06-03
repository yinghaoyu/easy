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
class LogRecord;
class LogFormatter;

// 抽象类
class LogAppender
{
    friend class Logger;

  public:
    typedef std::shared_ptr<LogAppender> ptr;

    LogAppender()          = default;
    virtual ~LogAppender() = default;

    virtual void log(std::shared_ptr<Logger> logger, LogLevel level, std::shared_ptr<LogRecord> event) = 0;

    virtual std::string toYamlString() = 0;

    void setFormatter(std::shared_ptr<LogFormatter> val);

    std::shared_ptr<LogFormatter> getFormatter();

    bool shouldLog(LogLevel) const;

    LogLevel getLevel() const { return level_; }

    void setLevel(LogLevel val) { level_ = val; }

    virtual bool reopen() { return true; }

  protected:
    LogLevel                      level_        = LogLevel::TRACE;
    bool                          hasFormatter_ = false;
    SpinLock                      lock_;
    std::shared_ptr<LogFormatter> formatter_;
};

// 输出到控制台
class ConsoleLogAppender : public LogAppender
{
  public:
    typedef std::shared_ptr<ConsoleLogAppender> ptr;

    void log(std::shared_ptr<Logger> logger, LogLevel level, std::shared_ptr<LogRecord> event) override;

    std::string toYamlString() override;
};

// 输出到文件
class FileLogAppender : public LogAppender
{
  public:
    typedef std::shared_ptr<FileLogAppender> ptr;

    FileLogAppender(const std::string& filename);

    void log(std::shared_ptr<Logger> logger, LogLevel level, std::shared_ptr<LogRecord> event) override;

    std::string toYamlString() override;

    bool reopen() override;

  private:
    std::string   filename_;
    std::ofstream filestream_;
    Timestamp     lastTime_;

    static const int kRoutineIntervalMs = 3000;
};

}  // namespace easy

#endif
