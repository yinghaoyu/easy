#ifndef __EASY_LOG_RECORD_H__
#define __EASY_LOG_RECORD_H__

#include "easy/base/LogLevel.h"
#include "easy/base/Timestamp.h"

#include <string.h>  // strrchr
#include <memory>
#include <sstream>

namespace easy
{
class Logger;

class LogRecord
{
  public:
    typedef std::shared_ptr<LogRecord> ptr;

    explicit LogRecord(std::shared_ptr<Logger> logger, LogLevel level, const char* file, int32_t line, const char* function, uint32_t elapse,
        uint32_t threadId, uint32_t fiberIId, Timestamp time, const std::string& thread_name);

    ~LogRecord() = default;

    const char*             getFileName() const { return fileName_; }
    int32_t                 getLine() const { return line_; }
    const char*             getFunction() const { return function_; }
    uint32_t                getElapse() const { return elapse_; }
    uint32_t                getThreadId() const { return threadId_; }
    uint32_t                getFiberId() const { return fiberId_; }
    Timestamp               getTimestamp() const { return timestamp_; }
    const std::string&      getThreadName() const { return threadName_; }
    std::string             getContent() const { return ss_.str(); }
    std::shared_ptr<Logger> getLogger() const { return logger_; }
    LogLevel                getLevel() const { return level_; }
    std::stringstream&      getSS() { return ss_; }

    void format(const char* fmt, ...);
    void format(const char* fmt, va_list al);

  private:
    inline const char* get_base_name(const char* filename) { return strrchr(filename, '/') ? strrchr(filename, '/') + 1 : filename; }

    const char*             fileName_ = nullptr;  // 文件名
    int32_t                 line_     = 0;        // 行号
    const char*             function_ = nullptr;  // 函数名
    uint32_t                elapse_   = 0;        // 程序启动开始到现在的毫秒数
    uint32_t                threadId_ = 0;        // 线程id
    uint32_t                fiberId_  = 0;        // 协程id
    Timestamp               timestamp_;           // 时间戳
    std::string             threadName_;          // 线程名
    std::stringstream       ss_;                  // 日志内容流
    std::shared_ptr<Logger> logger_;              // 日志事件归属的日志器
    LogLevel                level_;
};

class LogRecordRAII
{
  public:
    LogRecordRAII(LogRecord::ptr e);
    ~LogRecordRAII();
    LogRecord::ptr     getRecord() const { return record_; }
    std::stringstream& getSS();

  private:
    LogRecord::ptr record_;
};

}  // namespace easy

#endif
