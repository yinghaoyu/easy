#include "easy/base/hook.h"
#include "easy/base/Config.h"
#include "easy/base/FdManager.h"
#include "easy/base/IOManager.h"
#include "easy/base/Logger.h"
#include "easy/base/Macro.h"

#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <asm-generic/socket.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

static easy::Logger::ptr logger = EASY_LOG_NAME("system");

namespace easy
{

static ConfigVar<uint64_t>::ptr tcp_connect_timeout =
    Config::Lookup("tcp.connect.timeout", 5000UL, "tcp connect timeout");

static __thread bool t_hook_enable = false;

#define HOOK_FUN(XX) \
  XX(sleep)          \
  XX(usleep)         \
  XX(nanosleep)      \
  XX(socket)         \
  XX(connect)        \
  XX(accept)         \
  XX(read)           \
  XX(readv)          \
  XX(recv)           \
  XX(recvfrom)       \
  XX(recvmsg)        \
  XX(write)          \
  XX(writev)         \
  XX(send)           \
  XX(sendto)         \
  XX(sendmsg)        \
  XX(close)          \
  XX(fcntl)          \
  XX(ioctl)          \
  XX(getsockopt)     \
  XX(setsockopt)

void HookInit()
{
  static bool isInited = false;
  if (isInited)
  {
    return;
  }
#define XX(name) name##_f = (name##_fun) dlsym(RTLD_NEXT, #name);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
  HOOK_FUN(XX);
#pragma GCC diagnostic pop
#undef XX
}

static uint64_t s_connect_timeout = -1UL;

struct __HookIniter
{
  __HookIniter()
  {
    HookInit();
    s_connect_timeout = tcp_connect_timeout->value();
    tcp_connect_timeout->addListener(
        [](const uint64_t &old_val, const uint64_t &new_val)
        {
          EASY_LOG_DEBUG(logger) << "tcp connect timeout changed from "
                                 << old_val << " to " << new_val;
          s_connect_timeout = new_val;
        });
  }
};

static __HookIniter s_hook_initer;

bool IsHookEnable()
{
  return t_hook_enable;
}

void SetHookEnable(bool flag)
{
  t_hook_enable = flag;
}

}  // namespace easy

struct timer_info
{
  bool cancelled = 0;
};

template <typename OriginFunC, typename... Args>
static ssize_t do_io(int fd,
                     OriginFunC func,
                     const char *hook_fun_name,
                     uint32_t event,
                     int timeout_so,
                     Args &&...args)
{
  if (!easy::t_hook_enable)
  {
    return func(fd, std::forward<Args>(args)...);
  }
  easy::FdCtx::ptr ctx = easy::FdMgr::GetInstance()->getFdCtx(fd);
  if (!ctx)
  {
    return func(fd, std::forward<Args>(args)...);
  }
  if (ctx->isClosed())
  {
    errno = EBADF;
    return -1;
  }
  if (!ctx->isSocket() || ctx->isUserNonblock())
  {
    return func(fd, std::forward<Args>(args)...);
  }
  uint64_t ms = ctx->getTimeout(timeout_so);
  auto tinfo = std::make_shared<timer_info>();

  ssize_t n = 0;

retry:
  n = func(fd, std::forward<Args>(args)...);
  if (n == -1 && errno == EINTR)
  {
    goto retry;
  }
  if (n == -1 && errno == EAGAIN)
  {
    easy::IOManager *iom = easy::IOManager::GetThis();
    easy::Timer::ptr timer;
    std::weak_ptr<timer_info> winfo(tinfo);
    if (ms != -1UL)
    {
      timer = iom->addConditionTimer(
          ms,
          [winfo, fd, iom, event]()
          {
            auto t = winfo.lock();
            if (!t || t->cancelled)
            {
              return;
            }
            t->cancelled = ETIMEDOUT;
            iom->cancelEvent(fd, static_cast<easy::Channel::Event>(event));
          },
          winfo);
    }
    int ret = iom->addEvent(fd, static_cast<easy::Channel::Event>(event));
    if (EASY_UNLIKELY(ret))
    {
      EASY_LOG_ERROR(logger)
          << hook_fun_name << " addEvent(" << fd << ", " << event << ")";
      if (timer)
      {
        timer->cancel();
      }
      return -1;
    }
    else
    {
      easy::Coroutine::YieldToHold();
      if (timer)
      {
        timer->cancel();
      }
      if (tinfo->cancelled)
      {
        errno = tinfo->cancelled;
        return -1;
      }
      goto retry;
    }
  }
  return n;
}

