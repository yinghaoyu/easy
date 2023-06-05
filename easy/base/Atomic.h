#ifndef __EASY_ATOMIC_H__
#define __EASY_ATOMIC_H__

#include "easy/base/noncopyable.h"

namespace easy
{

template <typename T>
class AtomicInt : noncopyable
{
 public:
  AtomicInt() : value_(0) {}

  AtomicInt(T x) : value_(x) {}

  AtomicInt(const AtomicInt &other) : value_(other.get()) {}

  AtomicInt &operator=(const AtomicInt &other) {}

  T fetchAndAdd(T x) { return __sync_fetch_and_add(&value_, x); }

  T addAndFetch(T x) { return __sync_add_and_fetch(&value_, x); }

  T incrementAndFetch() { return __sync_add_and_fetch(&value_, 1); }

  T decrementAndFetch() { return __sync_sub_and_fetch(&value_, 1); }

  bool compareAndSet(T oldValue, T newValue)
  {
    return __sync_bool_compare_and_swap(&value_, oldValue, newValue);
  }

  // gcc >= 4.7
  void set(T x) { __atomic_store_n(&value_, x, __ATOMIC_SEQ_CST); }

  // gcc >= 4.7
  T get() { return __atomic_load_n(&value_, __ATOMIC_SEQ_CST); }

  void add(T x) { addAndFetch(x); }

  void increment() { incrementAndFetch(); }

  void decrement() { decrementAndFetch(); }

 private:
  volatile T value_;
};

}  // namespace easy

#endif
