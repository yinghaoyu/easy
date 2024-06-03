#include "easy/base/IOManager.h"
#include "easy/base/Logger.h"
#include "easy/base/hook.h"

#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <cstddef>

static easy::Logger::ptr g_logger = ELOG_ROOT();

void test_sleep()
{
    easy::IOManager iom(1);
    iom.schedule([]() {
        sleep(2);
        ELOG_INFO(g_logger) << "sleep 2";
    });

    iom.schedule([]() {
        sleep(3);
        ELOG_INFO(g_logger) << "sleep 3";
    });
    ELOG_INFO(g_logger) << "test_sleep";
}

void test_sock()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(80);
    inet_pton(AF_INET, "36.152.44.95", &addr.sin_addr.s_addr);

    ELOG_INFO(g_logger) << "begin connect";
    int ret = connect(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    ELOG_INFO(g_logger) << "connect rt=" << ret << " errno=" << errno;

    if (ret)
    {
        return;
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    ret               = static_cast<int>(send(sock, data, sizeof(data), 0));
    ELOG_INFO(g_logger) << "send rt=" << ret << " errno=" << errno << " strerr=" << strerror(errno);

    if (ret <= 0)
    {
        return;
    }

    std::string buff;
    buff.resize(4096);

    ret = static_cast<int>(recv(sock, &buff[0], buff.size(), 0));
    ELOG_INFO(g_logger) << "recv rt=" << ret << " errno=" << errno;

    if (ret <= 0)
    {
        return;
    }

    buff.resize(static_cast<size_t>(ret));
    ELOG_INFO(g_logger) << buff;
}

int main(int argc, char** argv)
{
    test_sleep();
    easy::IOManager iom;
    iom.schedule(test_sock);
    return 0;
}
