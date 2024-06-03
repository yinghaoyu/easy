#ifndef __EASY_TCPSERVER_H__
#define __EASY_TCPSERVER_H__

#include "easy/base/IOManager.h"
#include "easy/base/noncopyable.h"
#include "easy/net/Address.h"
#include "easy/net/Socket.h"

#include <memory>
#include <string>
#include <vector>

namespace easy
{
class TcpServer : noncopyable, public std::enable_shared_from_this<TcpServer>
{
  public:
    typedef std::shared_ptr<TcpServer> ptr;

    TcpServer(IOManager* worker = IOManager::GetThis(), IOManager* io_worker = IOManager::GetThis(), IOManager* accept_worker = IOManager::GetThis());

    virtual ~TcpServer();

    virtual bool bind(Address::ptr addr, bool ssl = false);

    virtual bool bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fails, bool ssl = false);

    bool loadCertificates(const std::string& cert_file, const std::string& key_file);

    virtual bool start();

    virtual void stop();

    uint64_t getRecvTimeout() const { return recvTimeout_; }

    void setRecvTimeout(uint64_t ms) { recvTimeout_ = ms; }

    std::string name() const { return name_; }

    void setName(const std::string& name) { name_ = name; }

    bool isRunning() const { return running_; }

    virtual std::string toString(const std::string& prefix = "");

    std::vector<Socket::ptr> socks() const { return socks_; }

    bool isSSL() const { return ssl_; }

  protected:
    virtual void handleClient(Socket::ptr client);

    virtual void startAccept(Socket::ptr sock);

  private:
    std::vector<Socket::ptr> socks_;
    IOManager*               worker_;
    IOManager*               io_worker_;
    IOManager*               accept_worker_;
    uint64_t                 recvTimeout_;
    std::string              name_;
    std::string              type_{"tcp"};
    bool                     running_;
    bool                     ssl_{false};
};
}  // namespace easy

#endif
