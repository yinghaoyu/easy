#include "easy/base/LogEvent.h"
#include "easy/base/Logger.h"

#include <stdarg.h>

namespace easy
{
LogEvent::LogEvent(Logger::ptr logger,
                   LogLevel::Level level,
                   const char *file,
                   int32_t line,
                   uint32_t elapse,
                   uint32_t threadId,
                   uint32_t coroutineId,
                   Timestamp time,
                   const std::string &threadName)
    : fileName_(slash_walk(file, file)),
      line_(line),
      elapse_(elapse),
      threadId_(threadId),
      coroutineId_(coroutineId),
      timestamp_(time),
      threadName_(threadName),
      logger_(logger),
      level_(level)
{
}

void LogEvent::format(const char *fmt, ...)
{
  va_list al;
  va_start(al, fmt);
  format(fmt, al);
  va_end(al);
}

void LogEvent::format(const char *fmt, va_list al)
{
  char *buf = nullptr;
  int len = vasprintf(&buf, fmt, al);
  if (len != -1)
  {
    contentSS_ << std::string(buf, static_cast<size_t>(len));
    free(buf);
  }
}

LogEventRAII::LogEventRAII(LogEvent::ptr e) : event_(e) {}

LogEventRAII::~LogEventRAII()
{
  event_->logger()->log(event_->level(), event_);
  // if (event_->level() == log_level::fatal)
  // {
  //   abort();
  // }
}

std::stringstream &LogEventRAII::getSS()
{
  return event_->contentSS();
}
}  // namespace easy
