#ifndef __EASY_LOG_LEVEL__
#define __EASY_LOG_LEVEL__

#include <string>

namespace easy
{
enum class LogLevel
{
    OFF,
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
};

const char* toString(LogLevel level);

LogLevel fromString(const std::string& str);
}  // namespace easy

#endif
