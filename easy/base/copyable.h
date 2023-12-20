#ifndef __EASY_COPYABLE_H__
#define __EASY_COPYABLE_H__

namespace easy {
class copyable {
 protected:
  // 构造函数设置为 protected 类型，类外不可调用，派生类可以
  copyable() = default;
  ~copyable() = default;
};
}  // namespace easy

#endif
