#include "Mutex.h"
#include "easy/base/Atomic.h"
#include "easy_define.h"

namespace easy
{
MutexLock::MutexLock()
{
  EASY_CHECK(pthread_mutex_init(&mutex_, nullptr));
}
MutexLock::~MutexLock()
{
  EASY_CHECK(pthread_mutex_destroy(&mutex_));
}

void MutexLock::lock()
{
  EASY_CHECK(pthread_mutex_lock(&mutex_));
}

void MutexLock::unlock()
{
  EASY_CHECK(pthread_mutex_unlock(&mutex_));
}

RWLock::RWLock() //: ref_cnt_(0), wait_write_(0)
{
   EASY_CHECK(pthread_rwlock_init(&mutex_, NULL));
}

RWLock::~RWLock()
{
   EASY_CHECK(pthread_rwlock_destroy(&mutex_));
}

void RWLock::rdlock()
{
 EASY_CHECK(pthread_rwlock_rdlock(&mutex_));
//  int cond = 1;
//  while (cond)
//  {
//    int loop = 1;
//    do
//    {
//      int oldv = ref_cnt_.value_;  // uncessary use ref_cnt_.get()
//
//      if (0 <= oldv && 0 == wait_write_.value_)
//      {
//        int newv = oldv + 1;
//
//        if (ref_cnt_.compareAndSet(oldv, newv))
//        {
//          cond = 0;
//          break;
//        }
//      }
//
//      asm("pause");
//      loop <<= 1;
//    } while (loop < 1024);
//
//    EASY_CHECK(sched_yield());
//  }
}

void RWLock::wrlock()
{
 EASY_CHECK(pthread_rwlock_wrlock(&mutex_));
//  int cond = 1;
//
//  wait_write_.increment();
//
//  while (cond)
//  {
//    int loop = 1;
//    do
//    {
//      int oldv = ref_cnt_.value_;
//      if (0 == oldv)
//      {
//        int newv = -1;
//        if (ref_cnt_.compareAndSet(oldv, newv))
//        {
//          cond = 0;
//          break;
//        }
//      }
//
//      asm("pause");
//
//      loop <<= 1;
//    } while (loop < 1024);
//
//    EASY_CHECK(sched_yield());
//  }
//
//  wait_write_.decrement();
}

void RWLock::unlock()
{
 EASY_CHECK(pthread_rwlock_unlock(&mutex_));
}

SpinLock::SpinLock()
{
  EASY_CHECK(pthread_spin_init(&mutex_, 0));
}

SpinLock::~SpinLock()
{
  EASY_CHECK(pthread_spin_destroy(&mutex_));
}

void SpinLock::lock()
{
  EASY_CHECK(pthread_spin_lock(&mutex_));
}

void SpinLock::unlock()
{
   EASY_CHECK(pthread_spin_unlock(&mutex_));
}

Semaphore::Semaphore(uint32_t count)
{
  EASY_CHECK(sem_init(&semaphore_, 0, count));
}

Semaphore::~Semaphore()
{
  EASY_CHECK(sem_destroy(&semaphore_));
}

void Semaphore::wait()
{
  EASY_CHECK(sem_wait(&semaphore_));
}

void Semaphore::notify()
{
  EASY_CHECK(sem_post(&semaphore_));
}

}  // namespace easy
