#include "easy/base/Coroutine.h"
#include "easy/base/Atomic.h"
#include "easy/base/Config.h"
#include "easy/base/Logger.h"
#include "easy/base/Scheduler.h"
#include "easy/base/common.h"
#include "easy/base/Macro.h"

#include <sys/mman.h>
#include <cassert>
#include <memory>

namespace easy
{

static Logger::ptr logger = EASY_LOG_NAME("system");

static AtomicInt<uint64_t> s_numsCreated{0};
static AtomicInt<uint64_t> s_coroutine_count{0};

static thread_local Coroutine *t_running_coroutine = nullptr;
static thread_local Coroutine::ptr t_root_coroutine = nullptr;

static ConfigVar<uint32_t>::ptr co_stack_size =
    Config::Lookup<uint32_t>("coroutine.stack_size",
                             128 * 1024,
                             "coroutine stack size");

class MallocStackAllocator
{
 public:
  static void *Alloc(size_t size) { return malloc(size); }

  static void Dealloc(void *vp, size_t size) { return free(vp); }
};

class MMapStackAllocator
{
 public:
  static void *Alloc(size_t size)
  {
    return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1,
                0);
  }

  static void Dealloc(void *vp, size_t size) { munmap(vp, size); }
};

// using StackAllocator = MallocStackAllocator;

using StackAllocator = MMapStackAllocator;

Coroutine *NewCoroutine()
{
  return new Coroutine();  // root continue only
}

Coroutine *NewCoroutine(std::function<void()> cb,
                        size_t stacksize,
                        bool use_caller)
{
  stacksize = stacksize ? stacksize : co_stack_size->value();
  Coroutine *p = static_cast<Coroutine *>(
      StackAllocator::Alloc(sizeof(Coroutine) + stacksize));
  return new (p) Coroutine(cb, stacksize, use_caller);
}

void FreeCoroutine(Coroutine *ptr)
{
  ptr->~Coroutine();
  StackAllocator::Dealloc(ptr, 0);
}

Coroutine::Coroutine()
{
  Coroutine::SetThis(this);
  state_ = EXEC;  // root should be EXEC
  EASY_CHECK(getcontext(&ctx_));
  s_coroutine_count.increment();
  EASY_LOG_DEBUG(logger) << "Coroutine ctor root id=" << id_;
}

Coroutine::Coroutine(std::function<void()> cb,
                     size_t stacksize,
                     bool use_caller)
    : id_(s_numsCreated.incrementAndFetch()), cb_(cb)
{
  s_coroutine_count.increment();

  EASY_ASSERT(stacksize != 0);
  stacksize_ = stacksize;

  stack_ = StackAllocator::Alloc(stacksize_);
  EASY_CHECK(getcontext(&ctx_));
  ctx_.uc_link = nullptr;
  ctx_.uc_stack.ss_sp = stack_;
  ctx_.uc_stack.ss_size = stacksize_;

  makecontext(&ctx_, &Coroutine::MainFunc, 0);

  EASY_LOG_DEBUG(logger) << "Coroutine ctor sub id=" << id_;
}

Coroutine::~Coroutine()
{
  s_coroutine_count.decrement();
  if (stack_)
  {
    // sub coroutine should free stack
    EASY_ASSERT(state_ == INIT || state_ == TERM || state_ == EXCEPT);
    StackAllocator::Dealloc(stack_, stacksize_);
  }
  else
  {
    EASY_ASSERT(!cb_);  // root coroutine has no cb
    EASY_ASSERT(state_ == EXEC);

    if (t_running_coroutine == this)
    {
      Coroutine::SetThis(nullptr);
    }
  }
  EASY_LOG_DEBUG(logger) << "Coroutine dtor~ id=" << id_
                         << " total=" << s_coroutine_count.get();
}

void Coroutine::sched_resume()
{
  EASY_ASSERT(state_ == INIT || state_ == READY || state_ == HOLD);
  Coroutine::SetThis(this);
  state_ = EXEC;
  EASY_CHECK(swapcontext(&Scheduler::GetSchedulerCoroutine()->ctx_, &ctx_));
}

