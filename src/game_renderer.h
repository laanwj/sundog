/*
 * Copyright (c) 2023 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
/* Graphics handling: abstract renderer */
#ifndef H_GAME_RENDERER
#define H_GAME_RENDERER

#include <stdint.h>

struct game_renderer {
    /** Draw current frame.
     */
    void (*draw)(struct game_renderer *renderer, const float tint[4]);

    /** Update texture from buffer.
     */
    void (*update_texture)(struct game_renderer *renderer, const uint8_t *buffer);

    /** Update palette from buffer.
     */
    void (*update_palette)(struct game_renderer *renderer, const uint16_t *palette);

    /* Destroy game_renderer instance.
     */
    void (*destroy)(struct game_renderer *renderer);
};

struct game_renderer *new_renderer_basic(void);
struct game_renderer *new_renderer_hq4x(void);
struct game_renderer *new_renderer_hqish(void);

#endif
