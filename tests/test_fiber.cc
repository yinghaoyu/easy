#include "easy/base/Fiber.h"
#include "easy/base/Logger.h"

static easy::Logger::ptr logger = ELOG_ROOT();

void run_in_fiber()
{
    ELOG_INFO(logger) << "run_in_fiber begin";
    easy::Fiber::YieldToHold();
    ELOG_INFO(logger) << "run_in_fiber resume";
    easy::Fiber::YieldToHold();
    ELOG_INFO(logger) << "run_in_fiber end";
}

void test_fiber()
{
    easy::Fiber::GetThis();
    ELOG_INFO(logger) << "main begin";
    easy::Fiber::ptr fiber(easy::NewFiber(run_in_fiber), easy::FreeFiber);
    fiber->resume();
    ELOG_INFO(logger) << "main after resume";
    fiber->resume();
    ELOG_INFO(logger) << "main after resume";
    fiber->resume();
    ELOG_INFO(logger) << "main after end";
}

int main()
{
    test_fiber();
    return 0;
}
