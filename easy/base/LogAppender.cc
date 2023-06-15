#include "easy/base/LogAppender.h"
#include "easy/base/LogFormatter.h"
#include "easy/base/Logger.h"
#include "easy/base/FileUtil.h"
#include "easy/base/Mutex.h"

#include <iostream>
#include <yaml-cpp/yaml.h>

namespace easy
{
void LogAppender::setFormatter(std::shared_ptr<LogFormatter> val)
{
  SpinLockGuard lock(lock_);
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
  SpinLockGuard lock(lock_);
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
    SpinLockGuard lock(lock_);
    formatter_->format(std::cout, logger, level, event);
  }
}

std::string ConsoleLogAppender::toYamlString()
{
  SpinLockGuard lock(lock_);
  YAML::Node node;
  node["type"] = "ConsoleLogAppender";
  if (level_ != LogLevel::off)
  {
    node["level"] = LogLevel::toString(level_);
  }
  if (hasFormatter_ && formatter_)
  {
    node["formatter"] = formatter_->pattern();
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
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
    Timestamp now = event->timestamp();
    if (timeDifference(now, lastTime_) >= kRoutineIntervalMs)
    {
      // 把文件 mov 后，进程已经打开的文件 inode 不会改变
      // 需要重新打开才会创建新的文件，inode 才会替换
      reopen();
      lastTime_ = now;
    }
    SpinLockGuard lock(lock_);
    if (!formatter_->format(filestream_, logger, level, event))
    {
      std::cout << "error" << std::endl;
    }
  }
}

std::string FileLogAppender::toYamlString()
{
  SpinLockGuard lock(lock_);
  YAML::Node node;
  node["type"] = "FileLogAppender";
  node["file"] = filename_;
  if (level_ != LogLevel::off)
  {
    node["level"] = LogLevel::toString(level_);
  }
  if (hasFormatter_ && formatter_)
  {
    node["formatter"] = formatter_->pattern();
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

bool FileLogAppender::reopen()
{
  SpinLockGuard lock(lock_);
  if (filestream_)
  {
    filestream_.close();
  }
  return FileUtil::OpenForWrite(filestream_, filename_, std::ios::app);
}

}  // namespace easy
