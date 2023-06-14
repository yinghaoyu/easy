#ifndef __EASY_TIMER_H__
#define __EASY_TIMER_H__

#include "easy/base/Mutex.h"
#include "easy/base/Timestamp.h"
#include "easy/base/noncopyable.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <set>
#include <vector>

namespace easy
{
class TimerManager;

class Timer : noncopyable, public std::enable_shared_from_this<Timer>
{
  friend class TimerManager;

 public:
  typedef std::shared_ptr<Timer> ptr;

  Timer(int64_t interval,
        std::function<void()> cb,
        bool repeat,
        TimerManager *manager);

  Timer(Timestamp time);

  bool cancel();

  bool refresh();

  bool reset(int64_t interval, bool from_now);

 private:
  struct Comparator
  {
    bool operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const;
  };

 private:
  bool repeat_{false};
  int64_t interval_{0};  // ms
  Timestamp expiration_;
  std::function<void()> cb_;
  TimerManager *manager_ = nullptr;
};

class TimerManager : noncopyable
{
  friend class Timer;

 public:
  TimerManager();

  virtual ~TimerManager();

  Timer::ptr addTimer(uint64_t interval,
                      std::function<void()> cb,
                      bool repeat = false);

  Timer::ptr addConditionTimer(uint64_t interval,
                               std::function<void()> cb,
                               std::weak_ptr<void> weak_cond,
                               bool repeat = false);
  int64_t getNextTimer();

  void listExpiredCallback(std::vector<std::function<void()>> &fns);

  bool hasTimer();

 protected:
  virtual void onTimerInsertedAtFront() = 0;

  void addTimer(Timer::ptr val, WriteLockGuard &lock);

 private:
  bool detectClockRollover(Timestamp now);

 private:
  RWLock lock_;
  std::set<Timer::ptr, Timer::Comparator> timers_;
  bool tickled_{false};
  Timestamp previouseTime_;
};
}  // namespace easy

#endif
