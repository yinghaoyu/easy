#include "Logger.h"
#include "LogAppender.h"
#include "LogFormatter.h"

#include <iostream>

namespace easy
{

Logger::Logger(const std::string &name) : name_(name), level_(LogLevel::trace)
{
  formatter_ = std::make_shared<LogFormatter>(
      "[%d{%Y-%m-%d %H:%M:%S.%06ld}]%b%t%b%N%b%C%b[%p]%b[%c]%b%F:%L%b%m%n");
}

void Logger::setFormatter(LogFormatter::ptr val)
{
  // RWMutexType::WriteLock lock(m_mutex);
  formatter_ = val;
  for (auto &i : appenders_)
  {
    // LogAppender::MutexType::Lock ll(i->m_mutex);
    if (!i->hasFormatter_)
    {
      i->formatter_ = formatter_;
    }
  }
}

void Logger::setFormatter(const std::string &val)
{
  easy::LogFormatter::ptr new_val = std::make_shared<easy::LogFormatter>(val);
  if (new_val->error())
  {
    std::cout << "logger set_formatter name=" << name_ << " value=" << val
              << " invalid formatter" << std::endl;
    return;
  }
  // formatter_ = new_val;
  setFormatter(new_val);
}

bool Logger::reopen()
{
  // RWMutexType::ReadLock lock(m_mutex);
  auto appenders = appenders_;
  // lock.unlock();

  for (auto &i : appenders)
  {
    i->reopen();
  }
  return true;
}

LogFormatter::ptr Logger::formatter()
{
  // RWMutexType::ReadLock lock(m_mutex);
  return formatter_;
}

void Logger::addAppender(LogAppender::ptr appender)
{
  // RWMutexType::WriteLock lock(m_mutex);
  if (!appender->getFormatter())
  {
    // LogAppender::MutexType::Lock ll(appender->m_mutex);
    appender->formatter_ = formatter_;
  }
  appenders_.push_back(appender);
}

void Logger::deleteAppender(LogAppender::ptr appender)
{
  // RWMutexType::WriteLock lock(m_mutex);
  for (auto it = appenders_.begin(); it != appenders_.end(); ++it)
  {
    if (*it == appender)
    {
      appenders_.erase(it);
      break;
    }
  }
}

void Logger::clearAppender()
{
  // RWMutexType::WriteLock lock(m_mutex);
  appenders_.clear();
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event)
{
  auto self = shared_from_this();
  // RWMutexType::ReadLock lock(m_mutex);
  if (!appenders_.empty())
  {
    for (auto &i : appenders_)
    {
      i->log(self, level, event);
    }
  }
  else if (root_)
  {
    // not have appender, use root
    root_->log(level, event);
  }
}

void Logger::trace(LogEvent::ptr event)
{
  log(LogLevel::trace, event);
}

void Logger::debug(LogEvent::ptr event)
{
  log(LogLevel::debug, event);
}

void Logger::info(LogEvent::ptr event)
{
  log(LogLevel::info, event);
}

void Logger::warn(LogEvent::ptr event)
{
  log(LogLevel::warn, event);
}

void Logger::error(LogEvent::ptr event)
{
  log(LogLevel::error, event);
}

void Logger::fatal(LogEvent::ptr event)
{
  log(LogLevel::fatal, event);
}

LoggerManager::LoggerManager()
{
  // root as default
  root_ = std::make_shared<Logger>();
  root_->addAppender(std::make_shared<ConsoleLogAppender>());
  loggers_[root_->name_] = root_;
}

Logger::ptr LoggerManager::getLogger(const std::string &name)
{
  do
  {
    // RWMutexType::ReadLock lock(m_mutex);
    auto it = loggers_.find(name);
    if (it != loggers_.end())
    {
      return it->second;
    }
  } while (0);

  // RWMutexType::WriteLock lock(m_mutex);
  auto it = loggers_.find(name);
  if (it != loggers_.end())
  {
    return it->second;
  }
  // not find
  Logger::ptr obj = std::make_shared<Logger>(name);
  obj->root_ = root_;
  loggers_[name] = obj;
  return obj;
}
}  // namespace easy
