#include "easy/net/Socket.h"
#include "easy/base/Channel.h"
#include "easy/base/FdManager.h"
#include "easy/base/FileUtil.h"
#include "easy/base/IOManager.h"
#include "easy/base/Logger.h"
#include "easy/base/Macro.h"
#include "easy/base/hook.h"

#include <netinet/tcp.h>

namespace easy
{
static Logger::ptr logger = ELOG_NAME("system");

Socket::ptr Socket::CreateTCP(Address::ptr address) { return std::make_shared<Socket>(address->family(), TCP, 0); }

Socket::ptr Socket::CreateUDP(Address::ptr address)
{
    Socket::ptr sock = std::make_shared<Socket>(address->family(), UDP, 0);
    sock->newSock();
    sock->isConnected_ = true;
    return sock;
}

Socket::ptr Socket::CreateTCPSocket() { return std::make_shared<Socket>(IPv4, TCP, 0); }

Socket::ptr Socket::CreateUDPSocket()
{
    Socket::ptr sock = std::make_shared<Socket>(IPv4, UDP, 0);
    sock->newSock();
    sock->isConnected_ = true;
    return sock;
}

Socket::ptr Socket::CreateTCPSocket6() { return std::make_shared<Socket>(IPv6, TCP, 0); }

Socket::ptr Socket::CreateUDPSocket6()
{
    Socket::ptr sock = std::make_shared<Socket>(IPv6, UDP, 0);
    sock->newSock();
    sock->isConnected_ = true;
    return sock;
}

Socket::ptr Socket::CreateUnixTCPSocket() { return std::make_shared<Socket>(UNIX, TCP, 0); }

Socket::ptr Socket::CreateUnixUDPSocket() { return std::make_shared<Socket>(UNIX, UDP, 0); }

Socket::Socket(int family, int type, int protocol) : fd_(-1), family_(family), type_(type), protocol_(protocol), isConnected_(false) {}

Socket::~Socket() { close(); }

bool Socket::checkConnected()
{
    struct tcp_info info;
    int             len = sizeof(info);
    getsockopt(fd_, IPPROTO_TCP, TCP_INFO, &info, reinterpret_cast<socklen_t*>(&len));
    isConnected_ = (info.tcpi_state == TCP_ESTABLISHED);
    return isConnected_;
}

int64_t Socket::getSendTimeout()
{
    FdCtx::ptr ctx = FdMgr::GetInstance()->getFdCtx(fd_);
    if (ctx)
    {
        return static_cast<int64_t>(ctx->getTimeout(SO_SNDTIMEO));
    }
    return -1;
}

void Socket::setSendTimeout(int64_t v)
{
    struct timeval tv
    {
        .tv_sec = static_cast<long int>(v / 1000), .tv_usec = static_cast<long int>(v % 1000 * 1000)
    };
    setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}

int64_t Socket::getRecvTimeout()
{
    FdCtx::ptr ctx = FdMgr::GetInstance()->getFdCtx(fd_);
    if (ctx)
    {
        return static_cast<int64_t>(ctx->getTimeout(SO_RCVTIMEO));
    }
    return -1;
}

void Socket::setRecvTimeout(int64_t v)
{
    struct timeval tv
    {
        .tv_sec = static_cast<long int>(v / 1000), .tv_usec = static_cast<long int>(v % 1000 * 1000)
    };
    setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}

bool Socket::getOption(int level, int option, void* result, socklen_t* len)
{
    if (getsockopt(fd_, level, option, result, static_cast<socklen_t*>(len)))
    {
        ELOG_DEBUG(logger) << "getOption sock=" << fd_ << " level=" << level << " option=" << option << " errno=" << errno
                           << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

bool Socket::setOption(int level, int option, const void* result, socklen_t len)
{
    if (setsockopt(fd_, level, option, result, static_cast<socklen_t>(len)))
    {
        ELOG_DEBUG(logger) << "setOption sock=" << fd_ << " level=" << level << " option=" << option << " errno=" << errno
                           << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

Socket::ptr Socket::accept()
{
    Socket::ptr sock  = std::make_shared<Socket>(family_, type_, protocol_);
    int         newFd = ::accept(fd_, nullptr, nullptr);
    if (newFd == -1)
    {
        ELOG_ERROR(logger) << "accept(" << fd_ << ") errno=" << errno << " errstr=" << strerror(errno);
        return nullptr;
    }
    if (sock->init(newFd))
    {
        return sock;
    }
    return nullptr;
}

bool Socket::init(int sock)
{
    FdCtx::ptr ctx = FdMgr::GetInstance()->getFdCtx(sock);
    if (ctx && ctx->isSocket() && !ctx->isClosed())
    {
        fd_          = sock;
        isConnected_ = true;
        initSock();
        getLocalAddress();
        getRemoteAddress();
        return true;
    }
    return false;
}

bool Socket::bind(const Address::ptr addr)
{
    // localAddress_ = addr;
    if (!isValid())
    {
        newSock();
        if (EASY_UNLIKELY(!isValid()))
        {
            return false;
        }
    }

    if (EASY_UNLIKELY(addr->family() != family_))
    {
        ELOG_ERROR(logger) << "bind sock.family(" << family_ << ") addr.family(" << addr->family() << ") not equal, addr=" << addr->toString();
        return false;
    }

    UnixAddress::ptr uaddr = std::dynamic_pointer_cast<UnixAddress>(addr);
    if (uaddr)
    {
        Socket::ptr sock = Socket::CreateUnixTCPSocket();
        if (sock->connect(uaddr))
        {
            return false;
        }
        else
        {
            FileUtil::Unlink(uaddr->path(), true);
        }
    }

    if (::bind(fd_, addr->addr(), addr->addrLen()))
    {
        ELOG_ERROR(logger) << "bind error errrno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    getLocalAddress();
    return true;
}

bool Socket::reconnect(uint64_t timeout_ms)
{
    if (!remoteAddress_)
    {
        ELOG_ERROR(logger) << "reconnect remoteAddress_ is null";
        return false;
    }
    localAddress_.reset();
    return connect(remoteAddress_, timeout_ms);
}

bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms)
{
    remoteAddress_ = addr;
    if (!isValid())
    {
        newSock();
        if (EASY_UNLIKELY(!isValid()))
        {
            return false;
        }
    }

    if (EASY_UNLIKELY(addr->family() != family_))
    {
        ELOG_ERROR(logger) << "connect sock.family(" << family_ << ") addr.family(" << addr->family() << ") not equal, addr=" << addr->toString();
        return false;
    }

    if (timeout_ms == -1UL)
    {
        if (::connect(fd_, addr->addr(), addr->addrLen()))
        {
            ELOG_ERROR(logger) << "sock=" << fd_ << " connect(" << addr->toString() << ") error errno=" << errno << " errstr=" << strerror(errno);
            close();
            return false;
        }
    }
    else
    {
        if (::connect_with_timeout(fd_, addr->addr(), addr->addrLen(), timeout_ms))
        {
            ELOG_ERROR(logger) << "sock=" << fd_ << " connect(" << addr->toString() << ") timeout=" << timeout_ms << " error errno=" << errno
                               << " errstr=" << strerror(errno);
            close();
            return false;
        }
    }
    isConnected_ = true;
    getRemoteAddress();
    getLocalAddress();
    return true;
}

bool Socket::listen(int backlog)
{
    if (!isValid())
    {
        ELOG_ERROR(logger) << "listen error sock=-1";
        return false;
    }
    if (::listen(fd_, backlog))
    {
        ELOG_ERROR(logger) << "listen error errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

bool Socket::close()
{
    if (!isConnected_ && fd_ == -1)
    {
        return true;
    }
    isConnected_ = false;
    if (fd_ != -1)
    {
        ::close(fd_);
        fd_ = -1;
    }
    return false;
}

int Socket::send(const void* buffer, size_t length, int flags)
{
    if (isConnected())
    {
        return static_cast<int>(::send(fd_, buffer, length, flags));
    }
    return -1;
}

int Socket::send(const iovec* buffers, size_t length, int flags)
{
    if (isConnected())
    {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov    = const_cast<iovec*>(buffers);
        msg.msg_iovlen = length;
        return static_cast<int>(::sendmsg(fd_, &msg, flags));
    }
    return -1;
}

int Socket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags)
{
    if (isConnected())
    {
        return static_cast<int>(::sendto(fd_, buffer, length, flags, to->addr(), to->addrLen()));
    }
    return -1;
}

int Socket::sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags)
{
    if (isConnected())
    {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov     = const_cast<iovec*>(buffers);
        msg.msg_iovlen  = length;
        msg.msg_name    = to->addr();
        msg.msg_namelen = to->addrLen();
        return static_cast<int>(::sendmsg(fd_, &msg, flags));
    }
    return -1;
}

int Socket::recv(void* buffer, size_t length, int flags)
{
    if (isConnected())
    {
        return static_cast<int>(::recv(fd_, buffer, length, flags));
    }
    return -1;
}

int Socket::recv(iovec* buffers, size_t length, int flags)
{
    if (isConnected())
    {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov    = buffers;
        msg.msg_iovlen = length;
        return static_cast<int>(::recvmsg(fd_, &msg, flags));
    }
    return -1;
}

int Socket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags)
{
    if (isConnected())
    {
        socklen_t len = from->addrLen();
        return static_cast<int>(::recvfrom(fd_, buffer, length, flags, from->addr(), &len));
    }
    return -1;
}

int Socket::recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags)
{
    if (isConnected())
    {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov     = static_cast<iovec*>(buffers);
        msg.msg_iovlen  = length;
        msg.msg_name    = from->addr();
        msg.msg_namelen = from->addrLen();
        return static_cast<int>(::recvmsg(fd_, &msg, flags));
    }
    return -1;
}

Address::ptr Socket::getRemoteAddress()
{
    if (remoteAddress_)
    {
        return remoteAddress_;
    }

    Address::ptr result;
    switch (family_)
    {
        case AF_INET: result = std::make_shared<IPv4Address>(); break;
        case AF_INET6: result = std::make_shared<IPv6Address>(); break;
        case AF_UNIX: result = std::make_shared<UnixAddress>(); break;
        default: result = std::make_shared<UnknownAddress>(family_); break;
    }
    socklen_t addrlen = result->addrLen();
    if (getpeername(fd_, result->addr(), &addrlen))
    {
        // ELOG_ERROR(logger)
        //     << "getpeername error sock=" << sock_ << " errno=" << errno
        //     << " errstr=" << strerror(errno);
        return std::make_shared<UnknownAddress>(family_);
    }
    if (family_ == AF_UNIX)
    {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    remoteAddress_ = result;
    return remoteAddress_;
}

Address::ptr Socket::getLocalAddress()
{
    if (localAddress_)
    {
        return localAddress_;
    }

    Address::ptr result;
    switch (family_)
    {
        case AF_INET: result = std::make_shared<IPv4Address>(); break;
        case AF_INET6: result = std::make_shared<IPv6Address>(); break;
        case AF_UNIX: result = std::make_shared<UnixAddress>(); break;
        default: result = std::make_shared<UnknownAddress>(family_); break;
    }
    socklen_t addrlen = result->addrLen();
    if (getsockname(fd_, result->addr(), &addrlen))
    {
        ELOG_ERROR(logger) << "getsockname error sock=" << fd_ << " errno=" << errno << " errstr=" << strerror(errno);
        return std::make_shared<UnknownAddress>(family_);
    }
    if (family_ == AF_UNIX)
    {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    localAddress_ = result;
    return localAddress_;
}

bool Socket::isValid() const { return fd_ != -1; }

int Socket::getError()
{
    int       error = 0;
    socklen_t len   = sizeof(error);
    if (!getOption(SOL_SOCKET, SO_ERROR, &error, &len))
    {
        error = errno;
    }
    return error;
}

std::ostream& Socket::dump(std::ostream& os) const
{
    os << "[Socket fd=" << fd_ << " connected=" << isConnected_ << " family=" << family_ << " type=" << type_ << " protocol=" << protocol_;
    if (localAddress_)
    {
        os << " local=" << localAddress_->toString();
    }
    if (remoteAddress_)
    {
        os << " remote=" << remoteAddress_->toString();
    }
    os << "]";
    return os;
}

std::string Socket::toString() const
{
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

bool Socket::cancelRead() { return IOManager::GetThis()->cancelEvent(fd_, Channel::READ); }

bool Socket::cancelWrite() { return IOManager::GetThis()->cancelEvent(fd_, Channel::WRITE); }

bool Socket::cancelAccept() { return IOManager::GetThis()->cancelEvent(fd_, Channel::READ); }

bool Socket::cancelAll() { return IOManager::GetThis()->cancelAll(fd_); }

void Socket::initSock()
{
    int val = 1;
    setOption(SOL_SOCKET, SO_REUSEADDR, val);
    if (type_ == SOCK_STREAM)
    {
        setOption(IPPROTO_TCP, TCP_NODELAY, val);
    }
}

void Socket::newSock()
{
    fd_ = ::socket(family_, type_, protocol_);
    if (EASY_LIKELY(fd_ != -1))
    {
        initSock();
    }
    else
    {
        ELOG_ERROR(logger) << "socket(" << family_ << ", " << type_ << ", " << protocol_ << ") errno=" << errno << " errstr=" << strerror(errno);
    }
}

namespace
{
struct _SSLInit
{
    _SSLInit()
    {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
    }
};

static _SSLInit s_init;

}  // namespace

SSLSocket::SSLSocket(int family, int type, int protocol) : Socket(family, type, protocol) {}

Socket::ptr SSLSocket::accept()
{
    SSLSocket::ptr sock    = std::make_shared<SSLSocket>(family_, type_, protocol_);
    int            newsock = ::accept(fd_, nullptr, nullptr);
    if (newsock == -1)
    {
        ELOG_ERROR(logger) << "accept(" << fd_ << ") errno=" << errno << " errstr=" << strerror(errno);
        return nullptr;
    }
    sock->ctx_ = ctx_;
    if (sock->init(newsock))
    {
        return sock;
    }
    return nullptr;
}

bool SSLSocket::bind(const Address::ptr addr) { return Socket::bind(addr); }

bool SSLSocket::connect(const Address::ptr addr, uint64_t timeout_ms)
{
    bool v = Socket::connect(addr, timeout_ms);
    if (v)
    {
        ctx_.reset(SSL_CTX_new(SSLv23_client_method()), SSL_CTX_free);
        ssl_.reset(SSL_new(ctx_.get()), SSL_free);
        SSL_set_fd(ssl_.get(), fd_);
        v = (SSL_connect(ssl_.get()) == 1);
    }
    return v;
}

bool SSLSocket::listen(int backlog) { return Socket::listen(backlog); }

bool SSLSocket::close() { return Socket::close(); }

int SSLSocket::send(const void* buffer, size_t length, int flags)
{
    if (ssl_)
    {
        return SSL_write(ssl_.get(), buffer, static_cast<int>(length));
    }
    return -1;
}

int SSLSocket::send(const iovec* buffers, size_t length, int flags)
{
    if (!ssl_)
    {
        return -1;
    }
    int total = 0;
    for (size_t i = 0; i < length; ++i)
    {
        int tmp = SSL_write(ssl_.get(), buffers[i].iov_base, static_cast<int>(buffers[i].iov_len));
        if (tmp <= 0)
        {
            return tmp;
        }
        total += tmp;
        if (tmp != static_cast<int>(buffers[i].iov_len))
        {
            break;
        }
    }
    return total;
}

int SSLSocket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags)
{
    EASY_ASSERT(false);
    return -1;
}

int SSLSocket::sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags)
{
    EASY_ASSERT(false);
    return -1;
}

int SSLSocket::recv(void* buffer, size_t length, int flags)
{
    if (ssl_)
    {
        return SSL_read(ssl_.get(), buffer, static_cast<int>(length));
    }
    return -1;
}

int SSLSocket::recv(iovec* buffers, size_t length, int flags)
{
    if (!ssl_)
    {
        return -1;
    }
    int total = 0;
    for (size_t i = 0; i < length; ++i)
    {
        int tmp = SSL_read(ssl_.get(), buffers[i].iov_base, static_cast<int>(buffers[i].iov_len));
        if (tmp <= 0)
        {
            return tmp;
        }
        total += tmp;
        if (tmp != static_cast<int>(buffers[i].iov_len))
        {
            break;
        }
    }
    return total;
}

int SSLSocket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags)
{
    EASY_ASSERT(false);
    return -1;
}

int SSLSocket::recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags)
{
    EASY_ASSERT(false);
    return -1;
}

bool SSLSocket::init(int sock)
{
    bool v = Socket::init(sock);
    if (v)
    {
        ssl_.reset(SSL_new(ctx_.get()), SSL_free);
        SSL_set_fd(ssl_.get(), fd_);
        v = (SSL_accept(ssl_.get()) == 1);
    }
    return v;
}

bool SSLSocket::loadCertificates(const std::string& cert_file, const std::string& key_file)
{
    ctx_.reset(SSL_CTX_new(SSLv23_server_method()), SSL_CTX_free);
    if (SSL_CTX_use_certificate_chain_file(ctx_.get(), cert_file.c_str()) != 1)
    {
        ELOG_ERROR(logger) << "SSL_CTX_use_certificate_chain_file(" << cert_file << ") error";
        return false;
    }
    if (SSL_CTX_use_PrivateKey_file(ctx_.get(), key_file.c_str(), SSL_FILETYPE_PEM) != 1)
    {
        ELOG_ERROR(logger) << "SSL_CTX_use_PrivateKey_file(" << key_file << ") error";
        return false;
    }
    if (SSL_CTX_check_private_key(ctx_.get()) != 1)
    {
        ELOG_ERROR(logger) << "SSL_CTX_check_private_key cert_file=" << cert_file << " key_file=" << key_file;
        return false;
    }
    return true;
}

SSLSocket::ptr SSLSocket::CreateTCP(Address::ptr address) { return std::make_shared<SSLSocket>(address->family(), TCP, 0); }

SSLSocket::ptr SSLSocket::CreateTCPSocket() { return std::make_shared<SSLSocket>(IPv4, TCP, 0); }

SSLSocket::ptr SSLSocket::CreateTCPSocket6() { return std::make_shared<SSLSocket>(IPv6, TCP, 0); }

std::ostream& SSLSocket::dump(std::ostream& os) const
{
    os << "[SSLSocket fd=" << fd_ << " connected=" << isConnected_ << " family=" << family_ << " type=" << type_ << " protocol=" << protocol_;
    if (localAddress_)
    {
        os << " local=" << localAddress_->toString();
    }
    if (remoteAddress_)
    {
        os << " remote=" << remoteAddress_->toString();
    }
    os << "]";
    return os;
}

std::ostream& operator<<(std::ostream& os, const Socket& sock) { return sock.dump(os); }
}  // namespace easy
