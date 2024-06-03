#include "easy/base/Thread.h"
#include "easy/base/Atomic.h"
#include "easy/base/Macro.h"

#include <sys/syscall.h>
#include <sys/types.h>

namespace easy
{

static_assert(std::is_same<int, pid_t>::value, "pid_t should be int");

static AtomicInt<uint64_t> s_num_created{0};

static thread_local const char* t_thread_name = "main";
static thread_local int         t_thread_id   = 0;

pid_t gettid() { return static_cast<pid_t>(::syscall(SYS_gettid)); }

static easy::Logger::ptr logger = ELOG_NAME("system");

int Thread::GetCurrentThreadId()
{
    if (EASY_UNLIKELY(!t_thread_id))
    {
        // slow path
        t_thread_id = gettid();
    }
    return t_thread_id;
}

const char* Thread::GetCurrentThreadName() { return t_thread_name; }

Thread::Thread(std::function<void()> cb, const std::string& name) : cb_(cb), name_(name)
{
    setDefaultName();
    EASY_CHECK(pthread_create(&pthreadId_, nullptr, &Thread::run, this));
    semaphore_.wait();
}

Thread::~Thread()
{
    if (pthreadId_)
    {
        pthread_detach(pthreadId_);
    }
}

void Thread::setDefaultName()
{
    int num = static_cast<int>(s_num_created.incrementAndFetch());
    if (name_.empty())
    {
        char buf[32];
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}

void Thread::join()
{
    if (pthreadId_)
    {
        EASY_CHECK(pthread_join(pthreadId_, nullptr));
        pthreadId_ = 0;
    }
}

void* Thread::run(void* arg)
{
    Thread* thread = static_cast<Thread*>(arg);

    thread->id_ = GetCurrentThreadId();

    t_thread_name = thread->name_.c_str();

    pthread_setname_np(pthread_self(), thread->name_.substr(0, 15).c_str());

    std::function<void()> cb;

    cb = std::move(thread->cb_);

    thread->semaphore_.notify();  // everything is ok

    try
    {
        cb();
        thread->name_ = "finished";
    }
    catch (std::exception& ex)
    {
        thread->name_ = "crashed";
        ELOG_ERROR(logger) << "thread crashed id=" << thread->id_;
        abort();
    }
    catch (...)
    {
        thread->name_ = "crashed";
        ELOG_ERROR(logger) << "thread crashed id=" << thread->id_;
        throw;  // rethrow
    }
    return 0;
}

}  // namespace easy
