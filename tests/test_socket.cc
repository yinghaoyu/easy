#include "easy/base/IOManager.h"
#include "easy/base/logger.h"
#include "easy/net/Socket.h"

static easy::Logger::ptr logger = ELOG_ROOT();

void test_socket()
{
    easy::IPAddress::ptr addr = easy::Address::LookupAnyIPAddress("www.baidu.com");
    if (addr)
    {
        ELOG_INFO(logger) << "get address: " << addr->toString();
    }
    else
    {
        ELOG_ERROR(logger) << "get address fail";
        return;
    }

    easy::Socket::ptr sock = easy::Socket::CreateTCP(addr);
    addr->setPort(80);
    ELOG_INFO(logger) << "addr=" << addr->toString();
    if (!sock->connect(addr))
    {
        ELOG_ERROR(logger) << "connect " << addr->toString() << " fail";
        return;
    }
    else
    {
        ELOG_INFO(logger) << "connect " << addr->toString() << " connected";
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int        rt     = sock->send(buff, sizeof(buff));
    if (rt <= 0)
    {
        ELOG_INFO(logger) << "send fail rt=" << rt;
        return;
    }

    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());

    if (rt <= 0)
    {
        ELOG_INFO(logger) << "recv fail rt=" << rt;
        return;
    }

    buffs.resize(static_cast<size_t>(rt));
    ELOG_INFO(logger) << buffs;
}

int main(int argc, char** argv)
{
    easy::IOManager iom;
    iom.schedule(&test_socket);
    return 0;
}
