/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
/* Graphics handling: Implements a subset of GEM */
#ifndef H_GAME_SCREEN
#define H_GAME_SCREEN

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define SCREEN_COLORS 16

struct game_screen;

typedef void(game_screen_vblank_func)(struct game_screen *screen, void *arg);

struct game_screen_point {
    int x;
    int y;
    unsigned color;
};

struct game_screen {
    /* v_pline */
    void (*v_pline)(struct game_screen *screen,
        unsigned vr_mode, unsigned line_color, unsigned line_width,
        unsigned count,
        int *coordinates);
    /* v_ellarc */
    void (*v_ellarc)(struct game_screen *screen,
        unsigned vr_mode, unsigned line_color, unsigned line_width,
        int x, int y, int xradius, int yradius,
        int begang, int endang);
    /* vr_recfl */
    void (*vr_recfl)(struct game_screen *screen,
        unsigned vr_mode, unsigned fill_color,
        int x0, int y0, int x1, int y1);
    /* v_show_c */
    void (*v_show_c)(struct game_screen *screen,
        bool visible);
    /* vs_clip */
    void (*vs_clip)(struct game_screen *screen,
        bool enable, int x0, int y0, int x1, int y1);
    /* vro_cpyfm */
    void (*vro_cpyfm)(struct game_screen *screen,
        unsigned vr_mode,
        uint8_t *src, unsigned src_width, unsigned src_height, unsigned src_wdwidth,
        int sx0, int sy0, int sx1, int sy1,
        int dx0, int dy0, int dx1, int dy1);
    /* vrt_cpyfm */
    void (*vrt_cpyfm)(struct game_screen *screen,
        unsigned vr_mode, unsigned col0, unsigned col1,
        const uint8_t *src, unsigned src_width, unsigned src_height, unsigned src_wdwidth,
        int sx0, int sy0, int sx1, int sy1,
        int dx0, int dy0, int dx1, int dy1);
    /* vsc_form */
    void (*vsc_form)(struct game_screen *screen,
        uint16_t *mform);
    /* vq_mouse */
    void (*vq_mouse)(struct game_screen *screen,
        unsigned *buttons, int *x, int *y);
    /* set_color */
    void (*set_color)(struct game_screen *screen,
        unsigned index, unsigned color);
    /* draw_image */
    void (*draw_image)(struct game_screen *screen,
        uint8_t *src, int src_width, int src_height,
        int x, int y);
    /* get_image - return (read only) pointer to screen */
    void (*get_image)(struct game_screen *screen,
        int sx, int sy, int width, int height,
        const uint8_t **image_ptr, unsigned *bytes_per_line);
    /* draw_sprite - draw a masked image of up to 8x8 */
    void (*draw_sprite)(struct game_screen *screen,
        int x, int y, const uint8_t *pattern,
        const uint8_t *colors,
        int width, int height, unsigned bytes_per_line);
    /* move screen contents */
    void (*move)(struct game_screen *screen,
        int dx, int dy, int sx, int sy,
        int width, int height);
    /* vblank interrupt - should be called at approximately 50Hz
     * in the interpreter thread.
     */
    void (*vblank_interrupt)(struct game_screen *screen);
    /* add_vblank_callback - will be run in interpreter thread.
     */
    void (*add_vblank_cb)(struct game_screen *screen, game_screen_vblank_func *f, void *arg);
    /* Draw a series of points.
     * Not affected by the clip rectangle, although points are clipped to screen.
     * vr_mode should be either 1 (overwrite) or 3 (xor).
     */
    void (*draw_points)(struct game_screen *screen, unsigned vr_mode, struct game_screen_point *points, unsigned npoints);
    /* Destroy game_screen instance.
     */
    void (*destroy)(struct game_screen *screen);
};

struct game_screen *new_game_screen(void);

/* internal - communication between render/input thread
 * and interpreter thread
 */

/** This gets passed the buffer and palette respectively, when dirty. */
typedef void(update_texture_func)(void *data, const uint8_t *buffer);
typedef void(update_palette_func)(void *data, const uint16_t *palette);

bool game_sdlscreen_update_textures(struct game_screen *screen, void *data, update_texture_func *update_texture, update_palette_func *update_palette);
void game_sdlscreen_update_cursor(struct game_screen *screen, void **cursor);

/** Save screen state to fd (return 0 on success) */
extern int game_sdlscreen_save_state(struct game_screen *b, int fd);

/** Load screen state from fd (return 0 on success) */
extern int game_sdlscreen_load_state(struct game_screen *b, int fd);

/** Set whether to bypass SDL input */
extern void game_sdlscreen_set_input_bypass(struct game_screen *b, bool input_bypass);

/** Update input viewport */
extern void game_sdlscreen_update_viewport(struct game_screen *b, int viewport[4]);

#ifdef __cplusplus
}
#endif

#endif
