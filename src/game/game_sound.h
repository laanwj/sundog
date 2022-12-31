/*
 * Copyright (c) 2022 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
/* Sound handling: Handle specific ST DoSound commands.. */
#ifndef H_GAME_SOUND
#define H_GAME_SOUND

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct game_sound {
    /** Play sound. The input is in XBIOS DoSound format: http://toshyp.atari.org/en/004011.html#Dosound
     */
    void (*play_sound)(struct game_sound *sound,
        const uint8_t *data, size_t len);

    /* Destroy game_sound instance.
     */
    void (*destroy)(struct game_sound *sound);
};

/** Create a new game_sound instance using SDL.
 */
struct game_sound *new_sdl_sound(const char *samples_path);

#ifdef __cplusplus
}
#endif

#endif
