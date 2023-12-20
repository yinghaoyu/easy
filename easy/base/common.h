#ifndef __EASY_COMMON_H__
#define __EASY_COMMON_H__

#include <cxxabi.h>
#include <boost/lexical_cast.hpp>
#include <memory>

namespace easy {

void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);

std::string BacktraceToString(int size = 64, int skip = 2,
                              const std::string& prefix = "");

template <class T, class... Args>
inline std::shared_ptr<T> protected_make_shared(Args&&... args) {
  struct Helper : T {
    Helper(Args&&... args) : T(std::forward<Args>(args)...) {}
  };
  return std::make_shared<Helper>(std::forward<Args>(args)...);
}

template <class V, class Map, class K>
V GetParamValue(const Map& m, const K& k, const V& def = V()) {
  auto it = m.find(k);
  if (it == m.end()) {
    return def;
  }
  try {
    return boost::lexical_cast<V>(it->second);
  } catch (...) {
  }
  return def;
}

template <class T>
const char* TypeToName() {
  static const char* s_name =
      abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
  return s_name;
}

}  // namespace easy

#endif
