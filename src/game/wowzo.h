/*
 * Copyright (c) 2022 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
/* Warp/hyperspace effect. */
#ifndef H_GAME_WOWZO
#define H_GAME_WOWZO

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct game_screen;
struct game_sound;

void wowzo(struct game_screen *screen, struct game_sound *sound, bool warp_failed, uint16_t distance, uint16_t seed);

#ifdef __cplusplus
}
#endif

#endif
