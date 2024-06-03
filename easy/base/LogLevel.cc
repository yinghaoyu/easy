#include "easy/base/LogLevel.h"

namespace easy
{
const char* toString(LogLevel level)
{
    switch (level)
    {
#define XX(name) \
    case LogLevel::name: return #name; break;

        XX(TRACE);
        XX(DEBUG);
        XX(INFO);
        XX(WARN);
        XX(ERROR);
        XX(FATAL);
#undef XX
        default: return "OFF";
    }
    return "OFF";
}

LogLevel fromString(const std::string& str)
{
#define XX(level, v)            \
    if (str == #v)              \
    {                           \
        return LogLevel::level; \
    }

    XX(TRACE, trace);
    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);

    XX(TRACE, TRACE);
    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARN, WARN);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);
    return LogLevel::OFF;
#undef XX
}
}  // namespace easy
