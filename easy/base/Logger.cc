#include "easy/base/Logger.h"
#include "easy/base/Config.h"
#include "easy/base/Env.h"
#include "easy/base/LogAppender.h"
#include "easy/base/LogFormatter.h"
#include "easy/base/Mutex.h"

#include <iostream>

namespace easy
{

Logger::Logger(const std::string& name) : name_(name), level_(LogLevel::TRACE)
{
    formatter_ = std::make_shared<LogFormatter>("[%d{%Y-%m-%d %H:%M:%S}]%b%t%b%N%b%C%b[%p]%b[%c]%b%f%b%F:%L%b%m%n");
}

void Logger::setFormatter(LogFormatter::ptr val)
{
    WriteLockGuard _(lock_);
    formatter_ = val;
    for (auto& i : appenders_)
    {
        SpinLockGuard __(i->lock_);
        if (!i->hasFormatter_)
        {
            i->formatter_ = formatter_;
        }
    }
}

void Logger::setFormatter(const std::string& val)
{
    easy::LogFormatter::ptr new_val = std::make_shared<easy::LogFormatter>(val);
    if (new_val->error())
    {
        std::cout << "logger set_formatter name=" << name_ << " value=" << val << " invalid formatter" << std::endl;
        return;
    }
    setFormatter(new_val);
}

std::string Logger::toYamlString()
{
    ReadLockGuard _(lock_);
    YAML::Node    node;
    node["name"] = name_;
    if (level_ != LogLevel::OFF)
    {
        node["level"] = toString(level_);
    }
    if (formatter_)
    {
        node["formatter"] = formatter_->getPattern();
    }
    for (auto& i : appenders_)
    {
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

bool Logger::reopen()
{
    ReadLockGuard lock(lock_);
    auto          appenders = appenders_;
    lock.unlock();

    for (auto& i : appenders)
    {
        i->reopen();
    }
    return true;
}

LogFormatter::ptr Logger::getFormatter()
{
    ReadLockGuard _(lock_);
    return formatter_;
}

void Logger::addAppender(LogAppender::ptr appender)
{
    WriteLockGuard _(lock_);
    if (!appender->getFormatter())
    {
        SpinLockGuard __(appender->lock_);
        appender->formatter_ = formatter_;
    }
    appenders_.push_back(appender);
}

void Logger::deleteAppender(LogAppender::ptr appender)
{
    WriteLockGuard _(lock_);
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
    WriteLockGuard _(lock_);
    appenders_.clear();
}

void Logger::log(LogLevel level, LogRecord::ptr record)
{
    auto          self = shared_from_this();
    ReadLockGuard _(lock_);
    if (!appenders_.empty())
    {
        for (auto& i : appenders_)
        {
            i->log(self, level, record);
        }
    }
    else if (root_)
    {
        // not have appender, use root
        root_->log(level, record);
    }
}

void Logger::trace(LogRecord::ptr record) { log(LogLevel::TRACE, record); }

void Logger::debug(LogRecord::ptr record) { log(LogLevel::DEBUG, record); }

void Logger::info(LogRecord::ptr record) { log(LogLevel::INFO, record); }

void Logger::warn(LogRecord::ptr record) { log(LogLevel::WARN, record); }

void Logger::error(LogRecord::ptr record) { log(LogLevel::ERROR, record); }

void Logger::fatal(LogRecord::ptr record) { log(LogLevel::FATAL, record); }

LoggerManager::LoggerManager()
{
    // root as default
    root_ = std::make_shared<Logger>();
    root_->addAppender(std::make_shared<ConsoleLogAppender>());
    loggers_[root_->name_] = root_;
}

Logger::ptr LoggerManager::getLogger(const std::string& name)
{
    {
        ReadLockGuard _(lock_);

        auto it = loggers_.find(name);
        if (it != loggers_.end())
        {
            return it->second;
        }
    }

    WriteLockGuard _(lock_);

    auto it = loggers_.find(name);
    if (it != loggers_.end())
    {
        return it->second;
    }
    // not find
    Logger::ptr obj = std::make_shared<Logger>(name);
    obj->root_      = root_;
    loggers_[name]  = obj;
    return obj;
}

std::string LoggerManager::toYamlString()
{
    ReadLockGuard _(lock_);
    YAML::Node    node;
    for (auto& i : loggers_)
    {
        node.push_back(YAML::Load(i.second->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

struct LogAppenderDefine
{
    int         type  = 0;  // 1 File, 2 Console
    LogLevel    level = LogLevel::OFF;
    std::string formatter;
    std::string file;

    bool operator==(const LogAppenderDefine& oth) const
    {
        return type == oth.type && level == oth.level && formatter == oth.formatter && file == oth.file;
    }
};

struct LogDefine
{
    std::string                    name;
    LogLevel                       level = LogLevel::OFF;
    std::string                    formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine& oth) const
    {
        return name == oth.name && level == oth.level && formatter == oth.formatter && appenders == appenders;
    }

    bool operator<(const LogDefine& oth) const { return name < oth.name; }

    bool isValid() const { return !name.empty(); }
};

template <>
class LexicalCast<std::string, LogDefine>
{
  public:
    LogDefine operator()(const std::string& v)
    {
        YAML::Node n = YAML::Load(v);
        LogDefine  ld;
        if (!n["name"].IsDefined())
        {
            std::cout << "log config error: name is null, " << n << std::endl;
            throw std::logic_error("log config name is null");
        }
        ld.name  = n["name"].as<std::string>();
        ld.level = fromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");
        if (n["formatter"].IsDefined())
        {
            ld.formatter = n["formatter"].as<std::string>();
        }

        if (n["appenders"].IsDefined())
        {
            for (size_t x = 0; x < n["appenders"].size(); ++x)
            {
                auto a = n["appenders"][x];
                if (!a["type"].IsDefined())
                {
                    std::cout << "log config error: appender type is null, " << a << std::endl;
                    continue;
                }
                std::string       type = a["type"].as<std::string>();
                LogAppenderDefine lad;
                if (type == "FileLogAppender")
                {
                    lad.type = 1;
                    if (!a["file"].IsDefined())
                    {
                        std::cout << "log config error: fileappender file is null, " << a << std::endl;
                        continue;
                    }
                    lad.file = a["file"].as<std::string>();
                    if (a["formatter"].IsDefined())
                    {
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                }
                else if (type == "ConsoleLogAppender")
                {
                    lad.type = 2;
                    if (a["formatter"].IsDefined())
                    {
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                }
                else
                {
                    std::cout << "log config error: appender type is invalid, " << a << std::endl;
                    continue;
                }

                ld.appenders.push_back(lad);
            }
        }
        return ld;
    }
};

template <>
class LexicalCast<LogDefine, std::string>
{
  public:
    std::string operator()(const LogDefine& i)
    {
        YAML::Node n;
        n["name"] = i.name;
        if (i.level != LogLevel::OFF)
        {
            n["level"] = toString(i.level);
        }
        if (!i.formatter.empty())
        {
            n["formatter"] = i.formatter;
        }

        for (auto& a : i.appenders)
        {
            YAML::Node na;
            if (a.type == 1)
            {
                na["type"] = "FileLogAppender";
                na["file"] = a.file;
            }
            else if (a.type == 2)
            {
                na["type"] = "ConsoleLogAppender";
            }
            if (a.level != LogLevel::OFF)
            {
                na["level"] = toString(a.level);
            }

            if (!a.formatter.empty())
            {
                na["formatter"] = a.formatter;
            }

            n["appenders"].push_back(na);
        }
        std::stringstream ss;
        ss << n;
        return ss.str();
    }
};

static easy::ConfigVar<std::set<LogDefine>>::ptr log_defines = easy::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

struct LogIniter
{
    LogIniter()
    {
        log_defines->addListener([](const std::set<LogDefine>& old_value, const std::set<LogDefine>& new_value) {
            ELOG_INFO(ELOG_ROOT()) << "on_logger_conf_changed";
            for (auto& i : new_value)
            {
                auto        it = old_value.find(i);
                Logger::ptr logger;
                if (it == old_value.end())
                {
                    // 新增logger
                    logger = ELOG_NAME(i.name);
                }
                else
                {
                    if (!(i == *it))
                    {
                        // 修改的logger
                        logger = ELOG_NAME(i.name);
                    }
                    else
                    {
                        continue;
                    }
                }
                logger->setLevel(i.level);
                if (!i.formatter.empty())
                {
                    logger->setFormatter(i.formatter);
                }

                logger->clearAppender();
                for (auto& a : i.appenders)
                {
                    LogAppender::ptr ap;
                    if (a.type == 1)
                    {
                        ap = std::make_shared<FileLogAppender>(a.file);
                    }
                    else if (a.type == 2)
                    {
                        if (!EnvMgr::GetInstance()->has("d"))
                        {
                            ap = std::make_shared<ConsoleLogAppender>();
                        }
                        else
                        {
                            continue;
                        }
                    }
                    ap->setLevel(a.level);
                    if (!a.formatter.empty())
                    {
                        LogFormatter::ptr fmt = std::make_shared<LogFormatter>(a.formatter);
                        if (!fmt->error())
                        {
                            ap->setFormatter(fmt);
                        }
                        else
                        {
                            std::cout << "log.name=" << i.name << " appender type=" << a.type << " formatter=" << a.formatter << " is invalid"
                                      << std::endl;
                        }
                    }
                    logger->addAppender(ap);
                }
            }

            for (auto& i : old_value)
            {
                auto it = new_value.find(i);
                if (it == new_value.end())
                {
                    // 删除logger
                    auto logger = ELOG_NAME(i.name);
                    logger->setLevel(LogLevel::OFF);
                    logger->clearAppender();
                }
            }
        });
    }
};
}  // namespace easy
