#include "easy/base/Logger.h"
#include "easy/base/Scheduler.h"
#include "easy/base/easy_define.h"

static easy::Logger::ptr logger = EASY_LOG_ROOT();

void test_fiber()
{
  static int s_count = 5;
  EASY_LOG_INFO(logger) << "test in fiber s_count=" << s_count;
  if (--s_count >= 0)
  {
    easy::Scheduler::GetThis()->schedule(
        &test_fiber, easy::Thread::GetCurrentThreadId());
  }
}

int main(int argc, char **argv)
{
  EASY_LOG_INFO(logger) << "main";
  easy::Scheduler sc(6, false, "test");
  sc.start();
  // sleep(2);
  EASY_LOG_INFO(logger) << "schedule";
  sc.schedule(&test_fiber);
  sc.stop();
  EASY_LOG_INFO(logger) << "over";
  return 0;
}
