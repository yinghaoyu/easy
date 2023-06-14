#ifndef __EASY_CONFIG_H__
#define __EASY_CONFIG_H__

#include "easy/base/Logger.h"
#include "easy/base/Mutex.h"
#include "easy/base/common.h"

#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace easy
{

class ConfigVarBase
{
 public:
  typedef std::shared_ptr<ConfigVarBase> ptr;

  ConfigVarBase(const std::string &name, const std::string &description = "")
      : name_(name), description_(description)
  {
    std::transform(name_.begin(), name_.end(), name_.begin(), ::tolower);
  }

  virtual ~ConfigVarBase() {}

  const std::string &name() const { return name_; }

  const std::string &description() const { return description_; }

  virtual std::string toString() = 0;

  virtual bool fromString(const std::string &val) = 0;

  virtual std::string typeName() const = 0;

 protected:
  std::string name_;
  std::string description_;
};

template <class F, class T>
class LexicalCast
{
 public:
  T operator()(const F &v) { return boost::lexical_cast<T>(v); }
};

template <typename T>
class LexicalCast<std::string, std::vector<T>>
{
 public:
  std::vector<T> operator()(const std::string &v)
  {
    YAML::Node node = YAML::Load(v);
    typename std::vector<T> vec;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); ++i)
    {
      ss.str("");
      ss << node[i];
      vec.push_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};

template <typename T>
class LexicalCast<std::vector<T>, std::string>
{
 public:
  std::string operator()(const std::vector<T> &v)
  {
    YAML::Node node(YAML::NodeType::Sequence);
    for (auto &i : v)
    {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template <typename T>
class LexicalCast<std::string, std::list<T>>

{
 public:
  std::list<T> operator()(const std::string &v)
  {
    YAML::Node node = YAML::Load(v);
    typename std::list<T> vec;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); ++i)
    {
      ss.str("");
      ss << node[i];
      vec.push_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};

template <typename T>
class LexicalCast<std::list<T>, std::string>
{
 public:
  std::string operator()(const std::list<T> &v)
  {
    YAML::Node node(YAML::NodeType::Sequence);
    for (auto &i : v)
    {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template <class T>
class LexicalCast<std::string, std::set<T>>
{
 public:
  std::set<T> operator()(const std::string &v)
  {
    YAML::Node node = YAML::Load(v);
    typename std::set<T> vec;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); ++i)
    {
      ss.str("");
      ss << node[i];
      vec.insert(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};

template <class T>
class LexicalCast<std::set<T>, std::string>
{
 public:
  std::string operator()(const std::set<T> &v)
  {
    YAML::Node node(YAML::NodeType::Sequence);
    for (auto &i : v)
    {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template <class T>
class LexicalCast<std::string, std::unordered_set<T>>
{
 public:
  std::unordered_set<T> operator()(const std::string &v)
  {
    YAML::Node node = YAML::Load(v);
    typename std::unordered_set<T> vec;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); ++i)
    {
      ss.str("");
      ss << node[i];
      vec.insert(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};

template <class T>
class LexicalCast<std::unordered_set<T>, std::string>
{
 public:
  std::string operator()(const std::unordered_set<T> &v)
  {
    YAML::Node node(YAML::NodeType::Sequence);
    for (auto &i : v)
    {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template <class T>
class LexicalCast<std::string, std::map<std::string, T>>
{
 public:
  std::map<std::string, T> operator()(const std::string &v)
  {
    YAML::Node node = YAML::Load(v);
    typename std::map<std::string, T> vec;
    std::stringstream ss;
    for (auto it = node.begin(); it != node.end(); ++it)
    {
      ss.str("");
      ss << it->second;
      vec.insert(std::make_pair(it->first.Scalar(),
                                LexicalCast<std::string, T>()(ss.str())));
    }
    return vec;
  }
};

template <class T>
class LexicalCast<std::map<std::string, T>, std::string>
{
 public:
  std::string operator()(const std::map<std::string, T> &v)
  {
    YAML::Node node(YAML::NodeType::Map);
    for (auto &i : v)
    {
      node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template <class T>
class LexicalCast<std::string, std::unordered_map<std::string, T>>
{
 public:
  std::unordered_map<std::string, T> operator()(const std::string &v)
  {
    YAML::Node node = YAML::Load(v);
    typename std::unordered_map<std::string, T> vec;
    std::stringstream ss;
    for (auto it = node.begin(); it != node.end(); ++it)
    {
      ss.str("");
      ss << it->second;
      vec.insert(std::make_pair(it->first.Scalar(),
                                LexicalCast<std::string, T>()(ss.str())));
    }
    return vec;
  }
};

template <class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string>
{
 public:
  std::string operator()(const std::unordered_map<std::string, T> &v)
  {
    YAML::Node node(YAML::NodeType::Map);
    for (auto &i : v)
    {
      node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template <typename T,
          typename FromStr = LexicalCast<std::string, T>,
          typename ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase
{
 public:
  typedef std::shared_ptr<ConfigVar> ptr;
  typedef std::function<void(const T &old_value, const T &new_value)>
      on_change_cb;

  ConfigVar(const std::string &name,
            const T &default_value,
            const std::string &description = "")
      : ConfigVarBase(name, description), val_(default_value)
  {
  }

  std::string toString() override
  {
    try
    {
      ReadLockGuard lock(lock_);
      return ToStr()(val_);
    }
    catch (std::exception &e)
    {
      EASY_LOG_ERROR(EASY_LOG_ROOT())
          << "ConfigVar::toString exception " << e.what()
          << " convert: " << TypeToName<T>() << " to string"
          << " name=" << name_;
    }
    return "";
  }

  bool fromString(const std::string &val) override

  {
    try
    {
      setValue(FromStr()(val));
    }
    catch (std::exception &e)
    {
      EASY_LOG_ERROR(EASY_LOG_ROOT())
          << "ConfigVar::fromString exception" << e.what()
          << " convert: string to " << TypeToName<T>()
          << " name=" << name_ << " - " << val;
    }
    return false;
  }

  const T value()
  {
    ReadLockGuard lock(lock_);
    return val_;
  }

  void setValue(const T &v)
  {
    {
      ReadLockGuard lock(lock_);
      if (v == val_)
      {
        return;
      }
      for (auto &i : cbs_)
      {
        i.second(val_, v);
      }
    }
    WriteLockGuard lock(lock_);
    val_ = v;
  }

  std::string typeName() const override { return TypeToName<T>(); }

  uint64_t addListener(on_change_cb cb)
  {
    static uint64_t s_fun_id = 0;
    WriteLockGuard lock(lock_);
    ++s_fun_id;
    cbs_[s_fun_id] = cb;
    return s_fun_id;
  }

  void delListener(uint64_t key)
  {
    WriteLockGuard lock(lock_);
    cbs_.erase(key);
  }

  on_change_cb linster(uint64_t key)
  {
    ReadLockGuard lock(lock_);
    auto it = cbs_.find(key);
    return it == cbs_.end() ? nullptr : it->second;
  }

  void clearListener()
  {
    WriteLockGuard lock(lock_);
    cbs_.clear();
  }

 private:
  RWLock lock_;
  T val_;
  std::map<uint64_t, on_change_cb> cbs_;
};

class Config
{
 public:
  typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;

  template <typename T>
  static typename ConfigVar<T>::ptr Lookup(const std::string &name,
                                           const T &default_value,
                                           const std::string &description = "")
  {
    WriteLockGuard lock(GetLock());
    auto it = GetDatas().find(name);
    if (it != GetDatas().end())
    {
      auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
      if (tmp)
      {
        EASY_LOG_INFO(EASY_LOG_ROOT()) << "Lookup name=" << name << " exists";
      }
      else
      {
        EASY_LOG_ERROR(EASY_LOG_ROOT())
            << "Lookup name=" << name << " exists but type not "
            << TypeToName<T>() << " real_type=" << it->second->typeName()
            << " " << it->second->toString();
        return nullptr;
      }
    }

    if (name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._0123456789") !=
        std::string::npos)
    {
      EASY_LOG_ERROR(EASY_LOG_ROOT()) << "Lookup name invalid " << name;
      throw std::invalid_argument(name);
    }

    typename ConfigVar<T>::ptr v(
        new ConfigVar<T>(name, default_value, description));
    GetDatas()[name] = v;
    return v;
  }

  template <typename T>
  static typename ConfigVar<T>::ptr Lookup(const std::string &name)
  {
    ReadLockGuard lock(GetLock());
    auto it = GetDatas().find(name);
    if (it == GetDatas().end())
    {
      return nullptr;
    }
    return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
  }

  static void LoadFromYaml(const YAML::Node &root);

  static void LoadFromConfDir(const std::string &path, bool force = false);

  static ConfigVarBase::ptr LookupBase(const std::string &name);

  static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

 private:
  static ConfigVarMap &GetDatas()
  {
    static ConfigVarMap s_datas;
    return s_datas;
  }

  static RWLock &GetLock()
  {
    static RWLock s_lock;
    return s_lock;
  }
};

}  // namespace easy

#endif
