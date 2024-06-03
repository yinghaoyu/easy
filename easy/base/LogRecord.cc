#include "easy/base/LogRecord.h"
#include "easy/base/Logger.h"

#include <stdarg.h>

namespace easy
{
LogRecord::LogRecord(Logger::ptr logger, LogLevel level, const char* file, int32_t line, const char* function, uint32_t elapse, uint32_t threadId,
    uint32_t fiberId, Timestamp time, const std::string& threadName)
    : fileName_(get_base_name(file)),
      line_(line),
      function_(function),
      elapse_(elapse),
      threadId_(threadId),
      fiberId_(fiberId),
      timestamp_(time),
      threadName_(threadName),
      logger_(logger),
      level_(level)
{}

void LogRecord::format(const char* fmt, ...)
{
    va_list al;
    va_start(al, fmt);
    format(fmt, al);
    va_end(al);
}

void LogRecord::format(const char* fmt, va_list al)
{
    char* buf = nullptr;
    int   len = vasprintf(&buf, fmt, al);
    if (len != -1)
    {
        ss_ << std::string(buf, static_cast<size_t>(len));
        free(buf);
    }
}

LogRecordRAII::LogRecordRAII(LogRecord::ptr record) : record_(record) {}

LogRecordRAII::~LogRecordRAII()
{
    record_->getLogger()->log(record_->getLevel(), record_);
    // if (record_->level() == LogLevel::FATAL)
    // {
    //   abort();
    // }
}

std::stringstream& LogRecordRAII::getSS() { return record_->getSS(); }
}  // namespace easy
