/*
 * Copyright (c) 2023 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "swoosh.h"

#include "game_renderer.h"
#include "glutil.h"
#include "sundog_resources.h"
#include "util/memutil.h"

#include <SDL.h>

#include <assert.h>

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

static struct {
    const char *name;
    int duration;
} swoosh_frames[] = {
    { "swoosh/frame000.bmp", 10 },
    { "swoosh/frame001.bmp", 1 },
    { "swoosh/frame002.bmp", 1 },
    { "swoosh/frame003.bmp", 2 },
    { "swoosh/frame004.bmp", 2 },
    { "swoosh/frame005.bmp", 3 },
    { "swoosh/frame006.bmp", 3 },
    { "swoosh/frame007.bmp", 3 },
    { "swoosh/frame008.bmp", 3 },
    { "swoosh/frame009.bmp", 12 },
    { "swoosh/frame010.bmp", 40 },
};

/** Load and show FTL swoosh animation. */
void swoosh(SDL_Window *window, struct game_renderer *renderer)
{
    /* Setup GL render and clear window to black. */
    int width, height;
    SDL_GL_GetDrawableSize(window, &width, &height);

    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glClearColor(0.0, 0.0, 0.0, 0.0);

    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapWindow(window);

    uint8_t image[SCREEN_WIDTH * SCREEN_HEIGHT];
    uint8_t palette[16][4];
    uint16_t palette_st[16];

    for (size_t idx = 0; idx < ARRAY_SIZE(swoosh_frames); ++idx) {
        /* Load frame */
        load_paletted(swoosh_frames[idx].name, image, SCREEN_WIDTH, SCREEN_HEIGHT, &palette[0][0]);
        renderer->update_texture(renderer, image);
        /* Here we actually convert the palette back to Atari ST paletter colors. */
        for (int x = 0; x < 16; ++x) {
            palette_st[x] = ((palette[x][0] >> 5) << 8)
                | ((palette[x][1] >> 5) << 4)
                | ((palette[x][2] >> 5) << 0);
        }
        renderer->update_palette(renderer, palette_st);

        SDL_GL_GetDrawableSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
        gl_viewport_fixed_ratio(width, height, SCREEN_WIDTH, SCREEN_HEIGHT);

        const float tint[4] = { 1.0, 1.0, 1.0, 1.0 };
        renderer->draw(renderer, tint);
        SDL_GL_SwapWindow(window);

        /* Sleep after frame */
        SDL_Delay(swoosh_frames[idx].duration * 1000 / 50);
        SDL_PumpEvents();
    }
}
