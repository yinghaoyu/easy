#ifndef __EASY_ENDIAN_H__
#define __EASY_ENDIAN_H__

#include <byteswap.h>
#include <endian.h>
#include <stdint.h>
#include <type_traits>

#define EASY_LITTLE_ENDIAN 1
#define EASY_BIG_ENDIAN 2

namespace easy {
template <typename T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type hostToNetwork(
    T value) {
  return htobe64(value);
}

template <typename T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type hostToNetwork(
    T value) {
  return htobe32(value);
}

template <typename T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type hostToNetwork(
    T value) {
  return htobe16(value);
}

template <typename T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type networkToHost(
    T value) {
  return be64toh(value);
}

template <typename T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type networkToHost(
    T value) {
  return be32toh(value);
}

template <typename T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type networkToHost(
    T value) {
  return be16toh(value);
}

template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type byteswap(
    T value) {
  return bswap_64(value);
}

template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type byteswap(
    T value) {
  return bswap_32(value);
}

template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type byteswap(
    T value) {
  return bswap_16(value);
}

#if BYTE_ORDER == BIG_ENDIAN
#define EASY_BYTE_ORDER EASY_BIG_ENDIAN
#else
#define EASY_BYTE_ORDER EASY_LITTLE_ENDIAN
#endif

}  // namespace easy

#endif
