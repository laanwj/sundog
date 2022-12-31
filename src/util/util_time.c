#include "util_time.h"

#include <errno.h>
#include <time.h>

unsigned int msleep(unsigned int ms)
{
    struct timespec req;
    struct timespec rem;

    req.tv_sec  = ms / 1000;
    req.tv_nsec = (ms % 1000) * 1000000L;
    if (nanosleep(&req, &rem) < 0) {
        if (errno == EINTR) {
            /* interrupted by a signal */
            return (rem.tv_sec * 1000) + (rem.tv_nsec / 1000000L);
        }
        return 0;
    }
    return 0;
}