extern "C"
{
#define XX(name) name##_fun name##_f = nullptr;
  HOOK_FUN(XX);
#undef XX

  unsigned int sleep(unsigned int seconds)
  {
    if (!easy::t_hook_enable)
    {
      return sleep_f(seconds);
    }

    easy::Coroutine::ptr co = easy::Coroutine::GetThis();
    easy::IOManager *iom = easy::IOManager::GetThis();
    iom->addTimer(seconds * 1000,
                  std::bind(static_cast<void (easy::Scheduler::*)(
                                easy::Coroutine::ptr, int threadId)>(
                                &easy::IOManager::schedule),
                            iom, co, -1));
    easy::Coroutine::YieldToHold();
    return 0;
  }

  int usleep(useconds_t usec)
  {
    if (!easy::t_hook_enable)
    {
      return usleep_f(usec);
    }

    easy::Coroutine::ptr co = easy::Coroutine::GetThis();
    easy::IOManager *iom = easy::IOManager::GetThis();
    iom->addTimer(usec / 1000,
                  std::bind(static_cast<void (easy::Scheduler::*)(
                                easy::Coroutine::ptr, int threadId)>(
                                &easy::IOManager::schedule),
                            iom, co, -1));
    easy::Coroutine::YieldToHold();
    return 0;
  }

  int nanosleep(const struct timespec *req, struct timespec *rem)
  {
    if (!easy::t_hook_enable)
    {
      return nanosleep_f(req, rem);
    }

    uint64_t timeout_ms =
        static_cast<uint64_t>(req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000);
    easy::Coroutine::ptr co = easy::Coroutine::GetThis();
    easy::IOManager *iom = easy::IOManager::GetThis();
    iom->addTimer(timeout_ms,
                  std::bind(static_cast<void (easy::Scheduler::*)(
                                easy::Coroutine::ptr, int threadId)>(
                                &easy::IOManager::schedule),
                            iom, co, -1));
    easy::Coroutine::YieldToHold();
    return 0;
  }

  int socket(int domain, int type, int protocol)
  {
    if (!easy::t_hook_enable)
    {
      return socket_f(domain, type, protocol);
    }
    int fd = socket_f(domain, type, protocol);
    if (fd == -1)
    {
      return fd;
    }
    easy::FdMgr::GetInstance()->getFdCtx(fd, true);
    return fd;
  }

  int connect_with_timeout(int fd,
                           const struct sockaddr *addr,
                           socklen_t addrlen,
                           uint64_t timeout_ms)
  {
    if (!easy::t_hook_enable)
    {
      struct timeval tv
      {
        .tv_sec = static_cast<long int>(timeout_ms / 1000),
        .tv_usec = static_cast<long int>(timeout_ms % 1000 * 1000)
      };
      setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
      return connect_f(fd, addr, addrlen);
    }
    easy::FdCtx::ptr ctx = easy::FdMgr::GetInstance()->getFdCtx(fd);
    if (!ctx || ctx->isClosed())
    {
      errno = EBADF;
      return -1;
    }
    if (!ctx->isSocket())
    {
      return connect_f(fd, addr, addrlen);
    }
    if (ctx->isUserNonblock())
    {
      return connect_f(fd, addr, addrlen);
    }
    int n = connect_f(fd, addr, addrlen);
    if (n == 0)
    {
      // already connect
      return 0;
    }
    else if (n != -1 || errno != EINPROGRESS)
    {
      // error happened
      return n;
    }
    // n == -1 && errno == EINPROGRESS
    easy::IOManager *iom = easy::IOManager::GetThis();
    easy::Timer::ptr timer;
    auto tinfo = std::make_shared<timer_info>();
    std::weak_ptr<timer_info> winfo(tinfo);
    if (timeout_ms != -1UL)
    {
      timer = iom->addConditionTimer(
          timeout_ms,
          [winfo, fd, iom]()
          {
            auto t = winfo.lock();
            if (!t || t->cancelled)
            {
              return;
            }
            t->cancelled = ETIMEDOUT;
            iom->cancelEvent(fd, easy::Channel::WRITE);
          },
          winfo);
    }

    int ret = iom->addEvent(fd, easy::Channel::WRITE);

    if (EASY_UNLIKELY(ret))
    {
      // add event failed
      if (timer)
      {
        timer->cancel();
      }
      EASY_LOG_ERROR(logger) << "connect addEvent(" << fd << ", WRITE) error";
    }
    else
    {
      easy::Coroutine::YieldToHold();
      if (timer)
      {
        timer->cancel();
      }
      if (tinfo->cancelled)
      {
        errno = tinfo->cancelled;
        return -1;
      }
    }
    // 1. connect error, socketfd can be WRITE and READ
    // 2. connect success, socketfd can be WRITE
    // 3. connect success, sockfd can be WRITE and READ
    // so we should be concerned about WRITE event, by getsockopt to get status
    int error = 0;
    socklen_t len = sizeof(int);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) == -1)
    {
      return -1;
    }
    if (error == 0)
    {
      return 0;
    }
    else
    {
      errno = error;
      return -1;
    }
  }

  int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
  {
    return connect_with_timeout(sockfd, addr, addrlen, easy::s_connect_timeout);
  }

  int accept(int s, struct sockaddr *addr, socklen_t *addrlen)
  {
    ssize_t n = do_io(s, accept_f, "accept", easy::Channel::READ, SO_RCVTIMEO,
                      addr, addrlen);
    int fd = static_cast<int>(n);
    if (fd >= 0)
    {
      easy::FdMgr::GetInstance()->getFdCtx(fd, true);
    }
    return fd;
  }

  ssize_t read(int fd, void *buf, size_t count)
  {
    return do_io(fd, read_f, "read", easy::Channel::READ, SO_RCVTIMEO, buf,
                 count);
  }

  ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
  {
    return do_io(fd, readv_f, "readv", easy::Channel::READ, SO_RCVTIMEO, iov,
                 iovcnt);
  }

  ssize_t recv(int sockfd, void *buf, size_t len, int flags)
  {
    return do_io(sockfd, recv_f, "recv", easy::Channel::READ, SO_RCVTIMEO, buf,
                 len, flags);
  }

  ssize_t recvfrom(int sockfd,
                   void *buf,
                   size_t len,
                   int flags,
                   struct sockaddr *src_addr,
                   socklen_t *addrlen)
  {
    return do_io(sockfd, recvfrom_f, "recvfrom", easy::Channel::READ,
                 SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
  }

  ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
  {
    return do_io(sockfd, recvmsg_f, "recvmsg", easy::Channel::READ, SO_RCVTIMEO,
                 msg, flags);
  }

  ssize_t write(int fd, const void *buf, size_t count)
  {
    return do_io(fd, write_f, "write", easy::Channel::WRITE, SO_SNDTIMEO, buf,
                 count);
  }

  ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
  {
    return do_io(fd, writev_f, "writev", easy::Channel::WRITE, SO_SNDTIMEO, iov,
                 iovcnt);
  }

  ssize_t send(int s, const void *msg, size_t len, int flags)
  {
    return do_io(s, send_f, "send", easy::Channel::WRITE, SO_SNDTIMEO, msg, len,
                 flags);
  }
  ssize_t sendto(int s,
                 const void *msg,
                 size_t len,
                 int flags,
                 const struct sockaddr *to,
                 socklen_t tolen)
  {
    return do_io(s, sendto_f, "sendto", easy::Channel::WRITE, SO_SNDTIMEO, msg,
                 len, flags, to, tolen);
  }

  ssize_t sendmsg(int s, const struct msghdr *msg, int flags)
  {
    return do_io(s, sendmsg_f, "sendmsg", easy::Channel::WRITE, SO_SNDTIMEO,
                 msg, flags);
  }

  int close(int fd)
  {
    if (!easy::t_hook_enable)
    {
      return close_f(fd);
    }
    easy::FdCtx::ptr ctx = easy::FdMgr::GetInstance()->getFdCtx(fd);
    if (ctx)
    {
      auto iom = easy::IOManager::GetThis();
      if (iom)
      {
        iom->cancelAll(fd);
      }
      easy::FdMgr::GetInstance()->removeFdCtx(fd);
    }
    return close_f(fd);
  }

  int fcntl(int fd, int cmd, ... /* arg */)
  {
    va_list va;
    va_start(va, cmd);
    switch (cmd)
    {
    case F_SETFL:
    {
      int arg = va_arg(va, int);
      va_end(va);
      easy::FdCtx::ptr ctx = easy::FdMgr::GetInstance()->getFdCtx(fd);
      if (!ctx || ctx->isClosed() || !ctx->isSocket())
      {
        return fcntl_f(fd, cmd, arg);
      }
      ctx->setUserNonblock(arg & O_NONBLOCK);
      if (ctx->isSysNonblock())
      {
        arg |= O_NONBLOCK;
      }
      else
      {
        arg &= ~O_NONBLOCK;
      }
      return fcntl_f(fd, cmd, arg);
    }
    break;
    case F_GETFL:
    {
      va_end(va);
      int arg = fcntl_f(fd, cmd);
      easy::FdCtx::ptr ctx = easy::FdMgr::GetInstance()->getFdCtx(fd);
      if (!ctx || ctx->isClosed() || !ctx->isSocket())
      {
        return arg;
      }
      if (ctx->isUserNonblock())
      {
        return arg | O_NONBLOCK;
      }
      else
      {
        return arg & ~O_NONBLOCK;
      }
    }
    break;
    case F_DUPFD:
    case F_DUPFD_CLOEXEC:
    case F_SETFD:
    case F_SETOWN:
    case F_SETSIG:
    case F_SETLEASE:
    case F_NOTIFY:
#ifdef F_SETPIPE_SZ
    case F_SETPIPE_SZ:
#endif
    {
      int arg = va_arg(va, int);
      va_end(va);
      return fcntl_f(fd, cmd, arg);
    }
    break;
    case F_GETFD:
    case F_GETOWN:
    case F_GETSIG:
    case F_GETLEASE:
#ifdef F_GETPIPE_SZ
    case F_GETPIPE_SZ:
#endif
    {
      va_end(va);
      return fcntl_f(fd, cmd);
    }
    break;
    case F_SETLK:
    case F_SETLKW:
    case F_GETLK:
    {
      struct flock *arg = va_arg(va, struct flock *);
      va_end(va);
      return fcntl_f(fd, cmd, arg);
    }
    break;
    case F_GETOWN_EX:
    case F_SETOWN_EX:
    {
      struct f_owner_exlock *arg = va_arg(va, struct f_owner_exlock *);
      va_end(va);
      return fcntl_f(fd, cmd, arg);
    }
    break;
    default:
      va_end(va);
      return fcntl_f(fd, cmd);
    }
  }

  int ioctl(int d, unsigned long int request, ...)
  {
    va_list va;
    va_start(va, request);
    void *arg = va_arg(va, void *);
    va_end(va);

    if (FIONBIO == request)
    {
      bool user_nonblock = !!*static_cast<int *>(arg);
      easy::FdCtx::ptr ctx = easy::FdMgr::GetInstance()->getFdCtx(d);
      if (!ctx || ctx->isClosed() || !ctx->isSocket())
      {
        return ioctl_f(d, request, arg);
      }
      ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(d, request, arg);
  }

  int getsockopt(int sockfd,
                 int level,
                 int optname,
                 void *optval,
                 socklen_t *optlen)
  {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
  }

  int setsockopt(int sockfd,
                 int level,
                 int optname,
                 const void *optval,
                 socklen_t optlen)
  {
    if (!easy::t_hook_enable)
    {
      return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    if (level == SOL_SOCKET)
    {
      if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)
      {
        easy::FdCtx::ptr ctx = easy::FdMgr::GetInstance()->getFdCtx(sockfd);
        if (ctx)
        {
          const timeval *v = static_cast<const timeval *>(optval);
          ctx->setTimeout(optname, static_cast<uint64_t>(v->tv_sec * 1000 +
                                                         v->tv_usec / 1000));
        }
      }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
  }
}
