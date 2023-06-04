#include "common.h"

#include <cxxabi.h>
#include <execinfo.h>
#include <sstream>
#include <string>

namespace easy
{
namespace util
{
std::string dirname(const std::string &filename)
{
  if (filename.empty())
  {
    return ".";
  }
  auto pos = filename.rfind('/');
  if (pos == 0)
  {
    return "/";
  }
  else if (pos == std::string::npos)
  {
    return ".";
  }
  else
  {
    return filename.substr(0, pos);
  }
}

bool mkdir_recursive(const std::string &dirname)
{
  if (__lstat(dirname.c_str()) == 0)
  {
    return true;
  }
  char *path = strdup(dirname.c_str());
  char *ptr = strchr(path + 1, '/');
  do
  {
    for (; ptr; *ptr = '/', ptr = strchr(ptr + 1, '/'))
    {
      *ptr = '\0';
      if (__mkdir(path) != 0)
      {
        break;
      }
    }
    if (ptr != nullptr)
    {
      break;
    }
    else if (__mkdir(path) != 0)
    {
      break;
    }
    free(path);
    return true;
  } while (0);
  free(path);
  return false;
}

bool open_for_write(std::ofstream &ofs,
                    const std::string &filename,
                    std::ios_base::openmode mode)
{
  ofs.open(filename.c_str(), mode);
  if (!ofs.is_open())
  {
    std::string dir = dirname(filename);
    mkdir_recursive(dir);
    ofs.open(filename.c_str(), mode);
  }
  return ofs.is_open();
}

static std::string demangle(const char *str)
{
  // https://panthema.net/2008/0901-stacktrace-demangled/
  size_t size = 0;
  int status = 0;
  std::string ret;
  ret.resize(256);
  if (1 == sscanf(str, "%*[^(]%*[^_]%255[^)+]", &ret[0]))
  {
    char *v = abi::__cxa_demangle(&ret[0], nullptr, &size, &status);
    if (v)
    {
      std::string result(v);
      free(v);
      return result;
    }
  }
  if (1 == sscanf(str, "%255s", &ret[0]))
  {
    return ret;
  }
  return str;
}

void backtrace(std::vector<std::string> &bt, size_t size, int skip)
{
  // heap is the best choice for coroutine
  void **array = static_cast<void **>(malloc((sizeof(void *) * size)));
  int s = ::backtrace(array, static_cast<int>(size));

  char **strings = backtrace_symbols(array, s);
  if (strings == NULL)
  {
    return;
  }

  // usually skip > 0
  // skipping the 0-th, which is this function
  for (int i = skip; i < s; ++i)
  {
    bt.push_back(demangle(strings[i]));
  }

  free(strings);
  free(array);
}

std::string backtrace_to_string(int size, int skip, const std::string &prefix)
{
  std::vector<std::string> bt;
  backtrace(bt, size, skip);
  std::stringstream ss;
  for (size_t i = 0; i < bt.size(); ++i)
  {
    ss << prefix << bt[i] << std::endl;
  }
  return ss.str();
}

}  // namespace util

}  // namespace easy
