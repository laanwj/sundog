#include "util_time.h"

#include <errno.h>
#include <time.h>

int util_usleep(unsigned int us)
{
    struct timespec req;
    struct timespec rem;

    req.tv_sec  = us / 1000000;
    req.tv_nsec = (us % 1000000) * 1000L;
    if (nanosleep(&req, &rem) < 0) {
        return -1;
    }
    return 0;
}
