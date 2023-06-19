#ifndef __EASY_SOCKET_H__
#define __EASY_SOCKET_H__

#include "easy/base/noncopyable.h"
#include "easy/net/Address.h"

#include <openssl/ssl.h>
#include <sys/socket.h>
#include <memory>

namespace easy
{
class Socket : noncopyable, public std::enable_shared_from_this<Socket>
{
 public:
  typedef std::shared_ptr<Socket> ptr;

  enum Type
  {
    TCP = SOCK_STREAM,
    UDP = SOCK_DGRAM,
  };

  enum Family
  {
    IPv4 = AF_INET,
    IPv6 = AF_INET6,
    UNIX = AF_UNIX,
  };

  static Socket::ptr CreateTCP(Address::ptr address);
  static Socket::ptr CreateUDP(Address::ptr address);

  static Socket::ptr CreateTCPSocket();
  static Socket::ptr CreateUDPSocket();

  static Socket::ptr CreateTCPSocket6();
  static Socket::ptr CreateUDPSocket6();

  static Socket::ptr CreateUnixTCPSocket();
  static Socket::ptr CreateUnixUDPSocket();

  Socket(int family, int type, int protocol = 0);
  virtual ~Socket();

  int64_t getSendTimeout();
  void setSendTimeout(int64_t v);

  int64_t getRecvTimeout();
  void setRecvTimeout(int64_t v);

  bool getOption(int level, int option, void *result, socklen_t *len);

  template <class T>
  bool getOption(int level, int option, T &result)
  {
    socklen_t length = sizeof(T);
    return getOption(level, option, &result, &length);
  }

  bool setOption(int level, int option, const void *result, socklen_t len);

  template <class T>
  bool setOption(int level, int option, const T &value)
  {
    return setOption(level, option, &value, sizeof(T));
  }

  virtual Socket::ptr accept();

  virtual bool bind(const Address::ptr addr);

  virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1UL);

  virtual bool reconnect(uint64_t timeout_ms = -1UL);

  virtual bool listen(int backlog = SOMAXCONN);

  virtual bool close();

  virtual int send(const void *buffer, size_t length, int flags = 0);

  virtual int send(const iovec *buffers, size_t length, int flags = 0);

  virtual int sendTo(const void *buffer,
                     size_t length,
                     const Address::ptr to,
                     int flags = 0);

  virtual int sendTo(const iovec *buffers,
                     size_t length,
                     const Address::ptr to,
                     int flags = 0);

  virtual int recv(void *buffer, size_t length, int flags = 0);

  virtual int recv(iovec *buffers, size_t length, int flags = 0);

  virtual int recvFrom(void *buffer,
                       size_t length,
                       Address::ptr from,
                       int flags = 0);

  virtual int recvFrom(iovec *buffers,
                       size_t length,
                       Address::ptr from,
                       int flags = 0);

  Address::ptr getRemoteAddress();
  Address::ptr getLocalAddress();

  int family() const { return family_; }
  int type() const { return type_; }
  int protocol() const { return protocol_; }

  bool isConnected() const { return isConnected_; }
  bool checkConnected();
  bool isValid() const;
  int getError();

  virtual std::ostream &dump(std::ostream &os) const;

  virtual std::string toString() const;

  int fd() const { return fd_; }

  bool cancelRead();
  bool cancelWrite();
  bool cancelAccept();
  bool cancelAll();

 protected:
  void initSock();
  void newSock();
  virtual bool init(int sock);

 protected:
  int fd_;
  int family_;
  int type_;
  int protocol_;
  bool isConnected_;
  Address::ptr localAddress_;
  Address::ptr remoteAddress_;
};

class SSLSocket : public Socket
{
 public:
  typedef std::shared_ptr<SSLSocket> ptr;

  static SSLSocket::ptr CreateTCP(Address::ptr address);
  static SSLSocket::ptr CreateTCPSocket();
  static SSLSocket::ptr CreateTCPSocket6();

  SSLSocket(int family, int type, int protocol = 0);

  virtual Socket::ptr accept() override;

  virtual bool bind(const Address::ptr addr) override;

  virtual bool connect(const Address::ptr addr,
                       uint64_t timeout_ms = -1UL) override;

  virtual bool listen(int backlog = SOMAXCONN) override;

  virtual bool close() override;

  virtual int send(const void *buffer, size_t length, int flags = 0) override;

  virtual int send(const iovec *buffers, size_t length, int flags = 0) override;

  virtual int sendTo(const void *buffer,
                     size_t length,
                     const Address::ptr to,
                     int flags = 0) override;

  virtual int sendTo(const iovec *buffers,
                     size_t length,
                     const Address::ptr to,
                     int flags = 0) override;

  virtual int recv(void *buffer, size_t length, int flags = 0) override;

  virtual int recv(iovec *buffers, size_t length, int flags = 0) override;

  virtual int recvFrom(void *buffer,
                       size_t length,
                       Address::ptr from,
                       int flags = 0) override;

  virtual int recvFrom(iovec *buffers,
                       size_t length,
                       Address::ptr from,
                       int flags = 0) override;

  bool loadCertificates(const std::string &cert_file,
                        const std::string &key_file);

  virtual std::ostream &dump(std::ostream &os) const override;

 protected:
  virtual bool init(int sock) override;

 private:
  std::shared_ptr<SSL_CTX> ctx_;
  std::shared_ptr<SSL> ssl_;
};

std::ostream &operator<<(std::ostream &os, const Socket &sock);

}  // namespace easy

#endif
