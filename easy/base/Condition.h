#ifndef __EASY_CONDITION_H__
#define __EASY_CONDITION_H__

#include "easy/base/Mutex.h"
#include "easy/base/noncopyable.h"

#include <pthread.h>

namespace easy
{
class Condition : noncopyable
{
 public:
  Condition(MutexLock &mutex);

  ~Condition();

  void wait();

  void notify();

  void notifyAll();

 private:
  MutexLock &lock_;
  pthread_cond_t pcond_;
};

}  // namespace easy

#endif
