#ifndef __EASY_FIBER_H__
#define __EASY_FIBER_H__

#include "easy/base/noncopyable.h"

#include <ucontext.h>
#include <cstdint>
#include <functional>
#include <memory>

namespace easy {
class Fiber;

Fiber* NewFiber();

Fiber* NewFiber(std::function<void()> cb, size_t stacksize = 0,
                bool use_caller = false);

void FreeFiber(Fiber* ptr);

class Fiber : noncopyable, public std::enable_shared_from_this<Fiber> {
  friend Fiber* NewFiber();
  friend Fiber* NewFiber(std::function<void()> cb, size_t stacksize,
                         bool use_caller);
  friend void FreeFiber(Fiber* ptr);

  friend class Scheduler;

 public:
  typedef std::shared_ptr<Fiber> ptr;

  enum State { INIT, READY, HOLD, EXEC, TERM, EXCEPT };

  Fiber(std::function<void()> cb, size_t stacksize = 0,
        bool use_caller = false);

  ~Fiber();

  void sched_resume();

  void sched_yield();

  void resume();

  void yield();

  void reset(std::function<void()> cb);

  uint64_t id() const { return id_; }

  State state() const { return state_; }

  bool finish() const { return (state_ == TERM || state_ == EXCEPT); }

  static void SetThis(Fiber* co);
  static Fiber::ptr GetThis();
  static void YieldToHold();
  static uint64_t CurrentFiberId();

 private:
  Fiber();  // root coroutine only

  static void MainFunc();

 private:
  uint64_t id_{0};
  size_t stacksize_{0};
  State state_{State::INIT};
  ucontext_t ctx_;
  void* stack_{nullptr};
  std::function<void()> cb_;
};

}  // namespace easy

#endif
