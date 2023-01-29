/*
 * Copyright (c) 2022 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "sundog_resources.h"

#include "util/memutil.h"

#include <SDL.h>

#include <stdint.h>
#include <string.h>

#include "sundog_resource_data.h"

SDL_RWops *load_resource_sdl(const char *name)
{
    /* XXX could use binary search, if we care. */
    for (size_t idx = 0; idx < ARRAY_SIZE(resource_directory); ++idx) {
        if (strcmp(name, resource_directory[idx].name) == 0) {
            return SDL_RWFromConstMem(resource_directory[idx].data, resource_directory[idx].size);
        }
    }
    /* XXX could fall back to SDL_RWFromFile here. */
    return NULL;
}
