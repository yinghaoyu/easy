#include "easy/base/Logger.h"
#include "easy/base/Macro.h"
#include "easy/base/Scheduler.h"

static easy::Logger::ptr logger = ELOG_ROOT();

void test_fiber()
{
    static int s_count = 5;
    ELOG_INFO(logger) << "test in fiber s_count=" << s_count;
    if (--s_count >= 0)
    {
        easy::Scheduler::GetThis()->schedule(&test_fiber, easy::Thread::GetCurrentThreadId());
    }
}

int main(int argc, char** argv)
{
    ELOG_INFO(logger) << "main";
    easy::Scheduler sc(6, false, "test");
    sc.start();
    // sleep(2);
    ELOG_INFO(logger) << "schedule";
    sc.schedule(&test_fiber);
    sc.stop();
    ELOG_INFO(logger) << "over";
    return 0;
}
