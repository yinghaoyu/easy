#include "easy/base/Coroutine.h"
#include "easy/base/Logger.h"

static easy::Logger::ptr logger = EASY_LOG_ROOT();

void run_in_coroutine()
{
  EASY_LOG_INFO(logger) << "run_in_fiber begin";
  easy::Coroutine::YieldToHold();
  EASY_LOG_INFO(logger) << "run_in_fiber resume";
  easy::Coroutine::YieldToHold();
  EASY_LOG_INFO(logger) << "run_in_fiber end";
}

void test_coroutine()
{
  easy::Coroutine::GetThis();
  EASY_LOG_INFO(logger) << "main begin";
  easy::Coroutine::ptr coroutine(easy::NewCoroutine(run_in_coroutine),
                                 easy::FreeCoroutine);
  coroutine->resume();
  EASY_LOG_INFO(logger) << "main after resume";
  coroutine->resume();
  EASY_LOG_INFO(logger) << "main after resume";
  coroutine->resume();
  EASY_LOG_INFO(logger) << "main after end";
}

int main()
{
  test_coroutine();
  return 0;
}
