#include "Mutex.h"
#include "easy_define.h"

namespace easy
{
Mutex::Mutex()
{
  EASY_CHECK(pthread_mutex_init(&mutex_, nullptr));
}
Mutex::~Mutex()
{
  EASY_CHECK(pthread_mutex_destroy(&mutex_));
}

void Mutex::lock()
{
  EASY_CHECK(pthread_mutex_lock(&mutex_));
}

void Mutex::unlock()
{
  EASY_CHECK(pthread_mutex_unlock(&mutex_));
}

RWMutex::RWMutex()
{
  EASY_CHECK(pthread_rwlock_init(&mutex_, NULL));
}

RWMutex::~RWMutex()
{
  EASY_CHECK(pthread_rwlock_destroy(&mutex_));
}

void RWMutex::rdlock()
{
  EASY_CHECK(pthread_rwlock_rdlock(&mutex_));
}

void RWMutex::wrlock()
{
  EASY_CHECK(pthread_rwlock_wrlock(&mutex_));
}

void RWMutex::unlock()
{
  EASY_CHECK(pthread_rwlock_unlock(&mutex_));
}

Spinlock::Spinlock()
{
  EASY_CHECK(pthread_spin_init(&m_mutex, 0));
}

Spinlock::~Spinlock()
{
  EASY_CHECK(pthread_spin_destroy(&m_mutex));
}

void Spinlock::lock()
{
  EASY_CHECK(pthread_spin_lock(&m_mutex));
}

void Spinlock::unlock()
{
  EASY_CHECK(pthread_spin_unlock(&m_mutex));
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
