#include "easy/base/Fiber.h"
#include "easy/base/Logger.h"

static easy::Logger::ptr logger = EASY_LOG_ROOT();

void run_in_fiber() {
  EASY_LOG_INFO(logger) << "run_in_fiber begin";
  easy::Fiber::YieldToHold();
  EASY_LOG_INFO(logger) << "run_in_fiber resume";
  easy::Fiber::YieldToHold();
  EASY_LOG_INFO(logger) << "run_in_fiber end";
}

void test_fiber() {
  easy::Fiber::GetThis();
  EASY_LOG_INFO(logger) << "main begin";
  easy::Fiber::ptr fiber(easy::NewFiber(run_in_fiber), easy::FreeFiber);
  fiber->resume();
  EASY_LOG_INFO(logger) << "main after resume";
  fiber->resume();
  EASY_LOG_INFO(logger) << "main after resume";
  fiber->resume();
  EASY_LOG_INFO(logger) << "main after end";
}

int main() {
  test_fiber();
  return 0;
}
