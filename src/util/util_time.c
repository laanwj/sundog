#include "util_time.h"

#include <SDL.h>

void util_msleep(unsigned int msec)
{
    SDL_Delay(msec);
}
