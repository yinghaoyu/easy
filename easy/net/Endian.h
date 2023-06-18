#ifndef __EASY_ENDIAN_H__
#define __EASY_ENDIAN_H__

#include <endian.h>
#include <stdint.h>
#include <type_traits>

namespace easy
{
template <typename T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type hostToNetwork(
    T value)
{
  return htobe64(value);
}

template <typename T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type hostToNetwork(
    T value)
{
  return htobe32(value);
}

template <typename T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type hostToNetwork(
    T value)
{
  return htobe16(value);
}

template <typename T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type networkToHost(
    T value)
{
  return be64toh(value);
}

template <typename T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type networkToHost(
    T value)
{
  return be32toh(value);
}

template <typename T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type networkToHost(
    T value)
{
  return be16toh(value);
}

}  // namespace easy

#endif
