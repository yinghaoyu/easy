#include "easy/base/LogLevel.h"

namespace easy
{
const char *LogLevel::toString(LogLevel::Level level)
{
  switch (level)
  {
#define XX(name)       \
  case LogLevel::name: \
    return #name;      \
    break;

    XX(trace);
    XX(debug);
    XX(info);
    XX(warn);
    XX(error);
    XX(fatal);
#undef XX
  default:
    return "off";
  }
  return "off";
}

LogLevel::Level LogLevel::fromString(const std::string &str)
{
#define XX(level, v)        \
  if (str == #v)            \
  {                         \
    return LogLevel::level; \
  }

  XX(trace, trace);
  XX(debug, debug);
  XX(info, info);
  XX(warn, warn);
  XX(error, error);
  XX(fatal, fatal);

  XX(trace, TRACE);
  XX(debug, DEBUG);
  XX(info, INFO);
  XX(warn, WARN);
  XX(error, ERROR);
  XX(fatal, FATAL);
  return LogLevel::off;
#undef XX
}
}  // namespace easy
