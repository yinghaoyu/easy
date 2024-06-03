#ifndef __EASY_LOGGER_H__
#define __EASY_LOGGER_H__

#include <list>
#include <map>
#include <memory>

#include "easy/base/Fiber.h"
#include "easy/base/LogRecord.h"
#include "easy/base/LogLevel.h"
#include "easy/base/Mutex.h"
#include "easy/base/Singleton.h"
#include "easy/base/Thread.h"
#include "easy/base/noncopyable.h"

#define ELOG_LEVEL(obj, level)                                     \
    if (obj->getLevel() <= level)                                  \
    easy::LogRecordRAII(std::make_shared<easy::LogRecord>(obj,     \
                            level,                                 \
                            __FILE__,                              \
                            __LINE__,                              \
                            __FUNCTION__,                          \
                            0,                                     \
                            easy::Thread::GetCurrentThreadId(),    \
                            easy::Fiber::CurrentFiberId(),         \
                            easy::Timestamp::now(),                \
                            easy::Thread::GetCurrentThreadName())) \
        .getSS()

#define ELOG_TRACE(obj) ELOG_LEVEL(obj, easy::LogLevel::TRACE)
#define ELOG_DEBUG(obj) ELOG_LEVEL(obj, easy::LogLevel::DEBUG)
#define ELOG_INFO(obj) ELOG_LEVEL(obj, easy::LogLevel::INFO)
#define ELOG_WARN(obj) ELOG_LEVEL(obj, easy::LogLevel::WARN)
#define ELOG_ERROR(obj) ELOG_LEVEL(obj, easy::LogLevel::ERROR)
#define ELOG_FATAL(obj) ELOG_LEVEL(obj, easy::LogLevel::FATAL)

#define ELOG_FMT_LEVEL(obj, level, fmt, ...)                       \
    if (obj->getLevel() <= level)                                  \
    easy::LogRecordRAII(std::make_shared<easy::LogRecord>(obj,     \
                            level,                                 \
                            __FILE__,                              \
                            __LINE__,                              \
                            __FUNCTION__,                          \
                            0,                                     \
                            easy::Thread::GetCurrentThreadId(),    \
                            easy::Fiber::CurrentFiberId(),         \
                            easy::Timestamp::now(),                \
                            easy::Thread::GetCurrentThreadName())) \
        .getRecord()                                               \
        ->format(fmt, __VA_ARGS__)

#define ELOG_FMT_TRACE(obj, fmt, ...) ELOG_FMT_LEVEL(obj, easy::LogLevel::TRACE, fmt, __VA_ARGS__)
#define ELOG_FMT_DEBUG(obj, fmt, ...) ELOG_FMT_LEVEL(obj, easy::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define ELOG_FMT_INFO(obj, fmt, ...) ELOG_FMT_LEVEL(obj, easy::LogLevel::INFO, fmt, __VA_ARGS__)
#define ELOG_FMT_WARN(obj, fmt, ...) ELOG_FMT_LEVEL(obj, easy::LogLevel::WARN, fmt, __VA_ARGS__)
#define ELOG_FMT_ERROR(obj, fmt, ...) ELOG_FMT_LEVEL(obj, easy::LogLevel::ERROR, fmt, __VA_ARGS__)
#define ELOG_FMT_FATAL(obj, fmt, ...) ELOG_FMT_LEVEL(obj, easy::LogLevel::FATAL, fmt, __VA_ARGS__)

#define ELOG_ROOT() easy::LoggerMgr::GetInstance()->getLogger("root")
#define ELOG_NAME(name) easy::LoggerMgr::GetInstance()->getLogger(name)

namespace easy
{
class LogAppender;
class LogFormatter;

class Logger : public std::enable_shared_from_this<Logger>
{
    friend class LoggerManager;

  public:
    typedef std::shared_ptr<Logger> ptr;

    explicit Logger(const std::string& name = "root");

    void log(LogLevel level, std::shared_ptr<LogRecord> record);

    void trace(std::shared_ptr<LogRecord> record);
    void debug(std::shared_ptr<LogRecord> record);
    void info(std::shared_ptr<LogRecord> record);
    void warn(std::shared_ptr<LogRecord> record);
    void error(std::shared_ptr<LogRecord> record);
    void fatal(std::shared_ptr<LogRecord> record);

    void addAppender(std::shared_ptr<LogAppender> sink);
    void deleteAppender(std::shared_ptr<LogAppender> sink);
    void clearAppender();

    LogLevel getLevel() const { return level_; }
    void     setLevel(LogLevel val) { level_ = val; }

    const std::string& getName() const { return name_; }

    void setFormatter(std::shared_ptr<LogFormatter> val);
    void setFormatter(const std::string& val);

    std::shared_ptr<LogFormatter> getFormatter();

    std::string toYamlString();

    bool reopen();

  private:
    std::string                             name_;   // 日志名称
    LogLevel                                level_;  // 日志级别
    ReadWriteLock                           lock_;
    std::list<std::shared_ptr<LogAppender>> appenders_;  // appenders 集合
    std::shared_ptr<LogFormatter>           formatter_;  // 默认的 formatter
    Logger::ptr                             root_;       // 根日志器
};

class LoggerManager : noncopyable
{
  public:
    LoggerManager();
    Logger::ptr getLogger(const std::string& name);

    Logger::ptr getRoot() const { return root_; }

    std::string toYamlString();

    bool reopen();

  private:
    ReadWriteLock                      lock_;
    std::map<std::string, Logger::ptr> loggers_;
    Logger::ptr                        root_;  // 根日志器
};

typedef easy::Singleton<LoggerManager> LoggerMgr;

}  // namespace easy

#endif
