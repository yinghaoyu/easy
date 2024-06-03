#ifndef __EASY_LOG_FORMATTER_H__
#define __EASY_LOG_FORMATTER_H__

#include "easy/base/LogRecord.h"
#include "easy/base/Logger.h"

#include <inttypes.h>
#include <memory.h>
#include <ctime>
#include <memory>
#include <vector>

namespace easy
{
class LogFormatter
{
  public:
    typedef std::shared_ptr<LogFormatter> ptr;

    explicit LogFormatter(const std::string& pattern);
    ~LogFormatter() = default;

    std::string format(Logger::ptr logger, LogLevel level, LogRecord::ptr record);

    std::ostream& format(std::ostream& ofs, Logger::ptr logger, LogLevel level, LogRecord::ptr record);

  public:
    class FormatUnit
    {
      public:
        typedef std::shared_ptr<FormatUnit> ptr;
        virtual ~FormatUnit()                                                                            = default;
        virtual void format(std::ostream& os, Logger::ptr logger, LogLevel level, LogRecord::ptr record) = 0;
    };

    bool error() const { return error_; }

    const std::string getPattern() const { return pattern_; }

  private:
    void compile(const std::string& pattern);

  private:
    std::string                  pattern_;
    std::vector<FormatUnit::ptr> units_;
    bool                         error_ = false;
};

class MessageFormatUnit : public LogFormatter::FormatUnit
{
  public:
    MessageFormatUnit(const std::string& str = "") {}

    void format(std::ostream& os, Logger::ptr logger, LogLevel level, LogRecord::ptr record) override { os << record->getContent(); }
};

class LevelFormatUnit : public LogFormatter::FormatUnit
{
  public:
    LevelFormatUnit(const std::string& str = "") {}

    void format(std::ostream& os, Logger::ptr logger, LogLevel level, LogRecord::ptr record) override { os << toString(level); }
};

class ElapseFormatUnit : public LogFormatter::FormatUnit
{
  public:
    ElapseFormatUnit(const std::string& str = "") {}

    void format(std::ostream& os, Logger::ptr logger, LogLevel level, LogRecord::ptr record) override { os << record->getElapse(); }
};

class NameFormatUnit : public LogFormatter::FormatUnit
{
  public:
    NameFormatUnit(const std::string& str = "") {}

    void format(std::ostream& os, Logger::ptr logger, LogLevel level, LogRecord::ptr record) override { os << record->getLogger()->getName(); }
};

class ThreadIdFormatUnit : public LogFormatter::FormatUnit
{
  public:
    ThreadIdFormatUnit(const std::string& str = "") {}

    void format(std::ostream& os, Logger::ptr logger, LogLevel level, LogRecord::ptr record) override { os << record->getThreadId(); }
};

class FiberIdFormatUnit : public LogFormatter::FormatUnit
{
  public:
    FiberIdFormatUnit(const std::string& str = "") {}

    void format(std::ostream& os, Logger::ptr logger, LogLevel level, LogRecord::ptr record) override { os << record->getFiberId(); }
};

class ThreadNameFormatUnit : public LogFormatter::FormatUnit
{
  public:
    ThreadNameFormatUnit(const std::string& str = "") {}

    void format(std::ostream& os, Logger::ptr logger, LogLevel level, LogRecord::ptr record) override { os << record->getThreadName(); }
};

class DateTimeFormatUnit : public LogFormatter::FormatUnit
{
  public:
    DateTimeFormatUnit(const std::string& format = "%Y-%m-%d %H:%M:%S") : format_(format) {}

    void format(std::ostream& os, Logger::ptr logger, LogLevel level, LogRecord::ptr record) override
    {
        static thread_local char   timeBuf[32];
        static thread_local time_t lastSecond;

        time_t seconds = record->getTimestamp().seconds();

        char buf[32];
        snprintf(buf, sizeof(buf), ".%06ld", record->getTimestamp().microSecondsRemainder());

        if (lastSecond == seconds)
        {
            os << timeBuf << buf;
            return;
        }

        struct tm tm;
        localtime_r(&seconds, &tm);
        strftime(timeBuf, sizeof(timeBuf), format_.c_str(), &tm);
        os << timeBuf << buf;

        lastSecond = seconds;
    }

  private:
    std::string format_;
};

class FilenameFormatUnit : public LogFormatter::FormatUnit
{
  public:
    FilenameFormatUnit(const std::string& str = "") {}

    void format(std::ostream& os, Logger::ptr logger, LogLevel level, LogRecord::ptr record) override { os << record->getFileName(); }
};

class LineFormatUnit : public LogFormatter::FormatUnit
{
  public:
    LineFormatUnit(const std::string& str = "") {}

    void format(std::ostream& os, Logger::ptr logger, LogLevel level, LogRecord::ptr record) override { os << record->getLine(); }
};

class FunctionFormatUnit : public LogFormatter::FormatUnit
{
  public:
    FunctionFormatUnit(const std::string& str = "") {}

    void format(std::ostream& os, Logger::ptr logger, LogLevel level, LogRecord::ptr record) override { os << record->getFunction(); }
};

class NewLineFormatUnit : public LogFormatter::FormatUnit
{
  public:
    NewLineFormatUnit(const std::string& str = "") {}

    void format(std::ostream& os, Logger::ptr logger, LogLevel level, LogRecord::ptr record) override { os << std::endl; }
};

class StringFormatUnit : public LogFormatter::FormatUnit
{
  public:
    StringFormatUnit(const std::string& str) : str_(str) {}

    void format(std::ostream& os, Logger::ptr logger, LogLevel level, LogRecord::ptr record) override { os << str_; }

  private:
    std::string str_;
};

class TabFormatUnit : public LogFormatter::FormatUnit
{
  public:
    TabFormatUnit(const std::string& str = "") {}

    void format(std::ostream& os, Logger::ptr logger, LogLevel level, LogRecord::ptr record) override { os << "\t"; }

  private:
    std::string str_;
};

class SpaceFormatUnit : public LogFormatter::FormatUnit
{
  public:
    SpaceFormatUnit(const std::string& str = "") {}

    void format(std::ostream& os, Logger::ptr logger, LogLevel level, LogRecord::ptr record) override { os << " "; }
};

}  // namespace easy

#endif
