#include "easy/net/Address.h"
#include "easy/base/Logger.h"
#include "easy/net/Endian.h"

#include <ifaddrs.h>  // freeifaddrs
#include <netdb.h>    // addrinfo
#include <sys/socket.h>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

namespace easy
{
static Logger::ptr logger = EASY_LOG_NAME("system");

template <typename T>
static T CreateMask(T bits)
{
  return static_cast<T>((1 << (sizeof(T) * 8 - bits)) - 1);
}

template <typename T>
static T BitCount(T value)
{
  T result = 0;
  for (; value; ++result)
  {
    // Brian Kernighan
    value = static_cast<T>(value & (value - 1));
  }
  return result;
}

Address::ptr Address::LookupAny(const std::string &host,
                                int family,
                                int type,
                                int protocol)
{
  std::vector<Address::ptr> result;
  if (Lookup(result, host, family, type, protocol))
  {
    return result[0];
  }
  return nullptr;
}

IPAddress::ptr Address::LookupAnyIPAddress(const std::string &host,
                                           int family,
                                           int type,
                                           int protocol)
{
  std::vector<Address::ptr> result;
  if (Lookup(result, host, family, type, protocol))
  {
    for (auto &i : result)
    {
      IPAddress::ptr ans = std::dynamic_pointer_cast<IPAddress>(i);
      if (ans)
      {
        return ans;
      }
    }
  }
  return nullptr;
}

bool Address::Lookup(std::vector<Address::ptr> &result,
                     const std::string &host,
                     int family,
                     int type,
                     int protocol)
{
  addrinfo hints, *results, *next;
  hints.ai_flags = 0;
  hints.ai_family = family;
  hints.ai_socktype = type;
  hints.ai_protocol = protocol;
  hints.ai_addrlen = 0;
  hints.ai_canonname = nullptr;
  hints.ai_addr = nullptr;
  hints.ai_next = nullptr;

  std::string node;
  const char *service = nullptr;

  // 检查 ipv6address serivce
  if (!host.empty() && host[0] == '[')
  {
    const char *endipv6 = static_cast<const char *>(
        memchr(host.c_str() + 1, ']', host.size() - 1));
    if (endipv6)
    {
      // TODO check out of range
      if (*(endipv6 + 1) == ':')
      {
        service = endipv6 + 2;
      }
      node = host.substr(1, static_cast<size_t>(endipv6 - host.c_str() - 1));
    }
  }

  // 检查 node serivce
  if (node.empty())
  {
    service = static_cast<const char *>(memchr(host.c_str(), ':', host.size()));
    if (service)
    {
      if (!memchr(
              service + 1, ':',
              static_cast<size_t>(host.c_str() + host.size() - service - 1)))
      {
        node = host.substr(0, static_cast<size_t>(service - host.c_str()));
        ++service;
      }
    }
  }

  if (node.empty())
  {
    node = host;
  }
  int error = getaddrinfo(node.c_str(), service, &hints, &results);
  if (error)
  {
    EASY_LOG_ERROR(logger) << "Address::Lookup getaddress(" << host << ", "
                           << family << ", " << type << ") err=" << error
                           << " errstr=" << gai_strerror(error);
    return false;
  }

  next = results;
  while (next)
  {
    result.push_back(
        Create(next->ai_addr, static_cast<socklen_t>(next->ai_addrlen)));
    next = next->ai_next;
  }

  freeaddrinfo(results);
  return !result.empty();
}

bool Address::GetInterfaceAddresses(
    std::multimap<std::string, std::pair<Address::ptr, uint32_t>> &result,
    int family)
{
  struct ifaddrs *next, *results;
  if (getifaddrs(&results) != 0)
  {
    EASY_LOG_ERROR(logger) << "Address::GetInterfaceAddresses getifaddrs "
                              " err="
                           << errno << " errstr=" << strerror(errno);
    return false;
  }

  try
  {
    for (next = results; next; next = next->ifa_next)
    {
      Address::ptr addr;
      uint32_t prefix_len = ~0u;
      if (family != AF_UNSPEC && family != next->ifa_addr->sa_family)
      {
        continue;
      }
      switch (next->ifa_addr->sa_family)
      {
      case AF_INET:
      {
        addr = Create(next->ifa_addr, sizeof(sockaddr_in));
        uint32_t netmask = (reinterpret_cast<sockaddr_in *>(next->ifa_netmask))
                               ->sin_addr.s_addr;
        prefix_len = BitCount(netmask);
      }
      break;
      case AF_INET6:
      {
        addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
        in6_addr &netmask =
            (reinterpret_cast<sockaddr_in6 *>(next->ifa_netmask))->sin6_addr;
        prefix_len = 0;
        for (int i = 0; i < 16; ++i)
        {
          prefix_len += BitCount(netmask.s6_addr[i]);
        }
      }
      break;
      default:
        break;
      }

      if (addr)
      {
        result.insert(
            std::make_pair(next->ifa_name, std::make_pair(addr, prefix_len)));
      }
    }
  }
  catch (...)
  {
    EASY_LOG_ERROR(logger) << "Address::GetInterfaceAddresses exception";
    freeifaddrs(results);
    return false;
  }
  freeifaddrs(results);
  return !result.empty();
}

bool Address::GetInterfaceAddresses(
    std::vector<std::pair<Address::ptr, uint32_t>> &result,
    const std::string &iface,
    int family)
{
  if (iface.empty() || iface == "*")
  {
    if (family == AF_INET || family == AF_UNSPEC)
    {
      result.push_back(std::make_pair(std::make_shared<IPv4Address>(), 0u));
    }
    if (family == AF_INET6 || family == AF_UNSPEC)
    {
      result.push_back(std::make_pair(std::make_shared<IPv6Address>(), 0u));
    }
    return true;
  }

  std::multimap<std::string, std::pair<Address::ptr, uint32_t>> results;

  if (!GetInterfaceAddresses(results, family))
  {
    return false;
  }

  auto its = results.equal_range(iface);
  for (; its.first != its.second; ++its.first)
  {
    result.push_back(its.first->second);
  }
  return !result.empty();
}

int Address::family() const
{
  return addr()->sa_family;
}

std::string Address::toString() const
{
  std::stringstream ss;
  insert(ss);
  return ss.str();
}

Address::ptr Address::Create(const sockaddr *addr, socklen_t addrlen)
{
  if (addr == nullptr)
  {
    return nullptr;
  }

  Address::ptr result;
  switch (addr->sa_family)
  {
  case AF_INET:
    result = std::make_shared<IPv4Address>(
        *reinterpret_cast<const sockaddr_in *>(addr));
    break;
  case AF_INET6:
    result = std::make_shared<IPv6Address>(
        *reinterpret_cast<const sockaddr_in6 *>(addr));
    break;
  default:
    result = std::make_shared<UnknownAddress>(*addr);
    break;
  }
  return result;
}

bool Address::operator<(const Address &other) const
{
  socklen_t minlen = std::min(addrLen(), other.addrLen());
  int result = memcmp(addr(), other.addr(), minlen);
  if (result < 0)
  {
    return 0;
  }
  else if (result > 0)
  {
    return false;
  }
  else if (addrLen() < other.addrLen())
  {
    return true;
  }
  return false;
}

bool Address::operator==(const Address &other) const
{
  return addrLen() == other.addrLen() &&
         memcmp(addr(), other.addr(), addrLen()) == 0;
}

bool Address::operator!=(const Address &other) const
{
  return !(*this == other);
}

IPAddress::ptr IPAddress::Create(const char *address, uint16_t port)
{
  addrinfo hints, *results;
  memset(&hints, 0, sizeof(addrinfo));

  hints.ai_flags = AI_NUMERICHOST;
  hints.ai_family = AF_UNSPEC;

  int error = getaddrinfo(address, NULL, &hints, &results);
  if (error)
  {
    EASY_LOG_DEBUG(logger) << "IPAddress::Create(" << address << ", " << port
                           << ") error=" << error << " errno=" << errno
                           << " errstr=" << strerror(errno);
    return nullptr;
  }
  try
  {
    IPAddress::ptr result =
        std::dynamic_pointer_cast<IPAddress>(Address::Create(
            results->ai_addr, static_cast<socklen_t>(results->ai_addrlen)));
    if (result)
    {
      result->setPort(port);
    }
    freeaddrinfo(results);
    return result;
  }
  catch (...)
  {
    freeaddrinfo(results);
    return nullptr;
  }
}

IPv4Address::ptr IPv4Address::Create(const char *address, uint16_t port)
{
  IPv4Address::ptr rt = std::make_shared<IPv4Address>();
  rt->addr_.sin_port = hostToNetwork(port);
  int result = inet_pton(AF_INET, address, &rt->addr_.sin_addr);
  if (result <= 0)
  {
    EASY_LOG_DEBUG(logger) << "IPv4Address::Create(" << address << ", " << port
                           << ") rt=" << result << " errno=" << errno
                           << " errstr=" << strerror(errno);
    return nullptr;
  }
  return rt;
}

IPv4Address::IPv4Address(const sockaddr_in &address)
{
  addr_ = address;
}

IPv4Address::IPv4Address(uint32_t address, uint16_t port)
{
  memset(&addr_, 0, sizeof(addr_));
  addr_.sin_family = AF_INET;
  addr_.sin_port = hostToNetwork(port);
  addr_.sin_addr.s_addr = hostToNetwork(address);
}

sockaddr *IPv4Address::addr()
{
  return reinterpret_cast<sockaddr *>(&addr_);
}

const sockaddr *IPv4Address::addr() const
{
  return reinterpret_cast<const sockaddr *>(&addr_);
}

socklen_t IPv4Address::addrLen() const
{
  return sizeof(addr_);
}

std::ostream &IPv4Address::insert(std::ostream &os) const
{
  uint32_t addr = networkToHost(addr_.sin_addr.s_addr);
  os << ((addr >> 24) & 0xff) << "." << ((addr >> 16) & 0xff) << "."
     << ((addr >> 8) & 0xff) << "." << (addr & 0xff);
  os << ":" << networkToHost(addr_.sin_port);
  return os;
}

IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len)
{
  if (prefix_len > 32)
  {
    return nullptr;
  }

  sockaddr_in baddr(addr_);
  baddr.sin_addr.s_addr |= hostToNetwork(CreateMask<uint32_t>(prefix_len));
  return std::make_shared<IPv4Address>(baddr);
}

