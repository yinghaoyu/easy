#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>

#include "easy/base/IOManager.h"
#include "easy/base/Logger.h"

static easy::Logger::ptr logger = EASY_LOG_ROOT();

int sock = 0;

void test_fiber()
{
  EASY_LOG_INFO(logger) << "test_fiber sock=" << sock;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  fcntl(sock, F_SETFL, O_NONBLOCK);

  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(80);
  inet_pton(AF_INET, "115.239.210.27", &addr.sin_addr.s_addr);

  if (!connect(sock, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)))
  {
  }
  else if (errno == EINPROGRESS)
  {
    EASY_LOG_INFO(logger) << "add event errno=" << errno << " "
                          << strerror(errno);
    easy::IOManager::GetThis()->addEvent(
        sock, easy::IOManager::READ,
        []() { EASY_LOG_INFO(logger) << "read callback"; });
    easy::IOManager::GetThis()->addEvent(
        sock, easy::IOManager::WRITE,
        []()
        {
          EASY_LOG_INFO(logger) << "write callback";
          // close(sock);
          easy::IOManager::GetThis()->triggerEvent(sock, easy::IOManager::READ);
          close(sock);
        });
  }
  else
  {
    EASY_LOG_INFO(logger) << "else " << errno << " " << strerror(errno);
  }
}

void test1()
{
  std::cout << "EPOLLIN=" << EPOLLIN << " EPOLLOUT=" << EPOLLOUT << std::endl;
  easy::IOManager iom(2, false);
  iom.schedule(&test_fiber);
}

easy::Timer::ptr s_timer;
void test_timer()
{
  easy::IOManager iom(2);
  s_timer = iom.addTimer(
      1000,
      []()
      {
        static int i = 0;
        EASY_LOG_INFO(logger) << "hello timer i=" << i;
        if (++i == 3)
        {
          s_timer->reset(2000, true);
        }
        if(i == 20)
        {
          s_timer->cancel();
        }
      },
      true);
}

int main(int argc, char **argv)
{
  // test1();
  test_timer();
  return 0;
}
