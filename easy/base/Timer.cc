#include "easy/base/Timer.h"
#include "easy/base/Config.h"
#include "easy/base/Logger.h"
#include "easy/base/Macro.h"
#include "easy/base/Mutex.h"
#include "easy/base/Timestamp.h"

#include <sys/timerfd.h>
#include <unistd.h>
#include <cstdint>
#include <ctime>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>

namespace easy
{
auto logger = ELOG_NAME("system");

bool Timer::Comparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const
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

Timer::Timer(int64_t interval, std::function<void()> cb, bool repeat, TimerManager* manager)
    : repeat_(repeat), interval_(interval), cb_(cb), manager_(manager)
{
    expiration_ = addTime(Timestamp::now(), interval);
}

Timer::Timer(Timestamp time) : expiration_(time) {}

bool Timer::cancel()
{
    WriteLockGuard _(manager_->lock_);
    if (cb_)
    {
        cb_     = nullptr;
        auto it = manager_->timers_.find(shared_from_this());
        manager_->timers_.erase(it);
        return true;
    }
    return false;
}

bool Timer::refresh()
{
    WriteLockGuard _(manager_->lock_);
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
    WriteLockGuard _(manager_->lock_);
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
    Timestamp start = from_now ? Timestamp::now() : addTime(expiration_, -interval_);
    interval_       = interval;
    expiration_     = addTime(start, interval_);
    manager_->addTimer(shared_from_this());
    return true;
}

TimerManager::TimerManager() : timerfd_(createTimerfd()), previous_(Timestamp::now()) {}

TimerManager::~TimerManager() { close(timerfd_); }

Timer::ptr TimerManager::addTimer(uint64_t interval, std::function<void()> cb, bool repeat)
{
    Timer::ptr     timer = easy::protected_make_shared<Timer>(interval, cb, repeat, this);
    WriteLockGuard _(lock_);
    addTimer(timer);
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

Timer::ptr TimerManager::addConditionTimer(uint64_t interval, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool repeat)
{
    return addTimer(interval, std::bind(&OnTimer, weak_cond, cb), repeat);
}

void TimerManager::listExpiredCallback(std::vector<std::function<void()>>& cbs)
{
    Timestamp               now = Timestamp::now();
    std::vector<Timer::ptr> expired;
    {
        ReadLockGuard _(lock_);
        if (timers_.empty())
        {
            return;
        }
    }

    WriteLockGuard _(lock_);
    bool           rollover = detectClockRollover(now);
    if (!rollover && (*timers_.begin())->expiration_ > now)
    {
        // nothing happened
        return;
    }
    Timer::ptr now_timer = easy::protected_make_shared<Timer>(now);
    // system time changed, take out all timer
    auto end = rollover ? timers_.end() : timers_.lower_bound(now_timer);
    while (end != timers_.end() && (*end)->expiration_ == now_timer->expiration_)
    {
        ++end;
    }
    std::copy(timers_.begin(), end, std::back_inserter(expired));
    timers_.erase(timers_.begin(), end);

    cbs.reserve(expired.size());
    for (auto& timer : expired)
    {
        cbs.push_back(timer->cb_);
        if (timer->repeat_)
        {
            timer->expiration_ = addTime(now, timer->interval_);
            addTimer(timer);
        }
        else
        {
            timer->cb_ = nullptr;
        }
    }

    Timestamp nextExpire;
    if (!timers_.empty())
    {
        nextExpire = (*timers_.begin())->expiration_;
    }
    if (nextExpire.valid())
    {
        resetTimerfd(timerfd_, nextExpire);
    }
}

void TimerManager::addTimer(Timer::ptr timer)
{
    auto it              = timers_.insert(timer).first;
    bool earliestChanged = (it == timers_.begin());
    if (earliestChanged)
    {
        resetTimerfd(timerfd_, timer->expiration_);
    }
}

bool TimerManager::hasTimer()
{
    ReadLockGuard _(lock_);
    return !timers_.empty();
}

bool TimerManager::detectClockRollover(Timestamp now)
{
    bool rollover = false;
    // rollover > 1 hour
    if (now < previous_ && now < addTime(previous_, -Timestamp::kSecondsPerHour))
    {
        rollover = true;
    }
    previous_ = now;
    return rollover;
}

int TimerManager::createTimerfd()
{
    int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK /*| TFD_CLOEXEC*/);
    if (timerfd < 0)
    {
        ELOG_ERROR(logger) << "Failed in timerfd_create";
    }
    return timerfd;
}

static struct timespec howMuchTimeFromNow(Timestamp when)
{
    int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 100)
    {
        microseconds = 100;
    }
    struct timespec ts;
    ts.tv_sec  = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts;
}

void TimerManager::resetTimerfd(int timerfd, Timestamp expiration)
{
    // wake up by timerfd_settime()
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memset(&newValue, 0, sizeof newValue);
    memset(&oldValue, 0, sizeof oldValue);
    newValue.it_value = howMuchTimeFromNow(expiration);
    EASY_CHECK(timerfd_settime(timerfd, 0, &newValue, &oldValue));
}

}  // namespace easy