IPAddress::ptr IPv4Address::networdAddress(uint32_t prefix_len)
{
  if (prefix_len > 32)
  {
    return nullptr;
  }

  sockaddr_in baddr(addr_);
  baddr.sin_addr.s_addr &= hostToNetwork(CreateMask<uint32_t>(prefix_len));
  return std::make_shared<IPv4Address>(baddr);
}

IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len)
{
  sockaddr_in subnet;
  memset(&subnet, 0, sizeof(subnet));
  subnet.sin_family = AF_INET;
  subnet.sin_addr.s_addr = ~hostToNetwork(CreateMask<uint32_t>(prefix_len));
  return std::make_shared<IPv4Address>(subnet);
}

uint32_t IPv4Address::port() const
{
  return networkToHost(addr_.sin_port);
}

void IPv4Address::setPort(uint16_t v)
{
  addr_.sin_port = hostToNetwork(v);
}

IPv6Address::ptr IPv6Address::Create(const char *address, uint16_t port)
{
  IPv6Address::ptr rt = std::make_shared<IPv6Address>();
  rt->addr_.sin6_port = hostToNetwork(port);
  int result = inet_pton(AF_INET6, address, &rt->addr_.sin6_addr);
  if (result <= 0)
  {
    EASY_LOG_DEBUG(logger) << "IPv6Address::Create(" << address << ", " << port
                           << ") rt=" << result << " errno=" << errno
                           << " errstr=" << strerror(errno);
    return nullptr;
  }
  return rt;
}

