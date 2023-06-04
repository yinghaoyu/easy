#ifndef __EASY_MUTEX_H__
#define __EASY_MUTEX_H__

#include "noncopyable.h"

#include <pthread.h>
#include <stdint.h>
#include <semaphore.h>

namespace easy
{
class Mutex : noncopyable
{
 public:
  Mutex();
  ~Mutex();

  void lock();

  void unlock();

 private:
  pthread_mutex_t mutex_;
};

class MutexGuard : noncopyable
{
 public:
  MutexGuard(Mutex &mutex) : mutex_(mutex) { mutex_.lock(); }

  ~MutexGuard() { mutex_.unlock(); }

 private:
  Mutex &mutex_;
};

class Semaphore : noncopyable
{
 public:
  Semaphore(uint32_t count = 0);

  ~Semaphore();

  void wait();

  void notify();

 private:
  sem_t semaphore_;
};

}  // namespace easy

#endif
