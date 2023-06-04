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
  pthread_mutex_destroy(&mutex_);
}

void Mutex::lock()
{
  pthread_mutex_lock(&mutex_);
}

void Mutex::unlock()
{
  pthread_mutex_unlock(&mutex_);
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
