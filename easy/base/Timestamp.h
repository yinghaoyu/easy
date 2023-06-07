#ifndef __EASY_TIMESTAMP_H__
#define __EASY_TIMESTAMP_H__

#include "easy/base/copyable.h"

#include <cstdint>
#include <string>

namespace easy
{
class Timestamp : public copyable
{
 public:
  Timestamp() : microSecondsSinceEpoch_(0) {}

  explicit Timestamp(int64_t msc) : microSecondsSinceEpoch_(msc) {}

  Timestamp(const Timestamp &other)
      : microSecondsSinceEpoch_(other.microSecondsSinceEpoch_)
  {
  }

  ~Timestamp(){}

  bool valid() const { return microSecondsSinceEpoch_ > 0; }

  int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }

  std::string toString(const std::string &format = "%Y-%m-%d %H:%M:%S") const;

  void operator=(const Timestamp& other)
  {
    microSecondsSinceEpoch_ = other.microSecondsSinceEpoch_;
  }

  bool operator==(const Timestamp &other) const
  {
    return microSecondsSinceEpoch_ == other.microSecondsSinceEpoch_;
  }

  bool operator<(const Timestamp &other) const
  {
    return microSecondsSinceEpoch_ < other.microSecondsSinceEpoch_;
  }

  bool operator>(const Timestamp &other) const
  {
    return microSecondsSinceEpoch_ > other.microSecondsSinceEpoch_;
  }

  static Timestamp now();

  static const int kMicroSecondsPerSecond = 1000 * 1000;

 private:
  int64_t microSecondsSinceEpoch_;
};

inline double timeDifference(Timestamp high, Timestamp low)
{
  int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
  return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

}  // namespace easy

#endif
