#include "easy/base/Scheduler.h"
#include "easy/base/Fiber.h"
#include "easy/base/Logger.h"
#include "easy/base/Macro.h"
#include "easy/base/Mutex.h"
#include "easy/base/hook.h"

#include <algorithm>
#include <functional>
#include <memory>

namespace easy {
static Logger::ptr logger = EASY_LOG_NAME("system");

static thread_local Scheduler* t_scheduler = nullptr;
static thread_local Fiber* t_scheduler_fiber = nullptr;

Scheduler::Scheduler(int threadNums, bool use_caller, const std::string& name)
    : name_(name) {
  EASY_ASSERT(threadNums > 0);
  if (use_caller) {
    Fiber::GetThis();

    --threadNums;

    // ensure that one scheduler peer one thread
    EASY_ASSERT(Scheduler::GetThis() == nullptr);

    t_scheduler = this;

    // stacksize > 0
    callerFiber_.reset(NewFiber(std::bind(&Scheduler::run, this), 0, true),
                       FreeFiber);

    t_scheduler_fiber = callerFiber_.get();

    callerTid_ = Thread::GetCurrentThreadId();

    threadIds_.push_back(callerTid_);
  } else {
    callerTid_ = -1;
  }

  threadNums_ = static_cast<size_t>(threadNums);

  EASY_LOG_DEBUG(logger) << "Scheduler ctor name=" << name_;
}

Scheduler::~Scheduler() {
  EASY_LOG_DEBUG(logger) << "Scheduler dtor~ name=" << name_;
  EASY_ASSERT(!running_);
  if (Scheduler::GetThis() == this) {
    t_scheduler = nullptr;
  }
}

bool Scheduler::canStop()  // virtual
{
  ReadLockGuard lock(lock_);
  return !running_ && tasks_.empty() && activeThreadNums_.get() == 0;
}

void Scheduler::weakup()  // virtual
{}

Scheduler* Scheduler::GetThis() { return t_scheduler; }

Fiber* Scheduler::GetSchedulerFiber() { return t_scheduler_fiber; }

void Scheduler::start() {
  WriteLockGuard lock(lock_);
  if (running_) {
    return;
  }
  running_ = true;
  EASY_ASSERT(threads_.empty());
  threads_.resize(threadNums_);
  for (size_t i = 0; i < threadNums_; i++) {
    threads_[i] = std::make_shared<Thread>(std::bind(&Scheduler::run, this),
                                           name_ + "_" + std::to_string(i));
    threadIds_.push_back(threads_[i]->id());
  }
}

void Scheduler::stop() {
  if (callerFiber_ && threadNums_ == 0 &&
      (callerFiber_->finish() || callerFiber_->state() == Fiber::INIT)) {
    // only one use_caller thread
    running_ = false;
    if (canStop()) {
      return;
    }
  }

  running_ = false;

  for (size_t i = 0; i < threadNums_; ++i) {
    weakup();
  }

  if (callerFiber_) {
    weakup();
    if (!canStop()) {
      callerFiber_->resume();
    }
  }

  {
    for (auto& t : threads_) {
      t->join();
    }
    threads_.clear();
  }

  if (canStop()) {
    return;
  }
}

void Scheduler::idle()  // virtual
{
  EASY_LOG_DEBUG(logger) << "idle";
  while (!canStop()) {
    Fiber::YieldToHold();
  }
}

void Scheduler::handleFiber(Fiber::ptr& fiber) {
  fiber->sched_resume();

  switch (fiber->state_) {
    case Fiber::INIT:
    case Fiber::EXEC:
    case Fiber::HOLD:
      fiber->state_ = Fiber::HOLD;
      break;
    case Fiber::READY:
      schedule(fiber);
      break;
    case Fiber::TERM:
    case Fiber::EXCEPT:
      break;
  }
}

Scheduler::Task::ptr Scheduler::take() {
  WriteLockGuard lock(lock_);
  for (auto cur = tasks_.begin(); cur != tasks_.end(); ++cur) {
    auto task = *cur;
    if (task->threadId_ != -1 &&
        task->threadId_ != Thread::GetCurrentThreadId()) {
      // designate thread
      continue;
    }
    EASY_ASSERT(task->fiber_ || task->cb_);
    if (task->fiber_ && task->fiber_->state() == Fiber::EXEC) {
      continue;
    }
    tasks_.erase(cur);
    return task;
  }
  return nullptr;
}

void Scheduler::run() {
  SetHookEnable(true);  // enable hook

  t_scheduler = this;
  if (Thread::GetCurrentThreadId() != callerTid_) {
    // thread in pool should create a main fiber as scheduler fiber
    t_scheduler_fiber = Fiber::GetThis().get();
  }

  Fiber::ptr idle(NewFiber(std::bind(&Scheduler::idle, this)), FreeFiber);

  while (true) {
    auto task = take();
    if (idle->finish()) {
      break;
    }
    if (!task) {
      idleThreadNums_.increment();
      handleFiber(idle);
      idleThreadNums_.decrement();
      continue;
    }
    if (task->cb_) {
      EASY_ASSERT(!task->fiber_);
      Fiber::ptr fiber(NewFiber(task->cb_), FreeFiber);
      task->fiber_ = std::move(fiber);
      task->cb_ = nullptr;
    }
    if (task->fiber_ && !task->fiber_->finish()) {
      activeThreadNums_.increment();
      handleFiber(task->fiber_);
      activeThreadNums_.decrement();
      task->fiber_ = nullptr;
    }
  }
}
}  // namespace easy
