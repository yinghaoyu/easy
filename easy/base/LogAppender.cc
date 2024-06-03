#include "easy/base/LogAppender.h"
#include "easy/base/FileUtil.h"
#include "easy/base/LogFormatter.h"
#include "easy/base/Logger.h"
#include "easy/base/Mutex.h"

#include <yaml-cpp/yaml.h>
#include <iostream>

namespace easy
{
void LogAppender::setFormatter(std::shared_ptr<LogFormatter> val)
{
    SpinLockGuard _(lock_);
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
    SpinLockGuard _(lock_);
    return formatter_;
}

bool LogAppender::shouldLog(LogLevel level) const { return level >= level_; }

void ConsoleLogAppender::log(std::shared_ptr<Logger> logger, LogLevel level, LogRecord::ptr record)
{
    if (shouldLog(level))
    {
        SpinLockGuard _(lock_);

        // add color
        if (level == LogLevel::WARN)
            std::cout << "\x1B[93m";
        if (level == LogLevel::ERROR)
            std::cout << "\x1B[91m";
        if (level == LogLevel::FATAL)
            std::cout << "\x1B[97m\x1B[41m";

        formatter_->format(std::cout, logger, level, record);

        // clean color
        if (level >= LogLevel::WARN)
            std::cout << "\x1B[0m\x1B[0K";
    }
}

std::string ConsoleLogAppender::toYamlString()
{
    SpinLockGuard _(lock_);
    YAML::Node    node;
    node["type"] = "ConsoleLogAppender";
    if (level_ != LogLevel::OFF)
    {
        node["level"] = toString(level_);
    }
    if (hasFormatter_ && formatter_)
    {
        node["formatter"] = formatter_->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

FileLogAppender::FileLogAppender(const std::string& filename) : filename_(filename) { reopen(); }

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel level, LogRecord::ptr record)
{
    if (shouldLog(level))
    {
        Timestamp now = record->getTimestamp();
        if (timeDifference(now, lastTime_) >= kRoutineIntervalMs)
        {
            // 把文件 mov 后，进程已经打开的文件 inode 不会改变
            // 需要重新打开才会创建新的文件，inode 才会替换
            reopen();
            lastTime_ = now;
        }
        SpinLockGuard _(lock_);
        if (!formatter_->format(filestream_, logger, level, record))
        {
            std::cout << "error" << std::endl;
        }
    }
}

std::string FileLogAppender::toYamlString()
{
    SpinLockGuard _(lock_);
    YAML::Node    node;
    node["type"] = "FileLogAppender";
    node["file"] = filename_;
    if (level_ != LogLevel::OFF)
    {
        node["level"] = toString(level_);
    }
    if (hasFormatter_ && formatter_)
    {
        node["formatter"] = formatter_->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

bool FileLogAppender::reopen()
{
    SpinLockGuard _(lock_);
    if (filestream_)
    {
        filestream_.close();
    }
    return FileUtil::OpenForWrite(filestream_, filename_, std::ios::app);
}

}  // namespace easy
