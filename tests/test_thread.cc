#include "easy/base/Logger.h"
#include "easy/base/Macro.h"
#include "easy/base/Mutex.h"
#include "easy/base/Thread.h"

#include <thread>
#include <vector>

static easy::Logger::ptr logger = EASY_LOG_NAME("system");

static int times = 100000;
static int numThread = 100;

easy::RWLock lock;
static int shared_data = 0;

void reader() {
  for (int i = 0; i < times; ++i) {
    easy::ReadLockGuard l(lock);
  }
}

void writer() {
  for (int i = 0; i < times; i++) {
    easy::WriteLockGuard l(lock);
    ++shared_data;
  }
}

void test_wrlock() {
  std::vector<easy::Thread::ptr> threads;

  for (int i = 0; i < numThread; ++i) {
    easy::Thread::ptr thr1 = std::make_shared<easy::Thread>(reader);
    easy::Thread::ptr thr2 = std::make_shared<easy::Thread>(writer);
    threads.push_back(thr1);
    threads.push_back(thr2);
  }
  for (auto& t : threads) {
    t->join();
  }
  EASY_ASSERT(shared_data == times * numThread);
  EASY_LOG_DEBUG(logger) << "WRLock success";
}

static int count = 0;
easy::MutexLock mutex;
void thread_cb() {
  for (int i = 0; i < times; ++i) {
    easy::MutexLockGuard l(mutex);
    ++count;
  }
}

void test_mutex() {
  std::vector<easy::Thread::ptr> thrs;
  for (int i = 0; i < numThread; ++i) {
    easy::Thread::ptr thr = std::make_shared<easy::Thread>(thread_cb);
    thrs.push_back(thr);
  }
  for (size_t i = 0; i < thrs.size(); ++i) {
    thrs[i]->join();
  }
  EASY_ASSERT(count == times * numThread);
  EASY_LOG_DEBUG(logger) << "MutexLock success";
}

int main() {
  test_mutex();
  test_wrlock();
  return 0;
}
