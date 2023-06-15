#ifndef __FDMANAGER_H__
#define __FDMANAGER_H__

#include "easy/base/Mutex.h"
#include "easy/base/Singleton.h"
#include "easy/base/noncopyable.h"

#include <memory>
#include <vector>

namespace easy
{

class FdCtx : noncopyable, public std::enable_shared_from_this<FdCtx>
{
 public:
  typedef std::shared_ptr<FdCtx> ptr;

  FdCtx(int fd);

  ~FdCtx();

  bool init();

  bool isInit() const { return isInit_; }

  bool isSocket() const { return isSocket_; }

  void setSysNonblock(bool flag) { sysNonblock_ = flag; }

  bool isSysNonblock() const { return sysNonblock_; }

  void setUserNonblock(bool flag) { userNonblock_ = flag; }

  bool isUserNonblock() const { return userNonblock_; }

  void setTimeout(int type, uint64_t ms);

  uint64_t getTimeout(int type);

  bool isClosed() const { return isClosed_; }

 private:
  bool isInit_ : 1;
  bool isSocket_ : 1;
  bool sysNonblock_ : 1;
  bool userNonblock_ : 1;
  bool isClosed_ : 1;
  int fd_;
  uint64_t recvTimeout_;
  uint64_t sendTimeout_;
};

class FdManager : noncopyable
{
 public:
  FdManager();
  FdCtx::ptr getFdCtx(int fd, bool autoCreate = false);
  void removeFdCtx(int fd);

 private:
  RWLock lock_;
  std::vector<FdCtx::ptr> datas_;
};

typedef Singleton<FdManager> FdMgr;
}  // namespace easy

#endif
