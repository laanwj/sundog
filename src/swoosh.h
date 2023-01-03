/*
 * Copyright (c) 2023 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#ifndef H_SWOOSH
#define H_SWOOSH

#include <SDL.h>

struct game_renderer;

/** Load and show FTL swoosh animation. */
void swoosh(SDL_Window *window, struct game_renderer *renderer, const char *frames_path);

#endif
