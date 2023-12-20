#ifndef __EASY_LOG_LEVEL__
#define __EASY_LOG_LEVEL__

#include <string>

namespace easy {
class LogLevel {
 public:
  enum Level {
    off,
    trace,
    debug,
    info,
    warn,
    error,
    fatal,
  };

  static const char* toString(LogLevel::Level level);

  static LogLevel::Level fromString(const std::string& str);
};
}  // namespace easy

#endif
