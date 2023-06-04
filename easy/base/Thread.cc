#include "Thread.h"
#include "easy_define.h"

#include <sys/syscall.h>
#include <sys/types.h>

namespace easy
{
static thread_local Thread *t_thread = nullptr;

static thread_local std::string t_thread_name = "unknow";

pid_t gettid()
{
  return static_cast<pid_t>(::syscall(SYS_gettid));
}

static easy::Logger::ptr logger = EASY_LOG_NAME("system");

Thread *Thread::GetCurrentThread()
{
  return t_thread;
}

Thread::Thread(std::function<void()> cb, const std::string &name)
    : cb_(cb), name_(name)
{
  if (name.empty())
  {
    name_ = "unknow";
  }
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

void Thread::join()
{
  if (pthreadId_)
  {
    EASY_CHECK(pthread_join(pthreadId_, nullptr));
    pthreadId_ = 0;
  }
}

void *Thread::run(void *arg)
{
  Thread *thread = static_cast<Thread *>(arg);

  t_thread = thread;

  thread->id_ = gettid();

  pthread_setname_np(pthread_self(), thread->name_.substr(0, 15).c_str());

  std::function<void()> cb;
  cb.swap(thread->cb_);

  thread->semaphore_.notify();  // everything is ok

  cb();
  return 0;
}

}  // namespace easy
