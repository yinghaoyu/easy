#include "easy/base/Timer.h"
#include "easy/base/Logger.h"
#include "easy/base/Config.h"
#include "easy/base/Mutex.h"
#include "easy/base/Timestamp.h"

#include <cstdint>
#include <ctime>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>

namespace easy
{
auto logger = EASY_LOG_NAME("system");

bool Timer::Comparator::operator()(const Timer::ptr &lhs,
                                   const Timer::ptr &rhs) const
{
  if (!lhs && !rhs)
  {
    return false;
  }
  if (!lhs)
  {
    return true;
  }
  if (!rhs)
  {
    return false;
  }
  if (lhs->expiration_ < rhs->expiration_)
  {
    return true;
  }
  if (rhs->expiration_ < lhs->expiration_)
  {
    return false;
  }
  return lhs.get() < rhs.get();
}

Timer::Timer(int64_t interval,
             std::function<void()> cb,
             bool repeat,
             TimerManager *manager)
    : repeat_(repeat), interval_(interval), cb_(cb), manager_(manager)
{
  expiration_ = addTime(Timestamp::now(), interval);
}

Timer::Timer(Timestamp time) : expiration_(time) {}

bool Timer::cancel()
{
  WriteLockGuard lock(manager_->lock_);
  if (cb_)
  {
    cb_ = nullptr;
    auto it = manager_->timers_.find(shared_from_this());
    manager_->timers_.erase(it);
    return true;
  }
  return false;
}

bool Timer::refresh()
{
  WriteLockGuard lock(manager_->lock_);
  if (!cb_)
  {
    return false;
  }
  auto it = manager_->timers_.find(shared_from_this());
  if (it == manager_->timers_.end())
  {
    return false;
  }
  manager_->timers_.erase(it);  // ensure modification outside std::set
  expiration_ = addTime(Timestamp::now(), interval_);
  manager_->timers_.insert(shared_from_this());
  return true;
}

bool Timer::reset(int64_t interval, bool from_now)
{
  if (interval == interval_ && !from_now)
  {
    return true;
  }
  WriteLockGuard lock(manager_->lock_);
  if (!cb_)
  {
    return false;
  }
  auto it = manager_->timers_.find(shared_from_this());
  if (it == manager_->timers_.end())
  {
    return false;
  }
  manager_->timers_.erase(it);
  Timestamp start =
      from_now ? Timestamp::now() : addTime(expiration_, -interval_);
  interval_ = interval;
  expiration_ = addTime(start, interval_);
  manager_->addTimer(shared_from_this(), lock);
  return true;
}

TimerManager::TimerManager()
{
  previouseTime_ = Timestamp::now();
}

TimerManager::~TimerManager() {}

Timer::ptr TimerManager::addTimer(uint64_t interval,
                                  std::function<void()> cb,
                                  bool repeat)
{
  Timer::ptr timer =
      easy::protected_make_shared<Timer>(interval, cb, repeat, this);
  WriteLockGuard lock(lock_);
  addTimer(timer, lock);
  return timer;
}

static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb)
{
  std::shared_ptr<void> tmp = weak_cond.lock();
  if (tmp)
  {
    cb();
  }
}

Timer::ptr TimerManager::addConditionTimer(uint64_t interval,
                                           std::function<void()> cb,
                                           std::weak_ptr<void> weak_cond,
                                           bool repeat)
{
  return addTimer(interval, std::bind(&OnTimer, weak_cond, cb), repeat);
}

int64_t TimerManager::getNextTimer()
{
  ReadLockGuard lock(lock_);
  tickled_ = false;
  if (timers_.empty())
  {
    return ~0;
  }
  auto next = timers_.begin();
  Timestamp now = Timestamp::now();
  if (now >= (*next)->expiration_)
  {
    // expiration
    return 0;
  }
  else
  {
    // waitting time
    return timeDifference((*next)->expiration_, now);
  }
}

void TimerManager::listExpiredCallback(std::vector<std::function<void()>> &cbs)
{
  Timestamp now = Timestamp::now();
  std::vector<Timer::ptr> expired;
  {
    ReadLockGuard lock(lock_);
    if (timers_.empty())
    {
      return;
    }
  }

  WriteLockGuard lock(lock_);
  bool rollover = detectClockRollover(now);
  if (!rollover && (*timers_.begin())->expiration_ > now)
  {
    // nothing happened
    return;
  }
  Timer::ptr now_timer = easy::protected_make_shared<Timer>(now);
  // system time changed, take all timer
  auto end = rollover ? timers_.end() : timers_.lower_bound(now_timer);
  // while (end != timers_.end() && (*end)->expiration_ ==
  // now_timer->expiration_)
  //{
  //  ++end;
  //}
  std::copy(timers_.begin(), end, std::back_inserter(expired));
  timers_.erase(timers_.begin(), end);

  cbs.reserve(expired.size());
  for (auto &timer : expired)
  {
    cbs.push_back(timer->cb_);
    if (timer->repeat_)
    {
      timer->expiration_ = addTime(now, timer->interval_);
      timers_.insert(timer);
    }
    else
    {
      timer->cb_ = nullptr;
    }
  }
}

void TimerManager::addTimer(Timer::ptr val, WriteLockGuard &lock)
{
  auto it = timers_.insert(val).first;
  bool at_front = (it == timers_.begin());
  lock.unlock();
  if (at_front)
  {
    onTimerInsertedAtFront();
  }
}

bool TimerManager::hasTimer()
{
  ReadLockGuard lock(lock_);
  return !timers_.empty();
}

bool TimerManager::detectClockRollover(Timestamp now)
{
  bool rollover = false;
  // rollover > 1 hour
  if (now < previouseTime_ &&
      now < addTime(previouseTime_, -Timestamp::kSecondsPerHour))
  {
    rollover = true;
  }
  previouseTime_ = now;
  return rollover;
}

}  // namespace easy
