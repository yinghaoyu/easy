#ifndef __EASY_MUTEX_H__
#define __EASY_MUTEX_H__

#include "noncopyable.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

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

class MutexLockGuard : noncopyable
{
 public:
  MutexLockGuard(Mutex &mutex) : mutex_(mutex) { mutex_.lock(); }

  ~MutexLockGuard() { mutex_.unlock(); }

 private:
  Mutex &mutex_;
};

class RWMutex : noncopyable
{
 public:
  RWMutex();
  ~RWMutex();

  void rdlock();
  void wrlock();
  void unlock();

 private:
  pthread_rwlock_t mutex_;
};

class ReadLockGuard : noncopyable
{
 public:
  ReadLockGuard(RWMutex &mutex) : mutex_(mutex) { mutex_.rdlock(); }

  ~ReadLockGuard() { mutex_.unlock(); }

  void lock() { mutex_.rdlock(); }

  void unlock() { mutex_.unlock(); }

 private:
  RWMutex &mutex_;
};

class WriteLockGuard : noncopyable
{
 public:
  WriteLockGuard(RWMutex &mutex) : mutex_(mutex) { mutex_.wrlock(); }

  ~WriteLockGuard() { mutex_.unlock(); }

  void lock() { mutex_.wrlock(); }

  void unlock() { mutex_.unlock(); }

 private:
  RWMutex &mutex_;
};

class Spinlock : noncopyable
{
 public:
  Spinlock();

  ~Spinlock();

  void lock();

  void unlock();

 private:
  pthread_spinlock_t m_mutex;
};

class SpinLockGuard : noncopyable
{
 public:
  SpinLockGuard(Spinlock &mutex) : mutex_(mutex) { mutex_.lock(); }

  ~SpinLockGuard() { mutex_.unlock(); }

 private:
  Spinlock &mutex_;
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
