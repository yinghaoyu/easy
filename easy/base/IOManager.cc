#include "easy/base/IOManager.h"
#include "easy/base/Logger.h"
#include "easy/base/Mutex.h"
#include "easy/base/Scheduler.h"
#include "easy/base/easy_define.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <functional>
#include <memory>

namespace easy
{
class IgnoreSigPipe
{
 public:
  IgnoreSigPipe() { ::signal(SIGPIPE, SIG_IGN); }
};

IgnoreSigPipe initObj;

static Logger::ptr logger = EASY_LOG_NAME("system");

IOManager::FdContext::EventContext &IOManager::FdContext::eventContext(
    Event event)
{
  switch (event)
  {
  case IOManager::READ:
    return read_;
  case IOManager::WRITE:
    return write_;
  default:
    EASY_ASSERT_MESSAGE(false, "getContext");
  }
  throw std::invalid_argument("getContext invalid event");
}

void IOManager::FdContext::resetContext(EventContext &ctx)
{
  ctx.scheduler_ = nullptr;
  ctx.co_.reset();
  ctx.cb_ = nullptr;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event)
{
  if (EASY_UNLIKELY(!(events_ & event)))
  {
    return;
  }
  events_ = static_cast<Event>(events_ & ~event);
  EventContext &ctx = eventContext(event);
  if (ctx.cb_)
  {
    ctx.scheduler_->schedule(std::move(ctx.cb_));
  }
  else
  {
    ctx.scheduler_->schedule(std::move(ctx.co_));
  }
  EASY_ASSERT(!ctx.cb_ && !ctx.co_);  // std::move
  ctx.scheduler_ = nullptr;
}

IOManager::IOManager(int threadNums, bool use_caller, const std::string &name)
    : Scheduler(threadNums, use_caller, name)
{
  epollFd_ = epoll_create(5000);
  EASY_ASSERT(epollFd_ > 0);

  EASY_CHECK(pipe(tickleFds_));

  epoll_event event;
  memset(&event, 0, sizeof(epoll_event));
  event.events = EPOLLIN | EPOLLET;  // level trigger
  event.data.fd = tickleFds_[0];

  EASY_CHECK(fcntl(tickleFds_[0], F_SETFL, O_NONBLOCK));

  EASY_CHECK(epoll_ctl(epollFd_, EPOLL_CTL_ADD, tickleFds_[0], &event));

  contextResize(32);  // avoid allocate memory during run

  start();  // Scheduler::start
}

IOManager::~IOManager()
{
  stop();  // Scheduler::stop

  close(epollFd_);
  close(tickleFds_[0]);
  close(tickleFds_[1]);

  for (size_t i = 0; i < fdContexts_.size(); ++i)
  {
    if (fdContexts_[i])
    {
      delete fdContexts_[i];
    }
  }
}

void IOManager::contextResize(size_t size)
{
  fdContexts_.resize(size);

  for (size_t i = 0; i < fdContexts_.size(); ++i)
  {
    if (!fdContexts_[i])
    {
      fdContexts_[i] = new FdContext;
      fdContexts_[i]->fd_ = static_cast<int>(i);
    }
  }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
{
  FdContext *fd_ctx = nullptr;
  ReadLockGuard lock(lock_);
  if (fdContexts_.size() > static_cast<size_t>(fd))
  {
    fd_ctx = fdContexts_[static_cast<size_t>(fd)];
    lock.unlock();
  }
  else
  {
    lock.unlock();
    WriteLockGuard l(lock_);
    contextResize(static_cast<size_t>(fd * 2));
    fd_ctx = fdContexts_[static_cast<size_t>(fd)];
  }

  SpinLockGuard l(fd_ctx->lock_);
  if (EASY_UNLIKELY(fd_ctx->events_ & event))
  {
    // event already exists
    return -1;
  }

  int op = fd_ctx->events_ ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
  epoll_event epevent;
  epevent.events = EPOLLET | static_cast<EPOLL_EVENTS>(fd_ctx->events_ | event);
  epevent.data.ptr = fd_ctx;

  int ret = epoll_ctl(epollFd_, op, fd, &epevent);

  if (ret)
  {
    EASY_LOG_ERROR(logger) << strerror(errno);
    return -1;
  }

  pendingEventCount_.increment();

  fd_ctx->events_ = static_cast<Event>(fd_ctx->events_ | event);

  FdContext::EventContext &event_ctx = fd_ctx->eventContext(event);

  EASY_ASSERT(!event_ctx.scheduler_ && !event_ctx.co_ && !event_ctx.cb_);

  event_ctx.scheduler_ = Scheduler::GetThis();
  if (cb)
  {
    event_ctx.cb_.swap(cb);
  }
  else
  {
    event_ctx.co_ = Coroutine::GetThis();
    EASY_ASSERT_MESSAGE(event_ctx.co_->state() == Coroutine::EXEC,
                        "state=" << event_ctx.co_->state());
  }
  return 0;
}

bool IOManager::removeEvent(int fd, Event event)
{
  FdContext *fd_ctx = nullptr;
  {
    ReadLockGuard lock(lock_);
    if (fdContexts_.size() <= static_cast<size_t>(fd))
    {
      return false;
    }
    fd_ctx = fdContexts_[static_cast<size_t>(fd)];
  }

  SpinLockGuard lock(fd_ctx->lock_);
  if (EASY_UNLIKELY(!(fd_ctx->events_ & event)))
  {
    // already not exists
    return false;
  }

  Event new_events = static_cast<Event>(fd_ctx->events_ & ~event);
  int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events = EPOLLET | static_cast<EPOLL_EVENTS>(new_events);
  epevent.data.ptr = fd_ctx;

  int ret = epoll_ctl(epollFd_, op, fd, &epevent);
  if (ret)
  {
    EASY_LOG_ERROR(logger) << strerror(errno);
    return false;
  }

  pendingEventCount_.decrement();
  fd_ctx->events_ = new_events;
  FdContext::EventContext &event_ctx = fd_ctx->eventContext(event);
  fd_ctx->resetContext(event_ctx);
  return true;
}

bool IOManager::cancelEvent(int fd, Event event)
{
  FdContext *fd_ctx = nullptr;
  {
    ReadLockGuard lock(lock_);
    if (fdContexts_.size() <= static_cast<size_t>(fd))
    {
      return false;
    }
    fd_ctx = fdContexts_[static_cast<size_t>(fd)];
  }

  SpinLockGuard lock(fd_ctx->lock_);
  if (EASY_UNLIKELY(!(fd_ctx->events_ & event)))
  {
    // not exists
    return false;
  }

  Event new_events = static_cast<Event>(fd_ctx->events_ & ~event);
  int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events = EPOLLET | static_cast<EPOLL_EVENTS>(new_events);
  epevent.data.ptr = fd_ctx;

  int ret = epoll_ctl(epollFd_, op, fd, &epevent);
  if (ret)
  {
    EASY_LOG_ERROR(logger) << strerror(errno);
    return false;
  }

  fd_ctx->triggerEvent(event);
  pendingEventCount_.decrement();
  return true;
}

bool IOManager::cancelAll(int fd)
{
  FdContext *fd_ctx = nullptr;
  {
    ReadLockGuard lock(lock_);
    if (fdContexts_.size() <= static_cast<size_t>(fd))
    {
      return false;
    }
    fd_ctx = fdContexts_[static_cast<size_t>(fd)];
    lock.unlock();
  }

  SpinLockGuard lock(fd_ctx->lock_);
  if (!fd_ctx->events_)
  {
    return false;
  }

  int op = EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events = 0;
  epevent.data.ptr = fd_ctx;

  int ret = epoll_ctl(epollFd_, op, fd, &epevent);
  if (ret)
  {
    EASY_LOG_ERROR(logger) << strerror(errno);
    return false;
  }

  if (fd_ctx->events_ & READ)
  {
    fd_ctx->triggerEvent(READ);
    pendingEventCount_.decrement();
  }
  if (fd_ctx->events_ & WRITE)
  {
    fd_ctx->triggerEvent(WRITE);
    pendingEventCount_.decrement();
  }
  EASY_ASSERT(fd_ctx->events_ == 0);
  return true;
}

IOManager *IOManager::GetThis()
{
  return dynamic_cast<IOManager *>(Scheduler::GetThis());
}

void IOManager::tickle()
{
  if (!hasIdleThread())
  {
    return;
  }
  ssize_t ret = write(tickleFds_[1], "T", 1UL);
  EASY_ASSERT(ret == 1);
}

bool IOManager::stopping(int64_t &timeout)
{
  timeout = getNextTimer();
  return timeout == ~0 && pendingEventCount_.get() == 0 && Scheduler::canStop();
}

bool IOManager::canStop()
{
  int64_t timeout = 0;
  return stopping(timeout);
}

void IOManager::idle()
{
  const uint64_t MAX_EVENTS = 256;
  epoll_event *events = new epoll_event[MAX_EVENTS]();
  std::shared_ptr<epoll_event> shared_events(
      events, [](epoll_event *ptr) { delete[] ptr; });

  while (true)
  {
    int64_t next_timeout = 0;
    if (EASY_UNLIKELY(stopping(next_timeout)))
    {
      break;
    }

    int eventNums = 0;
    do
    {
      static const int MAX_TIMEOUT = 3000;
      next_timeout = (next_timeout > MAX_TIMEOUT || next_timeout < 0)
                         ? MAX_TIMEOUT
                         : next_timeout;

      eventNums = epoll_wait(epollFd_, events, MAX_EVENTS,
                             static_cast<int>(next_timeout));
      if (eventNums < 0 && errno == EINTR)
      {
      }
      else
      {
        // event reach
        break;
      }
    } while (true);

    std::vector<std::function<void()>> cbs;
    listExpiredCallback(cbs);
    if (!cbs.empty())
    {
      schedule(cbs.begin(), cbs.end());
    }

    for (int i = 0; i < eventNums; ++i)
    {
      epoll_event &event = events[i];
      if (event.data.fd == tickleFds_[0])
      {
        uint8_t dummy[256];
        // EPOLLET
        while (read(tickleFds_[0], dummy, sizeof(dummy)) > 0)
          ;
        continue;
      }

      FdContext *fd_ctx = static_cast<FdContext *>(event.data.ptr);
      SpinLockGuard lock(fd_ctx->lock_);
      if (event.events & (EPOLLERR | EPOLLHUP))
      {
        event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events_;
      }
      int real_events = NONE;
      if (event.events & EPOLLIN)
      {
        real_events |= READ;
      }
      if (event.events & EPOLLOUT)
      {
        real_events |= WRITE;
      }

      if ((fd_ctx->events_ & real_events) == NONE)
      {
        // do nothing
        continue;
      }

      // TODO: use EPOLLONESHOT
      uint32_t left_events = (fd_ctx->events_ & ~real_events);
      int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
      event.events = EPOLLET | left_events;

      int ret = epoll_ctl(epollFd_, op, fd_ctx->fd_, &event);
      if (ret)
      {
        EASY_LOG_ERROR(logger)
            << " (" << errno << ") (" << strerror(errno) << ")";
        continue;
      }

      if (real_events & READ)
      {
        fd_ctx->triggerEvent(READ);
        pendingEventCount_.decrement();
      }
      if (real_events & WRITE)
      {
        fd_ctx->triggerEvent(WRITE);
        pendingEventCount_.decrement();
      }
    }
    Coroutine::ptr cur = Coroutine::GetThis();
    auto raw_ptr = cur.get();
    cur.reset();
    // back at scheduler::handleCoroutine co->sched_rsume()
    raw_ptr->sched_yield();
  }
}

void IOManager::onTimerInsertedAtFront()
{
  // should update epoll timeout
  tickle();
}

}  // namespace easy
