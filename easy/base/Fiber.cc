#include "easy/base/Fiber.h"
#include "easy/base/Atomic.h"
#include "easy/base/Config.h"
#include "easy/base/Logger.h"
#include "easy/base/Macro.h"
#include "easy/base/Scheduler.h"
#include "easy/base/common.h"

#include <sys/mman.h>
#include <cassert>

namespace easy
{

static Logger::ptr logger = ELOG_NAME("system");

static AtomicInt<uint64_t> s_numsCreated{0};
static AtomicInt<uint64_t> s_fiber_count{0};

static thread_local Fiber*     t_running_fiber = nullptr;
static thread_local Fiber::ptr t_root_fiber    = nullptr;

static ConfigVar<uint32_t>::ptr fiber_stack_size = Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");

class MallocStackAllocator
{
  public:
    static void* Alloc(size_t size) { return malloc(size); }

    static void Dealloc(void* vp, size_t size) { return free(vp); }
};

class MMapStackAllocator
{
  public:
    static void* Alloc(size_t size) { return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0); }

    static void Dealloc(void* vp, size_t size) { munmap(vp, size); }
};

// using StackAllocator = MallocStackAllocator;

using StackAllocator = MMapStackAllocator;

Fiber* NewFiber()
{
    return new Fiber();  // root continue only
}

Fiber* NewFiber(std::function<void()> cb, size_t stacksize, bool use_caller)
{
    stacksize = stacksize ? stacksize : fiber_stack_size->value();
    Fiber* p  = static_cast<Fiber*>(StackAllocator::Alloc(sizeof(Fiber) + stacksize));
    return new (p) Fiber(cb, stacksize, use_caller);
}

void FreeFiber(Fiber* ptr)
{
    ptr->~Fiber();
    StackAllocator::Dealloc(ptr, 0);
}

Fiber::Fiber()
{
    Fiber::SetThis(this);
    state_ = EXEC;  // root should be EXEC
    EASY_CHECK(getcontext(&ctx_));
    s_fiber_count.increment();
    ELOG_DEBUG(logger) << "Fiber ctor root id=" << id_;
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller) : id_(s_numsCreated.incrementAndFetch()), cb_(cb)
{
    s_fiber_count.increment();

    EASY_ASSERT(stacksize != 0);
    stacksize_ = stacksize;

    stack_ = StackAllocator::Alloc(stacksize_);
    EASY_CHECK(getcontext(&ctx_));
    ctx_.uc_link          = nullptr;
    ctx_.uc_stack.ss_sp   = stack_;
    ctx_.uc_stack.ss_size = stacksize_;

    makecontext(&ctx_, &Fiber::MainFunc, 0);

    ELOG_DEBUG(logger) << "Fiber ctor sub id=" << id_;
}

Fiber::~Fiber()
{
    s_fiber_count.decrement();
    if (stack_)
    {
        // sub fiber should free stack
        EASY_ASSERT(state_ == INIT || state_ == TERM || state_ == EXCEPT);
        StackAllocator::Dealloc(stack_, stacksize_);
    }
    else
    {
        EASY_ASSERT(!cb_);  // root fiber has no cb
        EASY_ASSERT(state_ == EXEC);

        if (t_running_fiber == this)
        {
            Fiber::SetThis(nullptr);
        }
    }
    ELOG_DEBUG(logger) << "Fiber dtor~ id=" << id_ << " total=" << s_fiber_count.get();
}

void Fiber::sched_resume()
{
    EASY_ASSERT(state_ == INIT || state_ == READY || state_ == HOLD);
    Fiber::SetThis(this);
    state_ = EXEC;
    EASY_CHECK(swapcontext(&Scheduler::GetSchedulerFiber()->ctx_, &ctx_));
}

void Fiber::sched_yield()
{
    Fiber::SetThis(Scheduler::GetSchedulerFiber());
    // state_ = HOLD;  don't do this

    EASY_CHECK(swapcontext(&ctx_, &Scheduler::GetSchedulerFiber()->ctx_));
}

void Fiber::resume()
{
    EASY_ASSERT_MESSAGE(t_root_fiber, "root fiber not exist");
    EASY_ASSERT(state_ == INIT || state_ == READY || state_ == HOLD);
    Fiber::SetThis(this);
    state_ = EXEC;
    EASY_CHECK(swapcontext(&t_root_fiber->ctx_, &ctx_));
}

void Fiber::yield()
{
    EASY_ASSERT_MESSAGE(t_root_fiber, "root fiber not exist");
    SetThis(t_root_fiber.get());
    EASY_CHECK(swapcontext(&ctx_, &t_root_fiber->ctx_));
}

void Fiber::reset(std::function<void()> cb)
{
    EASY_ASSERT(stack_);
    cb_ = std::move(cb);
    EASY_CHECK(getcontext(&ctx_));

    ctx_.uc_link          = nullptr;
    ctx_.uc_stack.ss_sp   = stack_;
    ctx_.uc_stack.ss_size = stacksize_;

    makecontext(&ctx_, &Fiber::MainFunc, 0);

    state_ = INIT;
}

void Fiber::SetThis(Fiber* co) { t_running_fiber = co; }

Fiber::ptr Fiber::GetThis()
{
    if (t_running_fiber)
    {
        return t_running_fiber->shared_from_this();
    }
    Fiber::ptr root(NewFiber(), FreeFiber);

    EASY_ASSERT(t_running_fiber == root.get());

    t_root_fiber = std::move(root);

    return t_running_fiber->shared_from_this();
}

void Fiber::YieldToHold()
{
    Fiber::ptr cur = GetThis();
    cur->state_    = HOLD;
    if (Scheduler::GetThis() && Scheduler::GetSchedulerFiber() != t_running_fiber)
    {
        cur->sched_yield();
    }
    else
    {
        cur->yield();
    }
}

uint64_t Fiber::CurrentFiberId()
{
    if (t_running_fiber)
    {
        return t_running_fiber->id();
    }
    return 0;  // root fiber
}

void Fiber::MainFunc()
{
    Fiber::ptr cur = GetThis();
    EASY_ASSERT(cur);
    try
    {
        cur->cb_();
        cur->cb_    = nullptr;
        cur->state_ = TERM;
    }
    catch (std::exception& ex)
    {
        cur->state_ = EXCEPT;
        ELOG_ERROR(logger) << "Fiber Except: " << ex.what() << " fiberId=" << cur->id() << std::endl << easy::BacktraceToString();
    }
    catch (...)
    {
        cur->state_ = EXCEPT;
        ELOG_ERROR(logger) << "Fiber Except"
                           << " fiberId=" << cur->id() << std::endl
                           << easy::BacktraceToString();
    }

    auto raw_ptr = cur.get();

    cur.reset();

    auto scheduler = Scheduler::GetThis();

    /*if (scheduler && scheduler->callerThreadId_ == Thread::GetCurrentThreadId()
       && scheduler->callerFiber_.get() != t_running_fiber)*/
    if (scheduler && Scheduler::GetSchedulerFiber() != t_running_fiber)
    {
        // only sub fiber in the scheduler can sched_yield
        raw_ptr->sched_yield();
    }
    else
    {
        raw_ptr->yield();
    }

    EASY_ASSERT_MESSAGE(false, "Never reach here, fiberId=" + std::to_string(raw_ptr->id()));
}

}  // namespace easy
