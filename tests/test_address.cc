#include "easy/base/Logger.h"
#include "easy/net/Address.h"

easy::Logger::ptr logger = EASY_LOG_ROOT();

void test() {
  std::vector<easy::Address::ptr> addrs;

  EASY_LOG_INFO(logger) << "Lookup begin";
  bool v = easy::Address::Lookup(addrs, "www.baidu.com");
  EASY_LOG_INFO(logger) << "Lookup end";
  if (!v) {
    EASY_LOG_ERROR(logger) << "lookup fail";
    return;
  }

  for (size_t i = 0; i < addrs.size(); ++i) {
    EASY_LOG_INFO(logger) << i << " - " << addrs[i]->toString();
  }
}

void test_iface() {
  EASY_LOG_DEBUG(logger) << __func__;
  std::multimap<std::string, std::pair<easy::Address::ptr, uint32_t>> results;

  bool v = easy::Address::GetInterfaceAddresses(results);
  if (!v) {
    EASY_LOG_ERROR(logger) << "GetInterfaceAddresses fail";
    return;
  }

  for (auto& i : results) {
    EASY_LOG_INFO(logger) << i.first << " - " << i.second.first->toString()
                          << " - " << i.second.second;
  }
}

void test_ipv4() {
  EASY_LOG_DEBUG(logger) << __func__;
  auto addr = easy::IPAddress::Create("127.0.0.8");
  if (addr) {
    EASY_LOG_INFO(logger) << addr->toString();
  }
}

int main(int argc, char** argv) {
  test_ipv4();
  test_iface();
  test();
  return 0;
}
