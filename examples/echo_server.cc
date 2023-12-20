#include "easy/base/Logger.h"
#include "easy/net/Buffer.h"
#include "easy/net/TcpServer.h"

#include <unistd.h>
#include <iostream>
#include <memory>

easy::Logger::ptr logger = EASY_LOG_ROOT();

class EchoServer : public easy::TcpServer {
 public:
  EchoServer(int type);
  void handleClient(easy::Socket::ptr client);

 private:
  int type_ = 0;
};

EchoServer::EchoServer(int type) : type_(type) {}

void EchoServer::handleClient(easy::Socket::ptr client) {
  EASY_LOG_INFO(logger) << "handleClient " << *client;
  easy::Buffer::ptr buff = std::make_shared<easy::Buffer>();
  while (true) {
    buff->clear();
    std::vector<iovec> iovs;
    buff->getWriteBuffers(iovs, 1024);

    int rt = client->recv(&iovs[0], iovs.size());
    if (rt == 0) {
      EASY_LOG_INFO(logger) << "client close: " << *client;
      break;
    } else if (rt < 0) {
      EASY_LOG_INFO(logger) << "client error rt=" << rt << " errno=" << errno
                            << " errstr=" << strerror(errno);
      break;
    }
    buff->setPosition(buff->position() + static_cast<size_t>(rt));
    buff->setPosition(0);
    if (type_ == 1) {                 // text
      std::cout << buff->toString();  // << std::endl;
    } else {
      std::cout << buff->toHexString();  // << std::endl;
    }
    std::cout.flush();
  }
}

int type = 1;

void run() {
  EASY_LOG_INFO(logger) << "server type=" << type;
  EchoServer::ptr es(new EchoServer(type));
  auto addr = easy::Address::LookupAny("0.0.0.0:12345");
  while (!es->bind(addr)) {
    sleep(2);
  }
  es->start();
}

int main(int argc, char** argv) {
  if (argc < 2) {
    EASY_LOG_INFO(logger) << "used as[" << argv[0] << " -t] or [" << argv[0]
                          << " -b]";
    return 0;
  }

  if (!strcmp(argv[1], "-b")) {
    type = 2;
  }

  easy::IOManager iom(2);
  iom.schedule(run);
  return 0;
}
