/*
 * Copyright (c) 2022 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
/* Minimalistic game resource system. */
#ifndef H_SUNDOG_RESOURCES
#define H_SUNDOG_RESOURCES

#include <SDL.h>

/** Load resource by name, as SDL stream handle. */
SDL_RWops *load_resource_sdl(const char *name);

#endif
