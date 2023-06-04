#ifndef __EASY_DEFINED_H__
#define __EASY_DEFINED_H__

#include <assert.h>

#include "common.h"
#include "Logger.h"

#if defined __GNUC__ || defined __llvm__
#define EASY_LIKELY(x) __builtin_expect(!!(x), 1)
#define EASY_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define EASY_LIKELY(x) (x)
#define EASY_UNLIKELY(x) (x)
#endif

#define EASY_ASSERT(x)                                      \
  if (EASY_UNLIKELY(!(x)))                                  \
  {                                                         \
    EASY_LOG_ERROR(EASY_LOG_ROOT())                         \
        << "assertion: " #x << "\nbacktrace:\n"             \
        << easy::util::backtrace_to_string(100, 2, "    "); \
    assert(x);                                              \
  }

#define EASY_ASSERT_MESSAGE(x, w)                           \
  if (EASY_UNLIKELY(!(x)))                                  \
  {                                                         \
    EASY_LOG_ERROR(EASY_LOG_ROOT())                         \
        << "assertion: " #x << "\n"                         \
        << w << "\nbacktrace:\n"                            \
        << easy::util::backtrace_to_string(100, 2, "    "); \
    assert(x);                                              \
  }

#define EASY_CHECK(x)                                   \
  if (EASY_UNLIKELY(x))                                 \
  {                                                     \
    EASY_LOG_ERROR(EASY_LOG_ROOT()) << "check: " << #x; \
    assert(x);                                          \
  }

#endif
