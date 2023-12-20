#ifndef __EASY__NONCOPYABLE_H__
#define __EASY__NONCOPYABLE_H__

namespace easy {
class noncopyable {
  // 禁止了拷贝构造和拷贝赋值
  // 类外部、派生类调用引发编译出错
  noncopyable(const noncopyable&) = delete;
  void operator=(const noncopyable&) = delete;

 protected:
  // 构造函数和析构函数设置 protected 权限
  // 类外部调用出错，派生类可以调用
  noncopyable() = default;
  ~noncopyable() = default;
};
}  // namespace easy

#endif
