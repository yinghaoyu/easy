#include "easy/base/Env.h"
#include "easy/base/Config.h"
#include "easy/base/Logger.h"

#include <string.h>  // strerror
#include <unistd.h>
#include <iomanip>
#include <iostream>

namespace easy
{
static Logger::ptr logger = ELOG_NAME("system");

bool Env::init(int argc, char** argv)
{
    char link[1024] = {0};
    char path[1024] = {0};
    sprintf(link, "/proc/%d/exe", getpid());
    if (readlink(link, path, sizeof(path)) < 0)
    {
        ELOG_ERROR(logger) << "readlink fail errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    // /path/xxx/exe
    exe_ = path;

    auto pos = exe_.find_last_of("/");
    cwd_     = exe_.substr(0, pos) + "/";

    program_ = argv[0];
    // -config /path/to/config -file xxxx -d
    const char* now_key = nullptr;
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i][0] == '-')
        {
            if (strlen(argv[i]) > 1)
            {
                if (now_key)
                {
                    add(now_key, "");
                }
                now_key = argv[i] + 1;
            }
            else
            {
                ELOG_ERROR(logger) << "invalid arg idx=" << i << " val=" << argv[i];
                return false;
            }
        }
        else
        {
            if (now_key)
            {
                add(now_key, argv[i]);
                now_key = nullptr;
            }
            else
            {
                ELOG_ERROR(logger) << "invalid arg idx=" << i << " val=" << argv[i];
                return false;
            }
        }
    }
    if (now_key)
    {
        add(now_key, "");
    }
    return true;
}

void Env::add(const std::string& key, const std::string& val)
{
    WriteLockGuard _(lock_);
    args_[key] = val;
}

bool Env::has(const std::string& key)
{
    ReadLockGuard _(lock_);
    auto          it = args_.find(key);
    return it != args_.end();
}

void Env::remove(const std::string& key)
{
    WriteLockGuard _(lock_);
    args_.erase(key);
}

std::string Env::get(const std::string& key, const std::string& default_value)
{
    ReadLockGuard _(lock_);
    auto          it = args_.find(key);
    return it != args_.end() ? it->second : default_value;
}

void Env::addHelp(const std::string& key, const std::string& desc)
{
    removeHelp(key);
    WriteLockGuard _(lock_);
    helps_.push_back(std::make_pair(key, desc));
}

void Env::removeHelp(const std::string& key)
{
    WriteLockGuard _(lock_);
    for (auto it = helps_.begin(); it != helps_.end();)
    {
        if (it->first == key)
        {
            it = helps_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void Env::printHelp()
{
    ReadLockGuard _(lock_);
    std::cout << "Usage: " << program_ << " [options]" << std::endl;
    for (auto& i : helps_)
    {
        std::cout << std::setw(5) << "-" << i.first << " : " << i.second << std::endl;
    }
}

bool Env::setEnv(const std::string& key, const std::string& val) { return !setenv(key.c_str(), val.c_str(), 1); }

std::string Env::env(const std::string& key, const std::string& default_value)
{
    const char* v = getenv(key.c_str());
    if (v == nullptr)
    {
        return default_value;
    }
    return v;
}

std::string Env::getAbsolutePath(const std::string& path) const
{
    if (path.empty())
    {
        return "/";
    }
    if (path[0] == '/')
    {
        return path;
    }
    return cwd_ + path;
}

std::string Env::getAbsoluteWorkPath(const std::string& path) const
{
    if (path.empty())
    {
        return "/";
    }
    if (path[0] == '/')
    {
        return path;
    }
    static ConfigVar<std::string>::ptr server_work_path = Config::Lookup<std::string>("server.work_path");
    return server_work_path->value() + "/" + path;
}

std::string Env::getConfigPath() { return getAbsolutePath(get("c", "conf")); }
}  // namespace easy
