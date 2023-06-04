#ifndef __EASY_DEFINED_H__
#define __EASY_DEFINED_H__

#include <assert.h>

#if defined __GNUC__ || defined __llvm__
#define EASY_LIKELY(x) __builtin_expect(!!(x), 1)
#define EASY_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define EASY_LIKELY(x) (x)
#define EASY_UNLIKELY(x) (x)
#endif

#define EASY_ASSERT(x)                                \
  if (EASY_UNLIKELY(!(x)))                            \
  {                                                   \
    EASY_LOG_ERROR(EASY_LOG_ROOT())                   \
        << "assertion: " #x << "\nbacktrace:\n"       \
        << util::backtrace_to_string(100, 2, "    "); \
    assert(x);                                        \
  }

#define EASY_ASSERT_MESSAGE(x, w)                     \
  if (EASY_UNLIKELY(!(x)))                            \
  {                                                   \
    EASY_LOG_ERROR(EASY_LOG_ROOT())                   \
        << "assertion: " #x << "\n"                   \
        << w << "\nbacktrace:\n"                      \
        << util::backtrace_to_string(100, 2, "    "); \
    assert(x);                                        \
  }

#define EASY_CHECK(x)                                   \
  if (EASY_UNLIKELY(x))                                 \
  {                                                     \
    EASY_LOG_ERROR(EASY_LOG_ROOT()) << "check: " << #x; \
    assert(x);                                          \
  }

#include "Coroutine.h"
#include "LogAppender.h"
#include "LogEvent.h"
#include "LogFormatter.h"
#include "LogLevel.h"
#include "Logger.h"

#define EASY_LOG_LEVEL(obj, lvl)                                            \
  if (obj->level() <= lvl)                                                  \
  easy::LogEventRAII(std::make_shared<easy::LogEvent>(                      \
                         obj, lvl, __FILE__, __LINE__, 0, 0 /*ThreadId()*/, \
                         easy::Coroutine::CurrentCoroutineId(),             \
                         easy::util::timestamp() /*time*/,                  \
                         "rain" /*Thread::GetName()*/))                     \
      .getSS()

#define EASY_LOG_TRACE(obj) EASY_LOG_LEVEL(obj, easy::LogLevel::trace)
#define EASY_LOG_DEBUG(obj) EASY_LOG_LEVEL(obj, easy::LogLevel::debug)
#define EASY_LOG_INFO(obj) EASY_LOG_LEVEL(obj, easy::LogLevel::info)
#define EASY_LOG_WARN(obj) EASY_LOG_LEVEL(obj, easy::LogLevel::warn)
#define EASY_LOG_ERROR(obj) EASY_LOG_LEVEL(obj, easy::LogLevel::error)
#define EASY_LOG_FATAL(obj) EASY_LOG_LEVEL(obj, easy::LogLevel::fatal)

#define EASY_LOG_FMT_LEVEL(obj, lvl, fmt, ...)                                \
  if (obj->level() <= lvl)                                                    \
  easy::LogEventRAII(                                                         \
      std::make_shared<easy::LogEvent>(obj, lvl, __FILE__, __LINE__, 0,       \
                                       0 /*easy::GetThreadId()*/,             \
                                       easy::Coroutine::CurrentCoroutineId(), \
                                       easy::util::timestamp() /*time(0)*/,   \
                                       "rain" /*easy::Thread::GetName()*/))   \
      .event()                                                                \
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

#endif
