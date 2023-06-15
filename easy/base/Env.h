#ifndef __EASY__ENV_H__
#define __EASY__ENV_H__

#include "easy/base/Mutex.h"
#include "easy/base/Singleton.h"

#include <map>
#include <vector>

namespace easy
{
class Env
{
 public:
  bool init(int argc, char **argv);

  void add(const std::string &key, const std::string &val);
  bool has(const std::string &key);
  void remove(const std::string &key);
  std::string get(const std::string &key,
                  const std::string &default_value = "");

  void addHelp(const std::string &key, const std::string &desc);
  void removeHelp(const std::string &key);
  void printHelp();

  const std::string &exe() { return exe_; }
  const std::string &cwd() { return cwd_; }

  bool setEnv(const std::string &key, const std::string &val);
  std::string env(const std::string &key,
                  const std::string &default_value = "");

  std::string getAbsolutePath(const std::string &path) const;
  std::string getAbsoluteWorkPath(const std::string &path) const;

  std::string getConfigPath();

 private:
  RWLock lock_;
  std::map<std::string, std::string> args_;
  std::vector<std::pair<std::string, std::string>> helps_;

  std::string program_;
  std::string exe_;
  std::string cwd_;
};

typedef Singleton<Env> EnvMgr;

}  // namespace easy

#endif
