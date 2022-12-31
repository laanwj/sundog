/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#ifndef H_GAME_GEMBIND
#define H_GAME_GEMBIND

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct game_screen;
struct game_sound;
struct psys_state;

/** Construction */
extern struct psys_binding *new_gembind(struct psys_state *state, struct game_screen *screen, struct game_sound *sound);

/** Destruction */
extern void destroy_gembind(struct psys_binding *b);

#ifdef __cplusplus
}
#endif

#endif
