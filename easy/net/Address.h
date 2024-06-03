#ifndef __EASY_ADDRESS_H__
#define __EASY_ADDRESS_H__

#include <arpa/inet.h>  // sockaddr_in
#include <sys/socket.h>
#include <sys/un.h>  // sockaddr_un
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace easy
{
class IPAddress;

class Address
{
  public:
    typedef std::shared_ptr<Address> ptr;

    static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);

    // 通过 host 地址返回对应的所有 Address
    static bool Lookup(std::vector<Address::ptr>& result, const std::string& host, int family = AF_INET, int type = 0, int protocol = 0);

    static Address::ptr LookupAny(const std::string& host, int family = AF_INET, int type = 0, int protocol = 0);

    static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host, int family = AF_INET, int type = 0, int protocol = 0);

    // 返回本机所有网卡的<网卡名, 地址, 子网掩码位数>
    static bool GetInterfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t>>& result, int family = AF_INET);

    // 获取指定网卡的地址和子网掩码位数
    static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>>& result, const std::string& iface, int family = AF_INET);

    virtual ~Address() {}

    int family() const;

    virtual const sockaddr* addr() const    = 0;
    virtual sockaddr*       addr()          = 0;
    virtual socklen_t       addrLen() const = 0;

    virtual std::ostream& insert(std::ostream& os) const = 0;
    std::string           toString() const;

    bool operator<(const Address& other) const;
    bool operator==(const Address& other) const;
    bool operator!=(const Address& other) const;
};

class IPAddress : public Address
{
  public:
    typedef std::shared_ptr<IPAddress> ptr;

    static IPAddress::ptr Create(const char* adddress, uint16_t port = 0);

    // 获取该地址的广播地址
    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;

    // 获取该地址的网段
    virtual IPAddress::ptr networdAddress(uint32_t prefix_len) = 0;

    // 获取子网掩码地址
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

    virtual uint32_t port() const = 0;

    virtual void setPort(uint16_t port) = 0;
};

class IPv4Address : public IPAddress
{
  public:
    typedef std::shared_ptr<IPv4Address> ptr;

    static IPv4Address::ptr Create(const char* address, uint16_t port = 0);

    IPv4Address(const sockaddr_in& address);

    IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

    const sockaddr* addr() const override;
    sockaddr*       addr() override;
    socklen_t       addrLen() const override;
    std::ostream&   insert(std::ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;
    uint32_t       port() const override;
    void           setPort(uint16_t port) override;

  private:
    sockaddr_in addr_;
};

class IPv6Address : public IPAddress
{
  public:
    typedef std::shared_ptr<IPv6Address> ptr;

    static IPv6Address::ptr Create(const char* address, uint16_t port = 0);

    IPv6Address();

    IPv6Address(const sockaddr_in6& address);

    IPv6Address(const uint8_t address[16], uint16_t port = 0);

    const sockaddr* addr() const override;
    sockaddr*       addr() override;
    socklen_t       addrLen() const override;
    std::ostream&   insert(std::ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;
    uint32_t       port() const override;
    void           setPort(uint16_t port) override;

  private:
    sockaddr_in6 addr_;
};

class UnixAddress : public Address
{
  public:
    typedef std::shared_ptr<UnixAddress> ptr;
    UnixAddress();

    // 通过路径构造UnixAddress
    UnixAddress(const std::string& path);

    const sockaddr* addr() const override;
    sockaddr*       addr() override;
    socklen_t       addrLen() const override;
    void            setAddrLen(uint32_t v);
    std::string     path() const;
    std::ostream&   insert(std::ostream& os) const override;

  private:
    sockaddr_un addr_;
    socklen_t   length_;
};

class UnknownAddress : public Address
{
  public:
    typedef std::shared_ptr<UnknownAddress> ptr;

    UnknownAddress(int family);

    UnknownAddress(const sockaddr& addr);

    const sockaddr* addr() const override;
    sockaddr*       addr() override;
    socklen_t       addrLen() const override;
    std::ostream&   insert(std::ostream& os) const override;

  private:
    sockaddr addr_;
};

std::ostream& operator<<(std::ostream& os, const Address& addr);

}  // namespace easy

#endif
