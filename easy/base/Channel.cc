#include "easy/base/Channel.h"
#include "easy/base/Macro.h"

namespace easy {
Channel::EventCtx& Channel::getEventCtx(Event event) {
  switch (event) {
    case READ:
      return read_;
    case WRITE:
      return write_;
    default:
      EASY_ASSERT_MESSAGE(false, "getContext");
  }
  throw std::invalid_argument("getContext invalid event");
}

void Channel::cleanUp(EventCtx& ctx) {
  ctx.scheduler_ = nullptr;
  ctx.fiber_.reset();
  ctx.cb_ = nullptr;
}

void Channel::handleEvent(Event event) {
  if (EASY_UNLIKELY(!(events_ & event))) {
    return;
  }
  events_ = static_cast<Event>(events_ & ~event);
  EventCtx& ctx = getEventCtx(event);
  if (ctx.cb_) {
    ctx.scheduler_->schedule(std::move(ctx.cb_));
  } else {
    ctx.scheduler_->schedule(std::move(ctx.fiber_));
  }
  EASY_ASSERT(!ctx.cb_ && !ctx.fiber_);  // std::move
  ctx.scheduler_ = nullptr;
}
}  // namespace easy
