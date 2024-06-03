#include "easy/base/Condition.h"
#include "easy/base/Macro.h"

namespace easy
{
Condition::Condition(MutexLock& mutex) : lock_(mutex) { EASY_CHECK(pthread_cond_init(&pcond_, nullptr)); }

Condition::~Condition() { EASY_CHECK(pthread_cond_destroy(&pcond_)); }

void Condition::wait() { EASY_CHECK(pthread_cond_wait(&pcond_, lock_.mutexPtr())); }

void Condition::notify() { EASY_CHECK(pthread_cond_signal(&pcond_)); }

void Condition::notifyAll() { EASY_CHECK(pthread_cond_broadcast(&pcond_)); }
}  // namespace easy
