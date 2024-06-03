#ifndef __EASY_THREAD_H__
#define __EASY_THREAD_H__

#include "easy/base/Mutex.h"
#include "easy/base/noncopyable.h"

#include <pthread.h>
#include <functional>
#include <memory>
#include <string>

namespace easy
{

class Thread : noncopyable
{
  public:
    typedef std::shared_ptr<Thread> ptr;

    Thread(std::function<void()> cb, const std::string& name = "");

    ~Thread();

    void join();

    pid_t id() const { return id_; }

    const std::string& name() const { return name_; }

    static int GetCurrentThreadId();

    static const char* GetCurrentThreadName();

    static void SetCurrentThreadName(const std::string& name);

  private:
    static void* run(void* arg);

    void setDefaultName();

  private:
    pid_t                 id_;  // from syscall
    pthread_t             pthreadId_;
    std::function<void()> cb_;
    std::string           name_;
    Semaphore             semaphore_;
};

}  // namespace easy

#endif
