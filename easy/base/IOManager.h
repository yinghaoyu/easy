#ifndef __EASY_IOMANAGER_H__
#define __EASY_IOMANAGER_H__

#include "easy/base/Atomic.h"
#include "easy/base/Channel.h"
#include "easy/base/Mutex.h"
#include "easy/base/Scheduler.h"
#include "easy/base/Timer.h"
#include "easy/base/noncopyable.h"

#include <functional>
#include <vector>

namespace easy {
class IOManager : public Scheduler, public TimerManager {
 public:
  IOManager(int threadNums = 1, bool use_caller = false,
            const std::string& name = "");

  ~IOManager();

  int addEvent(int fd, Channel::Event event,
               std::function<void()> cb = nullptr);

  bool removeEvent(int fd, Channel::Event event);

  // trigger and cancel
  bool cancelEvent(int fd, Channel::Event);

  // trigger and cancel
  bool cancelAll(int fd);

  static IOManager* GetThis();

 protected:
  void weakup() override;

  bool canStop() override;

  void idle() override;

  void resizeChannels(size_t size);

  void resizeRevent(size_t size);

 private:
  const int kEPollTimeMs = 10000;

  int epollFd_;
  int weakupFds_[2];
  AtomicInt<int> pendingEventCount_;
  RWLock lock_;
  std::vector<epoll_event*> events_;  // events from epoll_wait
  std::vector<Channel*> channels_;
};

}  // namespace easy

#endif
