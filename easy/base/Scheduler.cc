#include "easy/base/Scheduler.h"
#include "easy/base/Coroutine.h"
#include "easy/base/Logger.h"
#include "easy/base/Mutex.h"
#include "easy/base/easy_define.h"

#include <algorithm>
#include <functional>
#include <memory>

namespace easy
{
static Logger::ptr logger = EASY_LOG_NAME("system");

static thread_local Scheduler *t_scheduler = nullptr;
static thread_local Coroutine *t_scheduler_coroutine = nullptr;

Scheduler::Scheduler(int threadNums, bool use_caller, const std::string &name)
    : name_(name)
{
  EASY_ASSERT(threadNums > 0);
  if (use_caller)
  {
    Coroutine::GetThis();

    --threadNums;

    // ensure that one scheduler peer one thread
    EASY_ASSERT(Scheduler::GetThis() == nullptr);

    t_scheduler = this;

    // stacksize > 0
    callerCo_.reset(
        NewCoroutine(std::bind(&Scheduler::run, this), 128 * 1024, true),
        FreeCoroutine);

    t_scheduler_coroutine = callerCo_.get();

    callerTid_ = Thread::GetCurrentThreadId();

    threadIds_.push_back(callerTid_);
  }
  else
  {
    callerTid_ = -1;
  }

  threadNums_ = static_cast<size_t>(threadNums);

  EASY_LOG_DEBUG(logger) << "Scheduler ctor name=" << name_;
}

Scheduler::~Scheduler()
{
  EASY_LOG_DEBUG(logger) << "Scheduler dtor~ name=" << name_;
  EASY_ASSERT(!running_);
  if (Scheduler::GetThis() == this)
  {
    t_scheduler = nullptr;
  }
}

bool Scheduler::canStop() // virtual
{
  ReadLockGuard lock(lock_);
  return !running_ && tasks_.empty() && activeThreadNums_.get() == 0;
}

void Scheduler::tickle() // virtual
{
}

Scheduler *Scheduler::GetThis()
{
  return t_scheduler;
}

Coroutine *Scheduler::GetSchedulerCoroutine()
{
  return t_scheduler_coroutine;
}

void Scheduler::start()
{
  {
    WriteLockGuard lock(lock_);
    if (running_)
    {
      return;
    }
    running_ = true;
    EASY_ASSERT(threads_.empty());
    threads_.resize(threadNums_);
    for (size_t i = 0; i < threadNums_; i++)
    {
      threads_[i] = std::make_shared<Thread>(std::bind(&Scheduler::run, this),
                                             name_ + "_" + std::to_string(i));
      threadIds_.push_back(threads_[i]->id());
    }
  }
}

void Scheduler::stop()
{
  if (callerCo_ && threadNums_ == 0 &&
      (callerCo_->finish() ||
       callerCo_->state() == Coroutine::INIT))
  {
    // only one use_caller thread
    running_ = false;
    if (canStop())
    {
      return;
    }
  }

  running_ = false;

  for (size_t i = 0; i < threadNums_; ++i)
  {
    // weak up
    tickle();
  }

  if (callerCo_)
  {
    // weak up
    tickle();
    if (!canStop())
    {
      callerCo_->resume();
    }
  }

  {
    for (auto &t : threads_)
    {
      t->join();
    }
    threads_.clear();
  }

  if (canStop())
  {
    return;
  }
}

void Scheduler::idle()  // virtual
{
  EASY_LOG_DEBUG(logger) << "idle";
  while (!canStop())
  {
    Coroutine::YieldToHold();
  }
}

void Scheduler::handleCoroutine(Coroutine::ptr co)
{
  co->sched_resume();

  switch (co->state_)
  {
  case Coroutine::INIT:
  case Coroutine::EXEC:
  case Coroutine::HOLD:
    co->state_ = Coroutine::HOLD;
    break;
  case Coroutine::READY:
    schedule(co);
    break;
  case Coroutine::TERM:
  case Coroutine::EXCEPT:
    break;
  }
}

Scheduler::Task::ptr Scheduler::take()
{
  WriteLockGuard lock(lock_);
  for (auto cur = tasks_.begin(); cur != tasks_.end(); ++cur)
  {
    auto task = *cur;
    if (task->threadId_ != -1 &&
        task->threadId_ != Thread::GetCurrentThreadId())
    {
      // weak up for other thread
      tickle();
      continue;
    }
    EASY_ASSERT(task->co_ || task->cb_);
    if (task->co_ && task->co_->state() == Coroutine::EXEC)
    {
      continue;
    }
    tasks_.erase(cur);
    return task;
  }
  return nullptr;
}

void Scheduler::run()
{
  t_scheduler = this;
  if (Thread::GetCurrentThreadId() != callerTid_)
  {
    // thread in pool should create a coroutine as scheduler coroutine
    t_scheduler_coroutine = Coroutine::GetThis().get();
  }

  Coroutine::ptr idleCo(NewCoroutine(std::bind(&Scheduler::idle, this)),
                        FreeCoroutine);

  while (true)
  {
    auto task = take();
    if (idleCo->finish())
    {
      break;
    }
    if (!task)
    {
      handleCoroutine(idleCo);
      continue;
    }
    if (task->cb_)
    {
      EASY_ASSERT(!task->co_);
      Coroutine::ptr co(NewCoroutine(task->cb_), FreeCoroutine);
      task->co_ = co;
      task->cb_ = nullptr;
    }
    if (task->co_ && !task->co_->finish())
    {
      activeThreadNums_.increment();
      handleCoroutine(task->co_);
      activeThreadNums_.decrement();
      task->co_ = nullptr;
    }
  }
}
}  // namespace easy
