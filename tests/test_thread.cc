#include "easy/base/Mutex.h"
#include "easy/base/Thread.h"
#include "easy/base/easy_define.h"

static easy::Logger::ptr logger = EASY_LOG_NAME("system");

static int count = 0;

static int times = 100;

easy::Mutex mutex;

void thread_cb()
{
  for (int i = 0; i < times; ++i)
  {
    easy::MutexGuard lock(mutex);
    EASY_LOG_DEBUG(logger) << count;
    ++count;
  }
}

int main()
{
  std::vector<easy::Thread::ptr> thrs;
  int numThread = 100;
  for (int i = 0; i < numThread; ++i)
  {
    easy::Thread::ptr thr(
        new easy::Thread(&thread_cb, "name_" + std::to_string(i * 2)));
    thrs.push_back(thr);
  }

  for (size_t i = 0; i < thrs.size(); ++i)
  {
    thrs[i]->join();
  }

  EASY_ASSERT(count == times * numThread);

  return 0;
}
