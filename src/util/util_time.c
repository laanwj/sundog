#include "util_time.h"

#ifdef _WIN32
/* https://stackoverflow.com/questions/5801813/c-usleep-is-obsolete-workarounds-for-windows-mingw */
#include <windows.h>

int util_usleep(unsigned int usec)
{
    HANDLE timer;
    LARGE_INTEGER ft;

    ft.QuadPart = -(10*usec); // Convert to 100 nanosecond interval, negative value indicates relative time

    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0); 
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
    return 0;
}
#else
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
#endif
