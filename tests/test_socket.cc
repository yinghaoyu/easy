#include "easy/base/IOManager.h"
#include "easy/base/logger.h"
#include "easy/net/Socket.h"

static easy::Logger::ptr logger = EASY_LOG_ROOT();

void test_socket() {
  easy::IPAddress::ptr addr =
      easy::Address::LookupAnyIPAddress("www.baidu.com");
  if (addr) {
    EASY_LOG_INFO(logger) << "get address: " << addr->toString();
  } else {
    EASY_LOG_ERROR(logger) << "get address fail";
    return;
  }

  easy::Socket::ptr sock = easy::Socket::CreateTCP(addr);
  addr->setPort(80);
  EASY_LOG_INFO(logger) << "addr=" << addr->toString();
  if (!sock->connect(addr)) {
    EASY_LOG_ERROR(logger) << "connect " << addr->toString() << " fail";
    return;
  } else {
    EASY_LOG_INFO(logger) << "connect " << addr->toString() << " connected";
  }

  const char buff[] = "GET / HTTP/1.0\r\n\r\n";
  int rt = sock->send(buff, sizeof(buff));
  if (rt <= 0) {
    EASY_LOG_INFO(logger) << "send fail rt=" << rt;
    return;
  }

  std::string buffs;
  buffs.resize(4096);
  rt = sock->recv(&buffs[0], buffs.size());

  if (rt <= 0) {
    EASY_LOG_INFO(logger) << "recv fail rt=" << rt;
    return;
  }

  buffs.resize(static_cast<size_t>(rt));
  EASY_LOG_INFO(logger) << buffs;
}

int main(int argc, char** argv) {
  easy::IOManager iom;
  iom.schedule(&test_socket);
  return 0;
}
