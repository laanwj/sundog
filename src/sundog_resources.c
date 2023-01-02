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

const void *load_resource(const char *name, size_t *size_out)
{
    *size_out = 0;
    /* XXX could use binary search, if we care. */
    for (size_t idx = 0; idx < ARRAY_SIZE(resource_directory); ++idx) {
        if (strcmp(name, resource_directory[idx].name) == 0) {
            *size_out = resource_directory[idx].size;
            return resource_directory[idx].data;
        }
    }
    return NULL;
}

SDL_RWops *load_resource_sdl(const char *name)
{
    size_t resource_size;
    const void *resource_data = load_resource(name, &resource_size);
    if (!resource_data) {
        return NULL;
    }
    return SDL_RWFromConstMem(resource_data, resource_size);
}

void unload_resource(const void *data)
{
    /* Nothing to do! */
}
