#include "easy/base/Timestamp.h"

#include <memory.h>
#include <sys/time.h>
#include <string>

namespace easy
{
Timestamp Timestamp::now()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int64_t seconds = tv.tv_sec;
  return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}

std::string Timestamp::toString(const std::string &format) const
{
  std::string date_time_format;
  std::string microsecond_format;

  std::string ans;

  auto dot = format.find_first_of('.');

  if (dot == std::string::npos)
  {
    date_time_format = format;
  }
  else
  {
    date_time_format = format.substr(0, dot);
    microsecond_format = format.substr(dot);
  }

  struct tm tm_time;
  time_t seconds =
      static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
  localtime_r(&seconds, &tm_time);
  char buf[64] = {0};

  strftime(buf, sizeof(buf), date_time_format.c_str(), &tm_time);
  ans.append(buf);

  if (!microsecond_format.empty())
  {
    memset(buf, 0, sizeof(buf));
    int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
    snprintf(buf, sizeof(buf), microsecond_format.c_str(), microseconds);
    ans.append(buf);
  }
  return ans;
}
}  // namespace easy
