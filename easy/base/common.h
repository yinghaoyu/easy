#ifndef __EASY_COMMON_H__
#define __EASY_COMMON_H__

#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace easy
{
namespace util
{
static const int kMicroSecondsPerSecond = 1000 * 1000;

inline int64_t timestamp()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int64_t seconds = tv.tv_sec;
  return seconds * kMicroSecondsPerSecond + tv.tv_usec;
}

inline int __lstat(const char *file, struct stat *st = nullptr)
{
  struct stat lst;
  int ret = lstat(file, &lst);
  if (st)
  {
    *st = lst;
  }
  return ret;
}

inline int __mkdir(const char *dirname)
{
  if (access(dirname, F_OK) == 0)
  {
    return 0;
  }
  return mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

std::string dirname(const std::string &filename);

bool mkdir_recursive(const std::string &dirname);

bool open_for_write(std::ofstream &ofs,
                    const std::string &filename,
                    std::ios_base::openmode mode);

void backtrace(std::vector<std::string> &bt, size_t size = 64, int skip = 1);

std::string backtrace_to_string(int size = 64,
                                int skip = 2,
                                const std::string &prefix = "");

}  // namespace util

}  // namespace easy

#endif
