#include "easy/base/IOManager.h"
#include "easy/base/Logger.h"

static easy::Logger::ptr logger = ELOG_ROOT();

easy::Timer::ptr s_timer;
void             test_timer()
{
    easy::IOManager iom(2);
    s_timer = iom.addTimer(
        500,
        []() {
            static int i = 0;
            ELOG_INFO(logger) << "hello timer i=" << i;
            if (++i == 3)
            {
                s_timer->reset(1000, true);
            }
            if (i == 10)
            {
                s_timer->cancel();
            }
        },
        true);
}

int main(int argc, char** argv)
{
    test_timer();
    return 0;
}
