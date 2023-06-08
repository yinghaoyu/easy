#ifndef __EASY_MUTEX_H__
#define __EASY_MUTEX_H__

#include "easy/base/Atomic.h"
#include "easy/base/noncopyable.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

namespace easy
{
class MutexLock : noncopyable
{
 public:
  MutexLock();
  ~MutexLock();

  void lock();

  void unlock();

  pthread_mutex_t *mutexPtr() { return &mutex_; }

 private:
  pthread_mutex_t mutex_;
};

class MutexLockGuard : noncopyable
{
 public:
  MutexLockGuard(MutexLock &mutex) : mutex_(mutex)
  {
    if (!locked_)
    {
      mutex_.lock();
      locked_ = true;
    }
  }

  ~MutexLockGuard()
  {
    if (locked_)
    {
      locked_ = false;
      mutex_.unlock();
    }
  }

 private:
  MutexLock &mutex_;
  bool locked_{false};
};

class RWLock : noncopyable
{
 public:
  RWLock();
  ~RWLock();

  void rdlock();
  void wrlock();
  void unlock();

 private:
  pthread_rwlock_t mutex_;
};

class ReadLockGuard : noncopyable
{
 public:
  ReadLockGuard(RWLock &mutex) : mutex_(mutex) { lock(); }

  ~ReadLockGuard() { unlock(); }

  void lock()
  {
    if (!locked_)
    {
      mutex_.rdlock();
      locked_ = true;
    }
  }

  void unlock()
  {
    if (locked_)
    {
      locked_ = false;
      mutex_.unlock();
    }
  }

 private:
  RWLock &mutex_;
  bool locked_{false};
};

class WriteLockGuard : noncopyable
{
 public:
  WriteLockGuard(RWLock &mutex) : mutex_(mutex) { lock(); }

  ~WriteLockGuard() { unlock(); }

  void lock()
  {
    if (!locked_)
    {
      mutex_.wrlock();
      locked_ = true;
    }
  }

  void unlock()
  {
    if (locked_)
    {
      locked_ = false;
      mutex_.unlock();
    }
  }

 private:
  RWLock &mutex_;
  bool locked_{false};
};

class SpinLock : noncopyable
{
 public:
  SpinLock();

  ~SpinLock();

  void lock();

  void unlock();

 private:
  pthread_spinlock_t mutex_;
};

class SpinLockGuard : noncopyable
{
 public:
  SpinLockGuard(SpinLock &mutex) : mutex_(mutex)
  {
    if (!locked_)
    {
      mutex_.lock();
      locked_ = true;
    }
  }

  ~SpinLockGuard()
  {
    if (locked_)
    {
      locked_ = false;
      mutex_.unlock();
    }
  }

 private:
  SpinLock &mutex_;
  bool locked_{false};
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
