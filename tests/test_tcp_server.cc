#include "easy/base/IOManager.h"
#include "easy/base/Logger.h"
#include "easy/net/TcpServer.h"

#include <unistd.h>

static easy::Logger::ptr logger = ELOG_ROOT();

void run()
{
    auto addr = easy::Address::LookupAny("0.0.0.0:12345");
    // auto addr2 = easy::UnixAddress::ptr(new
    // easy::UnixAddress("/tmp/unix_addr"));
    std::vector<easy::Address::ptr> addrs;
    addrs.push_back(addr);
    // addrs.push_back(addr2);

    easy::TcpServer::ptr            tcp_server(new easy::TcpServer);
    std::vector<easy::Address::ptr> fails;
    while (!tcp_server->bind(addrs, fails))
    {
        sleep(2);
    }
    tcp_server->start();
}
int main(int argc, char** argv)
{
    easy::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
