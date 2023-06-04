#ifndef __EASY__SINGLETON_H__
#define __EASY__SINGLETON_H__

#include <memory>

namespace easy
{

template <typename T>
class Signleton
{
 public:
  static T *GetInstance()
  {
    static T v;
    return &v;
  }
};

template <typename T>
class SingletonPtr
{
 public:
  static std::shared_ptr<T> GetInstance()
  {
    static std::shared_ptr<T> v = std::make_shared<T>();
    return v;
  }
};

}  // namespace easy

#endif
