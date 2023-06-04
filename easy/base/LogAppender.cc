#include "LogAppender.h"
#include "LogFormatter.h"
#include "Logger.h"

#include <iostream>

namespace easy
{
void LogAppender::setFormatter(std::shared_ptr<LogFormatter> val)
{
  // MutexType::Lock lock(m_mutex);
  formatter_ = val;
  if (formatter_)
  {
    hasFormatter_ = true;
  }
  else
  {
    hasFormatter_ = false;
  }
}

std::shared_ptr<LogFormatter> LogAppender::getFormatter()
{
  // MutexType::Lock lock(m_mutex);
  return formatter_;
}

bool LogAppender::shouldLog(LogLevel::Level level) const
{
  return level >= level_;
}

void ConsoleLogAppender::log(std::shared_ptr<Logger> logger,
                             LogLevel::Level level,
                             LogEvent::ptr event)
{
  if (shouldLog(level))
  {
    // MutexType::Lock lock(m_mutex);
    formatter_->format(std::cout, logger, level, event);
  }
}

FileLogAppender::FileLogAppender(const std::string &filename)
    : filename_(filename)
{
  reopen();
}

void FileLogAppender::log(std::shared_ptr<Logger> logger,
                          LogLevel::Level level,
                          LogEvent::ptr event)
{
  if (shouldLog(level))
  {
    int64_t now = event->timestamp();
    if (now >= (last_time_ + kRoutineInterval))
    {
      // 把文件 mov 后，进程已经打开的文件 inode 不会改变
      // 需要重新打开才会创建新的文件，inode 才会替换
      reopen();
      last_time_ = now;
    }
    // MutexType::Lock lock(m_mutex);
    //  if(!(m_filestream << formatter_->format(logger, level, event))) {
    if (!formatter_->format(filestream_, logger, level, event))
    {
      std::cout << "error" << std::endl;
    }
  }
}

bool FileLogAppender::reopen()
{
  // MutexType::Lock lock(m_mutex);
  if (filestream_)
  {
    filestream_.close();
  }
  return util::open_for_write(filestream_, filename_, std::ios::app);
}

}  // namespace easy
