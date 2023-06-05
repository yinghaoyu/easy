#ifndef __EASY_MUTEX_H__
#define __EASY_MUTEX_H__

#include "noncopyable.h"
#include "Atomic.h"

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

 private:
  pthread_mutex_t mutex_;
};

class MutexLockGuard : noncopyable
{
 public:
  MutexLockGuard(MutexLock &mutex) : mutex_(mutex) { mutex_.lock(); }

  ~MutexLockGuard() { mutex_.unlock(); }

 private:
  MutexLock &mutex_;
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
  ReadLockGuard(RWLock &mutex) : mutex_(mutex) { mutex_.rdlock(); }

  ~ReadLockGuard() { mutex_.unlock(); }

  void lock() { mutex_.rdlock(); }

  void unlock() { mutex_.unlock(); }

 private:
  RWLock &mutex_;
};

class WriteLockGuard : noncopyable
{
 public:
  WriteLockGuard(RWLock &mutex) : mutex_(mutex) { mutex_.wrlock(); }

  ~WriteLockGuard() { mutex_.unlock(); }

  void lock() { mutex_.wrlock(); }

  void unlock() { mutex_.unlock(); }

 private:
  RWLock &mutex_;
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
  SpinLockGuard(SpinLock &mutex) : mutex_(mutex) { mutex_.lock(); }

  ~SpinLockGuard() { mutex_.unlock(); }

 private:
  SpinLock &mutex_;
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
