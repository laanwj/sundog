/*
 * Copyright (c) 2023 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "swoosh.h"

#include "game_renderer.h"
#include "glutil.h"
#include "sundog_resources.h"

#include <SDL.h>

#include <assert.h>

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

static const size_t swoosh_nframes          = 11;
static const int swoosh_frame_durations[11] = { 10, 1, 1, 2, 2, 3, 3, 3, 3, 12, 40 };

/** Load and show FTL swoosh animation. */
void swoosh(SDL_Window *window, struct game_renderer *renderer, const char *frames_path)
{
    size_t idx;
    size_t slen;
#define BUFLEN (200)
    char temp_path[BUFLEN];

    /* Setup GL render and clear window to black. */
    int width, height;
    SDL_GetWindowSize(window, &width, &height);

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

    for (idx = 0; idx < swoosh_nframes; ++idx) {
        /* Load frame */
        char istr[4];
        istr[0] = '0' + ((idx / 100) % 10);
        istr[1] = '0' + ((idx / 10) % 10);
        istr[2] = '0' + (idx % 10);
        istr[3] = 0;

        strncpy(temp_path, frames_path, BUFLEN);
        if (temp_path[BUFLEN - 1]) {
            goto error;
        }
        slen = strlen(temp_path);
        if (slen > 0 && temp_path[slen - 1] != '/') {
            strncat(temp_path, "/", BUFLEN - 1);
        }
        strncat(temp_path, "frame", BUFLEN - 1);
        strncat(temp_path, istr, BUFLEN - 1);
        strncat(temp_path, ".bmp", BUFLEN - 1);
        if (temp_path[BUFLEN - 2]) {
            goto error;
        }

        load_paletted(temp_path, image, SCREEN_WIDTH, SCREEN_HEIGHT, &palette[0][0]);
        renderer->update_texture(renderer, image);
        /* Here we actually convert the palette back to Atari ST paletter colors. */
        for (int x = 0; x < 16; ++x) {
            palette_st[x] = ((palette[x][0] >> 5) << 8)
                | ((palette[x][1] >> 5) << 4)
                | ((palette[x][2] >> 5) << 0);
        }
        renderer->update_palette(renderer, palette_st);

        SDL_GetWindowSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
        gl_viewport_fixed_ratio(width, height, SCREEN_WIDTH, SCREEN_HEIGHT);

        const float tint[4] = { 1.0, 1.0, 1.0, 1.0 };
        renderer->draw(renderer, tint);
        SDL_GL_SwapWindow(window);

        /* Sleep after frame */
        SDL_Delay(swoosh_frame_durations[idx] * 1000 / 50);
        SDL_PumpEvents();
    }
error:;
}