IPv6Address::IPv6Address()
{
  memset(&addr_, 0, sizeof(addr_));
  addr_.sin6_family = AF_INET6;
}

IPv6Address::IPv6Address(const sockaddr_in6 &address)
{
  addr_ = address;
}

IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port)
{
  memset(&addr_, 0, sizeof(addr_));
  addr_.sin6_family = AF_INET6;
  addr_.sin6_port = hostToNetwork(port);
  memcpy(&addr_.sin6_addr.s6_addr, address, 16);
}

sockaddr *IPv6Address::addr()
{
  return reinterpret_cast<sockaddr *>(&addr_);
}

const sockaddr *IPv6Address::addr() const
{
  return reinterpret_cast<const sockaddr *>(&addr_);
}

socklen_t IPv6Address::addrLen() const
{
  return sizeof(addr_);
}

std::ostream &IPv6Address::insert(std::ostream &os) const
{
  os << "[";
  const uint16_t *addr =
      reinterpret_cast<const uint16_t *>(addr_.sin6_addr.s6_addr);
  bool used_zeros = false;
  for (size_t i = 0; i < 8; ++i)
  {
    if (addr[i] == 0 && !used_zeros)
    {
      continue;
    }
    if (i && addr[i - 1] == 0 && !used_zeros)
    {
      os << ":";
      used_zeros = true;
    }
    if (i)
    {
      os << ":";
    }
    os << std::hex << networkToHost(addr[i]) << std::dec;
  }

  if (!used_zeros && addr[7] == 0)
  {
    os << "::";
  }
  os << "]:" << networkToHost(addr_.sin6_port);
  return os;
}

IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len)
{
  sockaddr_in6 baddr(addr_);

  auto &old = baddr.sin6_addr.s6_addr[prefix_len / 8];

  old = static_cast<uint8_t>(old | CreateMask<uint8_t>(prefix_len % 8));

  for (int i = prefix_len / 8 + 1; i < 16; ++i)
  {
    baddr.sin6_addr.s6_addr[i] = 0xff;
  }
  return std::make_shared<IPv6Address>(baddr);
}

IPAddress::ptr IPv6Address::networdAddress(uint32_t prefix_len)
{
  sockaddr_in6 baddr(addr_);

  auto &old = baddr.sin6_addr.s6_addr[prefix_len / 8];

  old = static_cast<uint8_t>(old & CreateMask<uint8_t>(prefix_len % 8));

  for (int i = prefix_len / 8 + 1; i < 16; ++i)
  {
    baddr.sin6_addr.s6_addr[i] = 0x00;
  }
  return std::make_shared<IPv6Address>(baddr);
}

IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len)
{
  sockaddr_in6 subnet;
  memset(&subnet, 0, sizeof(subnet));
  subnet.sin6_family = AF_INET6;

  subnet.sin6_addr.s6_addr[prefix_len / 8] =
      static_cast<uint8_t>(~CreateMask<uint8_t>(prefix_len % 8));

  for (uint32_t i = 0; i < prefix_len / 8; ++i)
  {
    subnet.sin6_addr.s6_addr[i] = 0xff;
  }
  return std::make_shared<IPv6Address>(subnet);
}

uint32_t IPv6Address::port() const
{
  return networkToHost(addr_.sin6_port);
}

void IPv6Address::setPort(uint16_t v)
{
  addr_.sin6_port = hostToNetwork(v);
}

static const size_t MAX_PATH_LEN =
    sizeof((static_cast<sockaddr_un *>(0))->sun_path) - 1;

UnixAddress::UnixAddress()
{
  memset(&addr_, 0, sizeof(addr_));
  addr_.sun_family = AF_UNIX;
  length_ = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}

UnixAddress::UnixAddress(const std::string &path)
{
  memset(&addr_, 0, sizeof(addr_));
  addr_.sun_family = AF_UNIX;
  length_ = static_cast<socklen_t>(path.size() + 1);

  if (!path.empty() && path[0] == '\0')
  {
    --length_;
  }

  if (length_ > sizeof(addr_.sun_path))
  {
    throw std::logic_error("path too long");
  }
  memcpy(addr_.sun_path, path.c_str(), length_);
  length_ += static_cast<socklen_t>(offsetof(sockaddr_un, sun_path));
}

void UnixAddress::setAddrLen(uint32_t v)
{
  length_ = v;
}

sockaddr *UnixAddress::addr()
{
  return reinterpret_cast<sockaddr *>(&addr_);
}

const sockaddr *UnixAddress::addr() const
{
  return reinterpret_cast<const sockaddr *>(&addr_);
}

socklen_t UnixAddress::addrLen() const
{
  return length_;
}

std::string UnixAddress::path() const
{
  std::stringstream ss;
  if (length_ > offsetof(sockaddr_un, sun_path) && addr_.sun_path[0] == '\0')
  {
    ss << "\\0"
       << std::string(addr_.sun_path + 1,
                      length_ - offsetof(sockaddr_un, sun_path) - 1);
  }
  else
  {
    ss << addr_.sun_path;
  }
  return ss.str();
}

std::ostream &UnixAddress::insert(std::ostream &os) const
{
  if (length_ > offsetof(sockaddr_un, sun_path) && addr_.sun_path[0] == '\0')
  {
    return os << "\\0"
              << std::string(addr_.sun_path + 1,
                             length_ - offsetof(sockaddr_un, sun_path) - 1);
  }
  return os << addr_.sun_path;
}

UnknownAddress::UnknownAddress(int family)
{
  memset(&addr_, 0, sizeof(addr_));
  addr_.sa_family = static_cast<sa_family_t>(family);
}

UnknownAddress::UnknownAddress(const sockaddr &addr)
{
  addr_ = addr;
}

sockaddr *UnknownAddress::addr()
{
  return reinterpret_cast<sockaddr *>(&addr_);
}

const sockaddr *UnknownAddress::addr() const
{
  return reinterpret_cast<const sockaddr *>(&addr_);
}

socklen_t UnknownAddress::addrLen() const
{
  return sizeof(addr_);
}

std::ostream &UnknownAddress::insert(std::ostream &os) const
{
  os << "[UnknownAddress family=" << addr_.sa_family << "]";
  return os;
}

std::ostream &operator<<(std::ostream &os, const Address &addr)
{
  return addr.insert(os);
}

}  // namespace easy
