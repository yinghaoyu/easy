#include "easy/net/TcpServer.h"
#include <vector>
#include "easy/base/Config.h"
#include "easy/base/Logger.h"

namespace easy
{
static ConfigVar<uint64_t>::ptr tcp_server_read_timeout =
    Config::Lookup("tcp_server.read_timeout", static_cast<uint64_t>(60 * 1000 * 2), "tcp server read timeout");

static Logger::ptr logger = ELOG_NAME("system");

TcpServer::TcpServer(IOManager* worker, IOManager* io_worker, IOManager* accept_worker)
    : worker_(worker),
      io_worker_(io_worker),
      accept_worker_(accept_worker),
      recvTimeout_(tcp_server_read_timeout->value()),
      name_("easy/1.0.0"),
      running_(false)
{}

TcpServer::~TcpServer()
{
    for (auto& i : socks_)
    {
        i->close();
    }
    socks_.clear();
}

bool TcpServer::bind(Address::ptr addr, bool ssl)
{
    std::vector<Address::ptr> addrs;
    std::vector<Address::ptr> fails;
    addrs.push_back(addr);
    return bind(addrs, fails, ssl);
}

bool TcpServer::bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fails, bool ssl)
{
    ssl_ = ssl;
    for (auto& addr : addrs)
    {
        Socket::ptr sock = ssl ? SSLSocket::CreateTCP(addr) : Socket::CreateTCP(addr);
        if (!sock->bind(addr))
        {
            ELOG_ERROR(logger) << "bind fail errno=" << errno << " errstr=" << strerror(errno) << " addr=[" << addr->toString() << "]";
            fails.push_back(addr);
            continue;
        }
        if (!sock->listen())
        {
            ELOG_ERROR(logger) << "listen fail errno=" << errno << " errstr=" << strerror(errno) << " addr=[" << addr->toString() << "]";
            fails.push_back(addr);
            continue;
        }
        socks_.push_back(sock);
    }

    if (!fails.empty())
    {
        socks_.clear();
        return false;
    }

    for (auto& i : socks_)
    {
        ELOG_INFO(logger) << "type=" << type_ << " name=" << name_ << " ssl=" << ssl_ << " server bind success: " << *i;
    }
    return true;
}

void TcpServer::startAccept(Socket::ptr sock)
{
    while (running_)  // accept_worker_ loop
    {
        Socket::ptr client = sock->accept();
        if (client)
        {
            client->setRecvTimeout(static_cast<int64_t>(recvTimeout_));
            io_worker_->schedule(std::bind(&TcpServer::handleClient, shared_from_this(), client));
        }
        else
        {
            ELOG_ERROR(logger) << "accept errno=" << errno << " errstr=" << strerror(errno);
        }
    }
}

bool TcpServer::start()
{
    if (running_)
    {
        return true;
    }
    running_ = true;
    for (auto& sock : socks_)
    {
        accept_worker_->schedule(std::bind(&TcpServer::startAccept, shared_from_this(), sock));
    }
    return true;
}

void TcpServer::stop()
{
    running_  = false;
    auto self = shared_from_this();
    accept_worker_->schedule([this, self]() {
        for (auto& sock : socks_)
        {
            sock->cancelAll();
            sock->close();
        }
        socks_.clear();
    });
}

void TcpServer::handleClient(Socket::ptr client)  // virtual
{
    ELOG_INFO(logger) << "handleClient: " << *client;
}

bool TcpServer::loadCertificates(const std::string& cert_file, const std::string& key_file)
{
    for (auto& i : socks_)
    {
        auto ssl_socket = std::dynamic_pointer_cast<SSLSocket>(i);
        if (ssl_socket)
        {
            if (!ssl_socket->loadCertificates(cert_file, key_file))
            {
                return false;
            }
        }
    }
    return true;
}

std::string TcpServer::toString(const std::string& prefix)
{
    std::stringstream ss;
    ss << prefix << "[type=" << type_ << " name=" << name_ << " ssl=" << ssl_ << " worker=" << (worker_ ? worker_->name() : "")
       << " accept=" << (accept_worker_ ? accept_worker_->name() : "") << " recv_timeout=" << recvTimeout_ << "]" << std::endl;
    std::string pfx = prefix.empty() ? "    " : prefix;
    for (auto& i : socks_)
    {
        ss << pfx << pfx << *i << std::endl;
    }
    return ss.str();
}

}  // namespace easy
