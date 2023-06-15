#ifndef __EASY_IOMANAGER_H__
#define __EASY_IOMANAGER_H__

#include "easy/base/Atomic.h"
#include "easy/base/Mutex.h"
#include "easy/base/Scheduler.h"
#include "easy/base/Timer.h"
#include "easy/base/noncopyable.h"

#include <sys/epoll.h>
#include <functional>
#include <vector>

namespace easy
{
class IOManager : public Scheduler, public TimerManager
{
 public:
  enum Event
  {
    NONE = 0x00,
    READ = EPOLLIN,
    WRITE = EPOLLOUT,
  };

 private:
  class FdContext
  {
   public:
    struct EventContext
    {
      Scheduler *scheduler_ = nullptr;
      Coroutine::ptr co_;
      std::function<void()> cb_;
    };

    EventContext &eventContext(Event event);

    void resetContext(EventContext &ctx);

    void triggerEvent(Event event);

    EventContext read_;
    EventContext write_;
    int fd_;
    Event events_ = NONE;
    SpinLock lock_;
  };

 public:
  IOManager(int threadNums = 1,
            bool use_caller = false,
            const std::string &name = "");

  ~IOManager();

  int addEvent(int fd, Event event, std::function<void()> cb = nullptr);

  bool removeEvent(int fd, Event event);

  bool cancelEvent(int fd, Event);  // trigger and cancel

  bool cancelAll(int fd);  // trigger and cancel

  static IOManager *GetThis();

 protected:
  void tickle() override;

  bool canStop() override;

  void idle() override;

  void onTimerInsertedAtFront() override;

  void contextResize(size_t size);

  bool stopping(int64_t &timeout);

 private:
  int epollFd_;
  int tickleFds_[2];
  AtomicInt<int> pendingEventCount_;
  RWLock lock_;
  std::vector<FdContext *> fdContexts_;
};

}  // namespace easy

#endif
