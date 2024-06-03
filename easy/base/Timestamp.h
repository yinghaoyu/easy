#ifndef __EASY_TIMESTAMP_H__
#define __EASY_TIMESTAMP_H__

#include "easy/base/copyable.h"

#include <cstdint>

namespace easy
{
class Timestamp : public copyable
{
  public:
    Timestamp() : microSecondsSinceEpoch_(0) {}

    explicit Timestamp(int64_t msc) : microSecondsSinceEpoch_(msc) {}

    Timestamp(const Timestamp& other) : microSecondsSinceEpoch_(other.microSecondsSinceEpoch_) {}

    ~Timestamp() {}

    bool valid() const { return microSecondsSinceEpoch_ > 0; }

    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }

    int64_t seconds() const { return microSecondsSinceEpoch_ / kMicroSecondsPerSecond; }

    int64_t microSecondsRemainder() const { return microSecondsSinceEpoch_ % kMicroSecondsPerSecond; }

    void operator=(const Timestamp& other) { microSecondsSinceEpoch_ = other.microSecondsSinceEpoch_; }

    bool operator==(const Timestamp& other) const { return microSecondsSinceEpoch_ == other.microSecondsSinceEpoch_; }

    bool operator<(const Timestamp& other) const { return microSecondsSinceEpoch_ < other.microSecondsSinceEpoch_; }

    bool operator>(const Timestamp& other) const { return microSecondsSinceEpoch_ > other.microSecondsSinceEpoch_; }

    bool operator>=(const Timestamp& other) { return microSecondsSinceEpoch_ >= other.microSecondsSinceEpoch_; }

    bool operator<=(const Timestamp& other) { return microSecondsSinceEpoch_ <= other.microSecondsSinceEpoch_; }

    static Timestamp now();

    static const int kMicroSecondsPerSecond = 1000 * 1000;
    static const int kMillisecondPerSeond   = 1000;
    static const int kSecondsPerHour        = 60 * 60;

  private:
    int64_t microSecondsSinceEpoch_;
};

// ms
inline int64_t timeDifference(Timestamp high, Timestamp low)
{
    int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
    return diff / Timestamp::kMillisecondPerSeond;
}

// ms
inline Timestamp addTime(Timestamp timestamp, int64_t ms)
{
    int64_t delta = static_cast<int64_t>(ms * Timestamp::kMillisecondPerSeond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

}  // namespace easy

#endif
