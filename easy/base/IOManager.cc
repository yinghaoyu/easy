#include "easy/base/IOManager.h"
#include "easy/base/Logger.h"
#include "easy/base/Macro.h"
#include "easy/base/Mutex.h"
#include "easy/base/Scheduler.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <algorithm>
#include <functional>
#include <memory>

namespace easy {
class IgnoreSigPipe {
 public:
  IgnoreSigPipe() { ::signal(SIGPIPE, SIG_IGN); }
};

IgnoreSigPipe initObj;

static Logger::ptr logger = EASY_LOG_NAME("system");

IOManager::IOManager(int threadNums, bool use_caller, const std::string& name)
    : Scheduler(threadNums, use_caller, name) {
  epollFd_ = epoll_create1(EPOLL_CLOEXEC);
  EASY_ASSERT(epollFd_ > 0);

  EASY_CHECK(pipe(weakupFds_));

  epoll_event event;
  memset(&event, 0, sizeof(epoll_event));
  event.events = EPOLLIN | EPOLLET;  // epoll lt
  event.data.fd = weakupFds_[0];

  EASY_CHECK(fcntl(weakupFds_[0], F_SETFL, O_NONBLOCK /*| O_CLOEXEC*/));

  EASY_CHECK(epoll_ctl(epollFd_, EPOLL_CTL_ADD, weakupFds_[0], &event));

  event.data.fd = timerfd();

  EASY_CHECK(fcntl(timerfd(), F_SETFL, O_NONBLOCK /*| O_CLOEXEC*/));

  EASY_CHECK(epoll_ctl(epollFd_, EPOLL_CTL_ADD, timerfd(), &event));

  resizeChannels(32);  // speed up
  resizeRevent(32);

  start();  // Scheduler::start
}

IOManager::~IOManager() {
  stop();  // Scheduler::stop

  close(epollFd_);
  close(weakupFds_[0]);
  close(weakupFds_[1]);

  for (size_t i = 0; i < channels_.size(); ++i) {
    if (channels_[i]) {
      delete channels_[i];
    }
  }

  for (size_t i = 0; i < events_.size(); ++i) {
    if (events_[i]) {
      delete events_[i];
    }
  }
}

void IOManager::resizeChannels(size_t size) {
  channels_.resize(size);

  for (size_t i = 0; i < channels_.size(); ++i) {
    if (!channels_[i]) {
      channels_[i] = new Channel;
      channels_[i]->fd_ = static_cast<int>(i);
    }
  }
}

void IOManager::resizeRevent(size_t size) {
  events_.resize(size);

  for (size_t i = 0; i < events_.size(); ++i) {
    if (!events_[i]) {
      events_[i] = new epoll_event;
    }
  }
}

int IOManager::addEvent(int fd, Channel::Event event,
                        std::function<void()> cb) {
  Channel* channel = nullptr;
  {
    size_t idx = static_cast<size_t>(fd);
    ReadLockGuard lock(lock_);
    if (channels_.size() > idx) {
      channel = channels_[idx];
      lock.unlock();
    } else {
      lock.unlock();
      WriteLockGuard l(lock_);
      resizeChannels(idx << 1);  // size * 2
      channel = channels_[idx];
    }
  }

  SpinLockGuard l(channel->lock_);
  if (EASY_UNLIKELY(channel->events_ & event)) {
    // event already exists
    return -1;
  }

  int op = channel->events_ ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
  epoll_event epevent;
  epevent.events =
      EPOLLET | static_cast<EPOLL_EVENTS>(channel->events_ | event);
  epevent.data.ptr = channel;

  if (epoll_ctl(epollFd_, op, fd, &epevent)) {
    EASY_LOG_ERROR(logger) << strerror(errno);
    return -1;
  }

  pendingEventCount_.increment();

  channel->events_ = static_cast<Channel::Event>(channel->events_ | event);

  Channel::EventCtx& ctx = channel->getEventCtx(event);

  EASY_ASSERT(!ctx.scheduler_ && !ctx.fiber_ && !ctx.cb_);

  ctx.scheduler_ = Scheduler::GetThis();
  if (cb) {
    ctx.cb_.swap(cb);
  } else {
    ctx.fiber_ = Fiber::GetThis();
    EASY_ASSERT_MESSAGE(ctx.fiber_->state() == Fiber::EXEC,
                        "state=" << ctx.fiber_->state());
  }
  return 0;
}

bool IOManager::removeEvent(int fd, Channel::Event event) {
  Channel* channel = nullptr;
  {
    size_t idx = static_cast<size_t>(fd);
    ReadLockGuard lock(lock_);
    if (channels_.size() <= idx) {
      return false;
    }
    channel = channels_[idx];
  }

  SpinLockGuard lock(channel->lock_);
  if (EASY_UNLIKELY(!(channel->events_ & event))) {
    // already not exists
    return false;
  }

  Channel::Event new_events =
      static_cast<Channel::Event>(channel->events_ & ~event);
  int option = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events = EPOLLET | static_cast<EPOLL_EVENTS>(new_events);
  epevent.data.ptr = channel;

  if (epoll_ctl(epollFd_, option, fd, &epevent)) {
    EASY_LOG_ERROR(logger) << strerror(errno);
    return false;
  }

  pendingEventCount_.decrement();
  channel->events_ = new_events;
  Channel::EventCtx& ctx = channel->getEventCtx(event);
  channel->cleanUp(ctx);
  return true;
}

bool IOManager::cancelEvent(int fd, Channel::Event event) {
  Channel* channel = nullptr;
  {
    size_t idx = static_cast<size_t>(fd);
    ReadLockGuard lock(lock_);
    if (channels_.size() <= idx) {
      return false;
    }
    channel = channels_[idx];
  }

  SpinLockGuard lock(channel->lock_);
  if (EASY_UNLIKELY(!(channel->events_ & event))) {
    // not exists
    return false;
  }

  Channel::Event new_events =
      static_cast<Channel::Event>(channel->events_ & ~event);
  int option = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events = EPOLLET | static_cast<EPOLL_EVENTS>(new_events);
  epevent.data.ptr = channel;

  if (epoll_ctl(epollFd_, option, fd, &epevent)) {
    EASY_LOG_ERROR(logger) << strerror(errno);
    return false;
  }

  channel->handleEvent(event);
  pendingEventCount_.decrement();
  return true;
}

bool IOManager::cancelAll(int fd) {
  Channel* channel = nullptr;
  {
    size_t idx = static_cast<size_t>(fd);
    ReadLockGuard lock(lock_);
    if (channels_.size() <= idx) {
      return false;
    }
    channel = channels_[idx];
    lock.unlock();
  }

  SpinLockGuard lock(channel->lock_);
  if (!channel->events_) {
    return false;
  }

  int option = EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events = 0;
  epevent.data.ptr = channel;

  if (epoll_ctl(epollFd_, option, fd, &epevent)) {
    EASY_LOG_ERROR(logger) << strerror(errno);
    return false;
  }

  if (channel->events_ & Channel::READ) {
    channel->handleEvent(Channel::READ);
    pendingEventCount_.decrement();
  }
  if (channel->events_ & Channel::WRITE) {
    channel->handleEvent(Channel::WRITE);
    pendingEventCount_.decrement();
  }
  EASY_ASSERT(channel->events_ == 0);
  return true;
}

IOManager* IOManager::GetThis() {
  return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

void IOManager::weakup() {
  if (!hasIdleThread()) {
    return;
  }
  char msg = 'X';
  ssize_t ret = write(weakupFds_[1], &msg, sizeof(msg));
  EASY_ASSERT(ret == 1);
}

bool IOManager::canStop() {
  return !hasTimer() && pendingEventCount_.get() == 0 && Scheduler::canStop();
}

void IOManager::idle() {
  while (EASY_UNLIKELY(!canStop())) {
    int numEvents = 0;
    while (true) {
      numEvents = epoll_wait(epollFd_, *events_.begin(),
                             static_cast<int>(events_.size()), kEPollTimeMs);
      int savedErrno = errno;
      if (numEvents > 0) {
        EASY_LOG_DEBUG(logger) << numEvents << " events happened";
        if (static_cast<size_t>(numEvents) == events_.size()) {
          resizeRevent(events_.size() << 1);
        }
        break;
      } else if (numEvents == 0) {
        EASY_LOG_DEBUG(logger) << "nothing happened";
      } else {
        if (savedErrno != EINTR) {
          errno = savedErrno;
          EASY_LOG_ERROR(logger) << "epoll_wait error";
        }
      }
    };

    for (int i = 0; i < numEvents; ++i) {
      epoll_event* event = events_[static_cast<size_t>(i)];
      if (event->data.fd == weakupFds_[0]) {
        uint8_t dummy[256];
        // EPOLLET
        while (read(weakupFds_[0], dummy, sizeof(dummy)) > 0)
          ;
        continue;
      }

      if (event->data.fd == timerfd()) {
        uint8_t dummy[256];
        // EPOLLET
        while (read(timerfd(), dummy, sizeof(dummy)) > 0)
          ;
        std::vector<std::function<void()>> cbs;
        listExpiredCallback(cbs);
        if (!cbs.empty()) {
          schedule(cbs.begin(), cbs.end());
        }
        continue;
      }

      Channel* channel = static_cast<Channel*>(event->data.ptr);
      SpinLockGuard lock(channel->lock_);
      if (event->events & (EPOLLERR | EPOLLHUP)) {
        event->events |= (EPOLLIN | EPOLLOUT) & channel->events_;
      }
      int revents = Channel::NONE;
      if (event->events & EPOLLIN) {
        revents |= Channel::READ;
      }
      if (event->events & EPOLLOUT) {
        revents |= Channel::WRITE;
      }

      if ((channel->events_ & revents) == Channel::NONE) {
        // do nothing
        continue;
      }

      // TODO: use EPOLLONESHOT
      uint32_t levents = (channel->events_ & ~revents);
      int op = levents ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
      event->events = EPOLLET | levents;

      if (epoll_ctl(epollFd_, op, channel->fd_, event)) {
        EASY_LOG_ERROR(logger)
            << " (" << errno << ") (" << strerror(errno) << ")";
        continue;
      }

      if (revents & Channel::READ) {
        channel->handleEvent(Channel::READ);
        pendingEventCount_.decrement();
      }
      if (revents & Channel::WRITE) {
        channel->handleEvent(Channel::WRITE);
        pendingEventCount_.decrement();
      }
    }
    Fiber::ptr cur = Fiber::GetThis();
    auto raw_ptr = cur.get();
    cur.reset();
    // back at scheduler::handleFiber co->sched_rsume()
    raw_ptr->sched_yield();
  }
}

}  // namespace easy
