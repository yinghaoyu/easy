#include "easy/base/Logger.h"
#include "easy/net/Address.h"

easy::Logger::ptr logger = ELOG_ROOT();

void test()
{
    std::vector<easy::Address::ptr> addrs;

    ELOG_INFO(logger) << "Lookup begin";
    bool v = easy::Address::Lookup(addrs, "www.baidu.com");
    ELOG_INFO(logger) << "Lookup end";
    if (!v)
    {
        ELOG_ERROR(logger) << "lookup fail";
        return;
    }

    for (size_t i = 0; i < addrs.size(); ++i)
    {
        ELOG_INFO(logger) << i << " - " << addrs[i]->toString();
    }
}

void test_iface()
{
    ELOG_DEBUG(logger) << __func__;
    std::multimap<std::string, std::pair<easy::Address::ptr, uint32_t>> results;

    bool v = easy::Address::GetInterfaceAddresses(results);
    if (!v)
    {
        ELOG_ERROR(logger) << "GetInterfaceAddresses fail";
        return;
    }

    for (auto& i : results)
    {
        ELOG_INFO(logger) << i.first << " - " << i.second.first->toString() << " - " << i.second.second;
    }
}

void test_ipv4()
{
    ELOG_DEBUG(logger) << __func__;
    auto addr = easy::IPAddress::Create("127.0.0.8");
    if (addr)
    {
        ELOG_INFO(logger) << addr->toString();
    }
}

int main(int argc, char** argv)
{
    test_ipv4();
    test_iface();
    test();
    return 0;
}