void Coroutine::sched_yield()
{
  Coroutine::SetThis(Scheduler::GetSchedulerCoroutine());
  // state_ = HOLD;  don't do this

  EASY_CHECK(swapcontext(&ctx_, &Scheduler::GetSchedulerCoroutine()->ctx_));
}

void Coroutine::resume()
{
  EASY_ASSERT_MESSAGE(t_root_coroutine, "root coroutine not exist");
  EASY_ASSERT(state_ == INIT || state_ == READY || state_ == HOLD);
  Coroutine::SetThis(this);
  state_ = EXEC;
  EASY_CHECK(swapcontext(&t_root_coroutine->ctx_, &ctx_));
}

void Coroutine::yield()
{
  EASY_ASSERT_MESSAGE(t_root_coroutine, "root coroutine not exist");
  SetThis(t_root_coroutine.get());
  EASY_CHECK(swapcontext(&ctx_, &t_root_coroutine->ctx_));
}

void Coroutine::reset(std::function<void()> cb)
{
  EASY_ASSERT(stack_);
  cb_ = std::move(cb);
  EASY_CHECK(getcontext(&ctx_));

  ctx_.uc_link = nullptr;
  ctx_.uc_stack.ss_sp = stack_;
  ctx_.uc_stack.ss_size = stacksize_;

  makecontext(&ctx_, &Coroutine::MainFunc, 0);

  state_ = INIT;
}

void Coroutine::SetThis(Coroutine *co)
{
  t_running_coroutine = co;
}

Coroutine::ptr Coroutine::GetThis()
{
  if (t_running_coroutine)
  {
    return t_running_coroutine->shared_from_this();
  }
  Coroutine::ptr root(NewCoroutine(), FreeCoroutine);

  EASY_ASSERT(t_running_coroutine == root.get());

  t_root_coroutine = std::move(root);

  return t_running_coroutine->shared_from_this();
}

void Coroutine::YieldToHold()
{
  Coroutine::ptr cur = GetThis();
  cur->state_ = HOLD;
  if (Scheduler::GetThis() &&
      Scheduler::GetSchedulerCoroutine() != t_running_coroutine)
  {
    cur->sched_yield();
  }
  else
  {
    cur->yield();
  }
}

uint64_t Coroutine::CurrentCoroutineId()
{
  if (t_running_coroutine)
  {
    return t_running_coroutine->id();
  }
  return 0;  // root coroutine
}

void Coroutine::MainFunc()
{
  Coroutine::ptr cur = GetThis();
  EASY_ASSERT(cur);
  try
  {
    cur->cb_();
    cur->cb_ = nullptr;
    cur->state_ = TERM;
  }
  catch (std::exception &ex)
  {
    cur->state_ = EXCEPT;
    EASY_LOG_ERROR(logger) << "Coroutine Except: " << ex.what()
                           << " coroutineId=" << cur->id() << std::endl
                           << easy::BacktraceToString();
  }
  catch (...)
  {
    cur->state_ = EXCEPT;
    EASY_LOG_ERROR(logger) << "Fiber Except"
                           << " coroutineId=" << cur->id() << std::endl
                           << easy::BacktraceToString();
  }

  auto raw_ptr = cur.get();

  cur.reset();

  auto scheduler = Scheduler::GetThis();

  /*if (scheduler && scheduler->callerThreadId_ == Thread::GetCurrentThreadId()
     && scheduler->callerCoroutine_.get() != t_running_coroutine)*/
  if (scheduler && Scheduler::GetSchedulerCoroutine() != t_running_coroutine)
  {
    // only sub coroutine in the scheduler can sched_yield
    raw_ptr->sched_yield();
  }
  else
  {
    raw_ptr->yield();
  }

  EASY_ASSERT_MESSAGE(
      false, "Never reach here, coroutineId=" + std::to_string(raw_ptr->id()));
}

}  // namespace easy
