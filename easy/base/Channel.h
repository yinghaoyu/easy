#ifndef __EASY_CHANNEL_H__
#define __EASY_CHANNEL_H__

#include "easy/base/Fiber.h"
#include "easy/base/Mutex.h"
#include "easy/base/Scheduler.h"
#include "easy/base/noncopyable.h"

#include <sys/epoll.h>

namespace easy
{
class Channel : noncopyable
{
  public:
    enum Event
    {
        NONE  = 0x00,
        READ  = EPOLLIN,
        WRITE = EPOLLOUT,
    };

    static const int kNoneEvent  = 0x00;
    static const int kReadEvent  = EPOLLIN /*| EPOLLPRI*/;
    static const int kWriteEvent = EPOLLOUT;

    struct EventCtx
    {
        Scheduler*            scheduler_ = nullptr;
        Fiber::ptr            fiber_;
        std::function<void()> cb_;
    };

    EventCtx& getEventCtx(Event event);

    void cleanUp(EventCtx& ctx);

    void handleEvent(Event event);

    EventCtx read_;
    EventCtx write_;
    int      fd_;
    Event    events_ = NONE;
    SpinLock lock_;
};

}  // namespace easy
#endif
