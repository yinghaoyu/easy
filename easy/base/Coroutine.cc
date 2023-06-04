#include "Coroutine.h"
#include "Logger.h"
#include "easy_define.h"

#include <sys/mman.h>
#include <atomic>
#include <memory>

namespace easy
{

static Logger::ptr logger = EASY_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id{0};
static std::atomic<uint64_t> s_fiber_count{0};

static thread_local Coroutine *t_running_co = nullptr;
static thread_local Coroutine::ptr t_main_co = nullptr;

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
  return new Coroutine();
}

Coroutine *NewCoroutine(std::function<void()> cb,
                        size_t stacksize,
                        bool use_caller)
{
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
  SetRunning(this);
  EASY_CHECK(getcontext(&ctx_));
  ++s_fiber_count;
  EASY_LOG_DEBUG(logger) << "Coroutine ctor~ main id=" << id_;
}

Coroutine::Coroutine(std::function<void()> cb,
                     size_t stacksize,
                     bool use_caller_thread)
    : id_(++s_fiber_id), cb_(cb)
{
  ++s_fiber_count;
  stacksize_ = stacksize;

  stack_ = StackAllocator::Alloc(stacksize_);
  EASY_CHECK(getcontext(&ctx_));
  ctx_.uc_link = nullptr;
  ctx_.uc_stack.ss_sp = stack_;
  ctx_.uc_stack.ss_size = stacksize_;

  makecontext(&ctx_, &Coroutine::MainFunc, 0);

  EASY_LOG_DEBUG(logger) << "Coroutine ctor~ id=" << id_;
}

Coroutine::~Coroutine()
{
  --s_fiber_count;
  if (stack_)
  {
    StackAllocator::Dealloc(stack_, stacksize_);
  }
  else
  {
    EASY_ASSERT(!cb_);

    Coroutine *cur = t_running_co;
    if (cur == this)
    {
      SetRunning(nullptr);
    }
  }
  EASY_LOG_DEBUG(logger) << "Coroutine dtor~ id=" << id_
                         << " total=" << s_fiber_count;
}

void Coroutine::resume()
{
  SetRunning(this);
  EASY_CHECK(swapcontext(&t_main_co->ctx_, &ctx_));
}

void Coroutine::yield()
{
  SetRunning(t_main_co.get());
  EASY_CHECK(swapcontext(&ctx_, &t_main_co->ctx_));
}

void Coroutine::reset(std::function<void()> cb)
{
  EASY_ASSERT(stack_);
  cb_ = cb;
  EASY_CHECK(getcontext(&ctx_));

  ctx_.uc_link = nullptr;
  ctx_.uc_stack.ss_sp = stack_;
  ctx_.uc_stack.ss_size = stacksize_;

  makecontext(&ctx_, &Coroutine::MainFunc, 0);
}

void Coroutine::SetRunning(Coroutine *co)
{
  t_running_co = co;
}

Coroutine::ptr Coroutine::GetRunning()
{
  if (t_running_co)
  {
    return t_running_co->shared_from_this();
  }
  Coroutine::ptr main_fiber(NewCoroutine(), FreeCoroutine);
  EASY_ASSERT(t_running_co == main_fiber.get());

  t_main_co = std::move(main_fiber);

  return t_running_co->shared_from_this();
}

uint64_t Coroutine::CurrentCoroutineId()
{
  if (t_running_co)
  {
    return t_running_co->id();
  }
  return 0;
}

void Coroutine::MainFunc()
{
  Coroutine::ptr cur = GetRunning();
  EASY_ASSERT(cur);
  try
  {
    cur->cb_();
    cur->cb_ = nullptr;
  }
  catch (std::exception &ex)
  {
    EASY_LOG_ERROR(logger) << "Coroutine Except: " << ex.what()
                           << " coroutineId=" << cur->id() << std::endl
                           << easy::util::backtrace_to_string();
  }
  catch (...)
  {
    EASY_LOG_ERROR(logger) << "Fiber Except"
                           << " coroutineId=" << cur->id() << std::endl
                           << easy::util::backtrace_to_string();
  }

  auto raw_ptr = cur.get();

  cur.reset();

  raw_ptr->yield();

  EASY_ASSERT_MESSAGE(
      false, "Never reach here, coroutineId=" + std::to_string(raw_ptr->id()));
}

}  // namespace easy
