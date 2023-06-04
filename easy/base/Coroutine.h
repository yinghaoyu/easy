#ifndef __EASY_COROUTINE_H__
#define __EASY_COROUTINE_H__

#include "noncopyable.h"

#include <ucontext.h>
#include <cstdint>
#include <functional>
#include <memory>

namespace easy
{
class Coroutine;

Coroutine *NewCoroutine();

Coroutine *NewCoroutine(std::function<void()> cb,
                        size_t stacksize = 128 * 1024,
                        bool use_caller_thread = false);

void FreeCoroutine(Coroutine *ptr);

class Coroutine : noncopyable, public std::enable_shared_from_this<Coroutine>
{
  friend Coroutine *NewCoroutine();
  friend Coroutine *NewCoroutine(std::function<void()> cb,
                                 size_t stacksize,
                                 bool use_caller_thread);
  friend void FreeCoroutine(Coroutine *ptr);

 public:
  typedef std::shared_ptr<Coroutine> ptr;

  Coroutine(std::function<void()> cb,
            size_t stacksize = 0,
            bool use_caller_thread = false);

  ~Coroutine();

  void resume();

  void yield();

  void reset(std::function<void()> cb);

  uint64_t id() const { return id_; }

  static void SetRunning(Coroutine *co);
  static Coroutine::ptr GetRunning();
  static uint64_t CurrentCoroutineId();

 private:
  Coroutine();

  static void MainFunc();

 private:
  uint64_t id_ = 0;
  size_t stacksize_ = 0;
  ucontext_t ctx_;
  void *stack_ = nullptr;
  std::function<void()> cb_;
};

}  // namespace easy

#endif
