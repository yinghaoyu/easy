#ifndef __EASY_LOG_FORMATTER_H__
#define __EASY_LOG_FORMATTER_H__

#include "easy/base/LogEvent.h"
#include "easy/base/Logger.h"

#include <inttypes.h>
#include <memory.h>
#include <ctime>
#include <memory>
#include <vector>

namespace easy {
// 日志格式器
class LogFormatter {
 public:
  typedef std::shared_ptr<LogFormatter> ptr;

  explicit LogFormatter(const std::string& pattern);
  ~LogFormatter() = default;

  std::string format(Logger::ptr logger, LogLevel::Level level,
                     LogEvent::ptr event);

  std::ostream& format(std::ostream& ofs, Logger::ptr logger,
                       LogLevel::Level level, LogEvent::ptr event);

 public:
  // 抽象类
  class FormatItem {
   public:
    typedef std::shared_ptr<FormatItem> ptr;
    virtual ~FormatItem() = default;
    virtual void format(std::ostream& os, Logger::ptr logger,
                        LogLevel::Level level, LogEvent::ptr event) = 0;
  };

  bool error() const { return error_; }

  const std::string pattern() const { return pattern_; }

 private:
  void compile_pattern(const std::string& pattern);

 private:
  std::string pattern_;
  std::vector<FormatItem::ptr> items_;
  bool error_ = false;
};

class MessageFormatItem : public LogFormatter::FormatItem {
 public:
  MessageFormatItem(const std::string& str = "") {}

  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->content();
  }
};

class LevelFormatItem : public LogFormatter::FormatItem {
 public:
  LevelFormatItem(const std::string& str = "") {}

  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << LogLevel::toString(level);
  }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
 public:
  ElapseFormatItem(const std::string& str = "") {}

  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->elapse();
  }
};

class NameFormatItem : public LogFormatter::FormatItem {
 public:
  NameFormatItem(const std::string& str = "") {}

  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->logger()->name();
  }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
 public:
  ThreadIdFormatItem(const std::string& str = "") {}

  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->threadId();
  }
};

class CoroutineIdFormatItem : public LogFormatter::FormatItem {
 public:
  CoroutineIdFormatItem(const std::string& str = "") {}

  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->coroutineId();
  }
};

class ThreadNameFormatItem : public LogFormatter::FormatItem {
 public:
  ThreadNameFormatItem(const std::string& str = "") {}

  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->threadName();
  }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
 public:
  DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S")
      : format_(format) {}

  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->timestamp().toString(format_);
  }

 private:
  std::string format_;
};

class FilenameFormatItem : public LogFormatter::FormatItem {
 public:
  FilenameFormatItem(const std::string& str = "") {}

  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->fileName();
  }
};

class LineFormatItem : public LogFormatter::FormatItem {
 public:
  LineFormatItem(const std::string& str = "") {}

  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->line();
  }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
 public:
  NewLineFormatItem(const std::string& str = "") {}

  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << std::endl;
  }
};

class StringFormatItem : public LogFormatter::FormatItem {
 public:
  StringFormatItem(const std::string& str) : str_(str) {}

  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << str_;
  }

 private:
  std::string str_;
};

class TabFormatItem : public LogFormatter::FormatItem {
 public:
  TabFormatItem(const std::string& str = "") {}

  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << "\t";
  }

 private:
  std::string str_;
};

class SpaceFormatItem : public LogFormatter::FormatItem {
 public:
  SpaceFormatItem(const std::string& str = "") {}

  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << " ";
  }
};

}  // namespace easy

#endif
