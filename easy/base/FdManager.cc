#include "easy/base/FdManager.h"
#include "easy/base/hook.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <fstream>
#include <memory>

namespace easy {
FdCtx::FdCtx(int fd)
    : isInit_(false),
      isSocket_(false),
      sysNonblock_(false),
      userNonblock_(false),
      isClosed_(false),
      fd_(fd),
      recvTimeout_(-1UL),
      sendTimeout_(-1UL) {
  init();
}

FdCtx::~FdCtx() {}

bool FdCtx::init() {
  if (isInit_) {
    return true;
  }

  recvTimeout_ = -1UL;
  sendTimeout_ = -1UL;

  struct stat fd_stat;
  if (fstat(fd_, &fd_stat) == -1) {
    isInit_ = false;
    isSocket_ = false;
  } else {
    isInit_ = true;
    isSocket_ = S_ISSOCK(fd_stat.st_mode);
  }

  if (isSocket_) {
    int flags = fcntl_f(fd_, F_GETFL, 0);
    if (!(flags & O_NONBLOCK)) {
      fcntl_f(fd_, F_SETFL, flags | O_NONBLOCK);
    }
    sysNonblock_ = true;
  } else {
    sysNonblock_ = false;
  }

  userNonblock_ = false;
  isClosed_ = false;
  return isInit_;
}

void FdCtx::setTimeout(int type, uint64_t ms) {
  if (type == SO_RCVTIMEO) {
    recvTimeout_ = ms;
  } else {
    sendTimeout_ = ms;
  }
}

uint64_t FdCtx::getTimeout(int type) {
  return type == SO_RCVTIMEO ? recvTimeout_ : sendTimeout_;
}

FdManager::FdManager() { datas_.resize(64); }

FdCtx::ptr FdManager::getFdCtx(int fd, bool autoCreate) {
  if (fd == -1) {
    return nullptr;
  }
  size_t idx = static_cast<size_t>(fd);
  {
    ReadLockGuard lock(lock_);
    if (datas_.size() <= idx) {
      if (!autoCreate) {
        return nullptr;
      }
    } else {
      if (datas_[idx] || !autoCreate) {
        return datas_[idx];
      }
    }
  }

  WriteLockGuard lock(lock_);
  FdCtx::ptr ctx = std::make_shared<FdCtx>(fd);
  if (idx >= datas_.size()) {
    datas_.resize(idx << 1);  // idx * 2
  }
  datas_[idx] = ctx;
  return ctx;
}

void FdManager::removeFdCtx(int fd) {
  WriteLockGuard lock(lock_);
  size_t idx = static_cast<size_t>(fd);
  if (idx >= datas_.size()) {
    return;
  }
  datas_[idx].reset();
}

}  // namespace easy
