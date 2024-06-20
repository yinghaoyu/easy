#include "easy/base/Timestamp.h"

#include <memory.h>
#include <sys/time.h>

namespace easy
{
Timestamp Timestamp::now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec;
    return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}

}  // namespace easy
