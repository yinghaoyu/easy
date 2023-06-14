#ifndef __EASY_COROUTINE_H__
#define __EASY_COROUTINE_H__

#include "easy/base/noncopyable.h"

#include <ucontext.h>
#include <cstdint>
#include <functional>
#include <memory>

namespace easy
{
class Coroutine;

Coroutine *NewCoroutine();

Coroutine *NewCoroutine(std::function<void()> cb,
                        size_t stacksize = 0,
                        bool use_caller = false);

void FreeCoroutine(Coroutine *ptr);

class Coroutine : noncopyable, public std::enable_shared_from_this<Coroutine>
{
  friend Coroutine *NewCoroutine();
  friend Coroutine *NewCoroutine(std::function<void()> cb,
                                 size_t stacksize,
                                 bool use_caller);
  friend void FreeCoroutine(Coroutine *ptr);

  friend class Scheduler;

 public:
  typedef std::shared_ptr<Coroutine> ptr;

  enum State
  {
    INIT,
    READY,
    HOLD,
    EXEC,
    TERM,
    EXCEPT
  };

  Coroutine(std::function<void()> cb,
            size_t stacksize = 0,
            bool use_caller = false);

  ~Coroutine();

  void sched_resume();

  void sched_yield();

  void resume();

  void yield();

  void reset(std::function<void()> cb);

  uint64_t id() const { return id_; }

  State state() const { return state_; }

  bool finish() const { return (state_ == TERM || state_ == EXCEPT); }

  static void SetThis(Coroutine *co);
  static Coroutine::ptr GetThis();
  static void YieldToHold();
  static uint64_t CurrentCoroutineId();

 private:
  Coroutine();  // root coroutine only

  static void MainFunc();

 private:
  uint64_t id_ = 0;
  size_t stacksize_ = 0;
  State state_ = State::INIT;
  ucontext_t ctx_;
  void *stack_ = nullptr;
  std::function<void()> cb_;
};

}  // namespace easy

#endif
