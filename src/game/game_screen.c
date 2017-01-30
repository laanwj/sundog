/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "game_screen.h"

#include "psys/psys_debug.h"
#include "util/memutil.h"
#include "util/util_img.h"
#include "util/util_save_state.h"

#include "SDL.h"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

/* Header for savestates */
#define GAME_SDLSCREEN_STATE_ID 0x53444c53

/** Rectangle structure used for clipping rectangle.
 */
struct rect {
    int x0, y0, x1, y1;
};

static const struct rect fullscreen = { 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1 };

/** Cursor info */
#define CURSOR_WIDTH 32
#define CURSOR_SIZE (CURSOR_WIDTH * CURSOR_WIDTH / 8)

/** SDL screen implementation. We implement our own line and arc drawing
 * functions instead of rendering to a texture using OpenGL because
 * - The number of draws is so low, that the overhead of doing it in software
 *   is minimal.
 * - We need to be able to read back the screen for sprite collision detection.
 * - The interpreter runs in its own thread, and synchronization is much easier
 *   if we don't have to take cross-thread OpenGL rendering into account.
 */
struct sdl_screen {
    struct game_screen base;
    SDL_mutex *mutex;

    /** The screen - represented as a simple grid of pixels, one byte per
     * pixel, with a pointer to every row for easy access. Only indexes 0-15
     * are valid.
     */
    uint8_t *rows[SCREEN_HEIGHT];
    uint8_t buffer[SCREEN_WIDTH * SCREEN_HEIGHT];
    bool buffer_dirty;

    /** 16-color palette.
     */
    uint16_t palette[SCREEN_COLORS];
    bool palette_dirty;

    /** Mouse cursor information.
     */
    uint8_t cursor_data[CURSOR_SIZE];
    uint8_t cursor_mask[CURSOR_SIZE];
    int cursor_hot_x, cursor_hot_y;
    bool cursor_dirty;

    /* Clipping rectangle: seems inclusive, in both directions, although I've
     * been unable to find GEM documentation on this.
     */
    struct rect clip;

    game_screen_vblank_func *vblank_cb;
    void *vblank_cb_arg;
};

static inline struct sdl_screen *sdl_screen(struct game_screen *base)
{
    return (struct sdl_screen *)base;
}

/** Draw a pixel, taking vr_mode into account, the GEM drawing mode
 * for B/W.
 */
static inline void draw_pixel(unsigned vr_mode, uint8_t *drow, unsigned dx, unsigned bit, unsigned col0, unsigned col1)
{
    unsigned col = bit ? col1 : col0;
    switch (vr_mode) {
    case 1:
        drow[dx] = col;
        break; /* Replace */
    case 2:
        if (bit) {
            drow[dx] = col;
        }
        break; /* Transparent */
    case 3:
        drow[dx] ^= col;
        break; /* XOR */
    case 4:
        if (!bit) {
            drow[dx] = col;
        }
        break; /* Inverse transparent */
    }
}

/* Naive fixed-bit line drawing implementation: ignore line width for now,
 * I'm not sure what non-width-1 lines are used for in Sundog if anything.
 */
static void draw_line(uint8_t **rows, int x0, int y0, int x1, int y1, unsigned vr_mode, uint8_t color, unsigned line_width,
    struct rect *clip)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int cx = (x0 << 16) + 0x8000;
    int cy = (y0 << 16) + 0x8000;
    int slopex, slopey;
    int i, n;
#if 0
    psys_debug("  line %d %d %d %d vr_mode %d color %d width %d\n", x0, y0, x1, y1, vr_mode, color, line_width);
#endif
    if (dx < dy) {
        slopex = ((x1 - x0) << 16) / dy;
        slopey = (y0 < y1) ? (1 << 16) : (-1 << 16);
        n      = dy;
    } else {
        slopex = (x0 < x1) ? (1 << 16) : (-1 << 16);
        slopey = ((y1 - y0) << 16) / dx;
        n      = dx;
    }
    for (i = 0; i <= n; ++i) {
        int xx = cx >> 16, yy = cy >> 16;
        if (yy >= clip->y0 && yy <= clip->y1 && xx >= clip->x0 && xx <= clip->x1) {
            draw_pixel(vr_mode, rows[yy], xx, 1, 0, color);
        }
        cx += slopex;
        cy += slopey;
    }
}

/** Draw an arc segment.
 */
static void draw_arc(uint8_t **rows,
    unsigned vr_mode, unsigned color, unsigned line_width,
    int x, int y, int xradius, int yradius,
    int begang_, int endang_,
    struct rect *clip)
{
    float theta  = begang_ / 3600.0 * 2.0 * M_PI;
    float endang = endang_ / 3600.0 * 2.0 * M_PI;
    float step;
    int i, steps;
    int maxradius = xradius < yradius ? yradius : xradius;
    /* Approximate necessary number of steps based on circumference in pixels */
    steps = 2 * M_PI * maxradius * (endang_ - begang_) / 3600.0 + 0.5;
    /* Round up to make sure the arc is symmetric in four quadrants */
    steps = ((steps - 1) / 4) * 4 + 1;
    if (steps <= 0)
        return;
    step = (endang - theta) / (steps - 1);
    for (i = 0; i < steps; ++i) {
        int xx = (int)(x + 0.5f + xradius * sin(theta));
        int yy = (int)(y + 0.5f + yradius * cos(theta));
        if (yy >= clip->y0 && yy <= clip->y1 && xx >= clip->x0 && xx <= clip->x1) {
            draw_pixel(vr_mode, rows[yy], xx, 1, 0, color);
        }
        theta += step;
    }
}

static void sdlscreen_v_pline(struct game_screen *screen_,
    unsigned vr_mode, unsigned line_color, unsigned line_width,
    unsigned count,
    int *coordinates)
{
    struct sdl_screen *screen = sdl_screen(screen_);
    unsigned i;
#if 0
    psys_debug("screen v_pline vr=%d col=%d width=%d count=%d\n", vr_mode, line_color, line_width, count);
#endif
    SDL_LockMutex(screen->mutex);
    for (i = 1; i < count; ++i) {
        draw_line(screen->rows, coordinates[i * 2 - 2], coordinates[i * 2 - 1], coordinates[i * 2 + 0], coordinates[i * 2 + 1],
            vr_mode, line_color, line_width, &screen->clip);
    }
    screen->buffer_dirty = true;
    SDL_UnlockMutex(screen->mutex);
}

static void sdlscreen_v_ellarc(struct game_screen *screen_,
    unsigned vr_mode, unsigned line_color, unsigned line_width,
    int x, int y, int xradius, int yradius,
    int begang, int endang)
{
    struct sdl_screen *screen = sdl_screen(screen_);
#if 0
    psys_debug("screen v_ellarc vr=%d col=%d width=%d (%d,%d) (%d,%d) (%d,%d)\n",
        vr_mode, line_color, line_width,
        x, y, xradius, yradius,
        begang, endang);
#endif
    SDL_LockMutex(screen->mutex);
    draw_arc(screen->rows, vr_mode, line_color, line_width, x, y, xradius, yradius, begang, endang, &screen->clip);
    screen->buffer_dirty = true;
    SDL_UnlockMutex(screen->mutex);
}

static void sdlscreen_vr_recfl(struct game_screen *screen_,
    unsigned vr_mode, unsigned fill_color,
    int dx0, int dy0, int dx1, int dy1)
{
    struct sdl_screen *screen = sdl_screen(screen_);
    int dx, dy;
#if 0
    psys_debug("screen vr_recfl vr=%d col=%d %d,%d %d,%d\n", vr_mode, fill_color, dx0, dy0, dx1, dy1);
#endif

    SDL_LockMutex(screen->mutex);
    for (dy = dy0; dy <= dy1; ++dy) {
        if (dy < screen->clip.y0 || dy > screen->clip.y1) {
            continue;
        }
        uint8_t *drow = screen->rows[dy];
        for (dx = dx0; dx <= dx1; ++dx) {
            if (dx < screen->clip.x0 || dx > screen->clip.x1) {
                continue;
            }
            draw_pixel(vr_mode, drow, dx, 1, 0, fill_color);
        }
    }
    screen->buffer_dirty = true;
    SDL_UnlockMutex(screen->mutex);
}

static void sdlscreen_v_show_c(struct game_screen *screen_,
    bool visible)
{
    struct sdl_screen *screen = sdl_screen(screen_);
    (void)screen;
    /* Mouse hide/show is ignored. On Atari ST this is done before
     * and after every draw operation, but on modern hardware with hardware
     * cursors and overlays there is no reason to.
     */
}

static void sdlscreen_vs_clip(struct game_screen *screen_,
    bool enable, int x0, int y0, int x1, int y1)
{
    struct sdl_screen *screen = sdl_screen(screen_);
#if 0
    psys_debug("screen vs_clip %d %d,%d %d,%d\n", enable, x0, y0, x1, y1);
#endif
    if (!enable) {
        screen->clip.x0 = 0;
        screen->clip.y0 = 0;
        screen->clip.x1 = SCREEN_WIDTH - 1;
        screen->clip.y1 = SCREEN_HEIGHT - 1;
    } else {
        screen->clip.x0 = x0;
        screen->clip.y0 = y0;
        screen->clip.x1 = x1;
        screen->clip.y1 = y1;
    }
}

static void sdlscreen_vro_cpyfm(struct game_screen *screen_,
    unsigned vr_mode,
    uint8_t *src, unsigned src_width, unsigned src_height, unsigned src_wdwidth,
    int sx0, int sy0, int sx1, int sy1,
    int dx0, int dy0, int dx1, int dy1)
{
    struct sdl_screen *screen = sdl_screen(screen_);
    int dx, dy;
#if 0
    psys_debug("screen vro_cpyfm vr=%d %p[%d %d %d] (%d,%d,%d,%d) -> (%d,%d,%d,%d)\n",
            vr_mode,
            src, src_width, src_height, src_wdwidth,
            sx0, sy0, sx1, sy1,
            dx0, dy0, dx1, dy1);
#endif
    SDL_LockMutex(screen->mutex);
    /* If the sizes of both rasters don't match, then the size of the source raster
     * will be used.
     */
    if ((sx1 - sx0) != (dx1 - dx0) || (sy1 - sy0) != (dy1 - dy0)) {
        dx1 = dx0 + (sx1 - sx0);
        dy1 = dy0 + (sy1 - sy0);
    }
    /* Unplanarize and draw color data */
    for (dy = dy0; dy <= dy1; ++dy) {
        if (dy < screen->clip.y0 || dy > screen->clip.y1) {
            continue;
        }
        unsigned sofs = (dy - dy0 + sy0) * src_wdwidth * 8;
        uint8_t *drow = screen->rows[dy];
        for (dx = dx0; dx <= dx1; ++dx) {
            unsigned sx = (dx - dx0 + sx0);
            unsigned bit0, bit1, bit2, bit3;
            unsigned bitid, byteid, unitofs, pixel;
            if (dx < screen->clip.x0 || dx > screen->clip.x1) {
                continue;
            }
            /* fetch bit from all four planes and combine pixel */
            bitid   = ~sx & 7;
            byteid  = (sx >> 3) & 1;
            unitofs = sofs + (sx >> 4) * 8;
            bit0    = (src[unitofs + 0 + byteid] >> bitid) & 1;
            bit1    = (src[unitofs + 2 + byteid] >> bitid) & 1;
            bit2    = (src[unitofs + 4 + byteid] >> bitid) & 1;
            bit3    = (src[unitofs + 6 + byteid] >> bitid) & 1;
            pixel   = (bit3 << 3) | (bit2 << 2) | (bit1 << 1) | bit0;
            switch (vr_mode) { /* These are different from vr_mode for other commands */
            case 6:            /* S_XOR_D - is used for dragging */
                drow[dx] ^= pixel;
                break;
            case 3:  /* S_ONLY */
            default: /* No others used by the game: shut up and write */
                drow[dx] = pixel;
                break;
            }
        }
    }
    screen->buffer_dirty = true;
    SDL_UnlockMutex(screen->mutex);
}

static void sdlscreen_vrt_cpyfm(struct game_screen *screen_,
    unsigned vr_mode, unsigned col0, unsigned col1,
    const uint8_t *src, unsigned src_width, unsigned src_height, unsigned src_wdwidth,
    int sx0, int sy0, int sx1, int sy1,
    int dx0, int dy0, int dx1, int dy1)
{
    struct sdl_screen *screen = sdl_screen(screen_);
    int dx, dy;
#if 0
    psys_debug("screen vrt_cpyfm vr=%d %p[%d %d %d] (%d,%d,%d,%d), col0 %d col1 %d -> (%d,%d,%d,%d)\n",
            vr_mode,
            src, src_width, src_height, src_wdwidth,
            sx0, sy0, sx1, sy1,
            col0, col1,
            dx0, dy0, dx1, dy1);
#endif
    /* If the dimensions don't match, the size of the source raster and the upper
     * left corner of the destination raster will be used as starting point.
     */
    if ((sx1 - sx0) != (dx1 - dx0) || (sy1 - sy0) != (dy1 - dy0)) {
        dx1 = dx0 + (sx1 - sx0);
        dy1 = dy0 + (sy1 - sy0);
    }
    SDL_LockMutex(screen->mutex);
    /* Draw B/W image data.
     * Every byte in the source image will have 8 pixels, arranged MSB to LSB.
     * The 0/1 states are converted to color depending on col0 and col1 respectively.
     * How these are combined with the destination image depends on vr_mode.
     * This could be optimized a lot, if it matters.
     */
    for (dy = dy0; dy <= dy1; ++dy) {
        if (dy < screen->clip.y0 || dy > screen->clip.y1) {
            continue;
        }
        unsigned sofs = (dy - dy0 + sy0) * src_wdwidth * 2;
        uint8_t *drow = screen->rows[dy];
        for (dx = dx0; dx <= dx1; ++dx) {
            unsigned sx = (dx - dx0 + sx0);
            unsigned bit;
            if (dx < screen->clip.x0 || dx > screen->clip.x1) {
                continue;
            }
            bit = src[sofs + (sx >> 3)] & (1 << ((~sx) & 7));
            draw_pixel(vr_mode, drow, dx, bit, col0, col1);
        }
    }
    screen->buffer_dirty = true;
    SDL_UnlockMutex(screen->mutex);
}

static void sdlscreen_vsc_form(struct game_screen *screen_,
    uint16_t *mform)
{
    struct sdl_screen *screen = sdl_screen(screen_);
    int y;
    psys_debug("screen vsc_form\n");
    SDL_LockMutex(screen->mutex);
    screen->cursor_hot_x = ((int16_t)mform[0]) * 2;
    screen->cursor_hot_y = ((int16_t)mform[1]) * 2;
    /* Convert to SDL format, and blow up 16x16 cursor to 32x32 */
    for (y = 0; y < 16; ++y) {
        uint32_t data = double_bits16(~mform[21 + y] & mform[5 + y]);
        uint32_t mask = double_bits16(mform[5 + y]);
        screen->cursor_data[y * 8 + 0]
            = screen->cursor_data[y * 8 + 4]
            = (data >> 24) & 0xff;
        screen->cursor_data[y * 8 + 1]
            = screen->cursor_data[y * 8 + 5]
            = (data >> 16) & 0xff;
        screen->cursor_data[y * 8 + 2]
            = screen->cursor_data[y * 8 + 6]
            = (data >> 8) & 0xff;
        screen->cursor_data[y * 8 + 3]
            = screen->cursor_data[y * 8 + 7]
            = data & 0xff;
        screen->cursor_mask[y * 8 + 0]
            = screen->cursor_mask[y * 8 + 4] = (mask >> 24) & 0xff;
        screen->cursor_mask[y * 8 + 1]
            = screen->cursor_mask[y * 8 + 5] = (mask >> 16) & 0xff;
        screen->cursor_mask[y * 8 + 2]
            = screen->cursor_mask[y * 8 + 6] = (mask >> 8) & 0xff;
        screen->cursor_mask[y * 8 + 3]
            = screen->cursor_mask[y * 8 + 7] = mask & 0xff;
    }
    screen->cursor_dirty = true;
    SDL_UnlockMutex(screen->mutex);
    (void)screen;
}

static void sdlscreen_vq_mouse(struct game_screen *screen_,
    unsigned *buttons, int *x, int *y)
{
    struct sdl_screen *screen = sdl_screen(screen_);
    (void)screen;
#if 0
    /* dummyscreen for psys_compare_trace */
    *buttons = 0;
    *x = 0;
    *y = 0;
#else
    /* TODO: Not allowed to do this from a thread?
     * This seems to work but as these values are updated from the main
     * event loop without synchronization, they may be stale or even corrupted.
     */
    int sx, sy;
    uint32_t sb   = SDL_GetMouseState(&sx, &sy);
    uint32_t bout = 0;
    if (sb & SDL_BUTTON(SDL_BUTTON_LEFT)) {
        bout |= 1;
    }
    if (sb & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
        bout |= 2;
    }
    *x       = sx / 2; /* XXX depend on scaling factor */
    *y       = sy / 2;
    *buttons = bout;
#endif
}

static void sdlscreen_set_color(struct game_screen *screen_,
    unsigned index, unsigned color)
{
    struct sdl_screen *screen = sdl_screen(screen_);
    SDL_LockMutex(screen->mutex);
    screen->palette[index] = color;
    screen->palette_dirty  = true;
    SDL_UnlockMutex(screen->mutex);
}

static void sdlscreen_destroy(struct game_screen *screen_)
{
    struct sdl_screen *screen = sdl_screen(screen_);
    SDL_DestroyMutex(screen->mutex);
    free(screen);
}

static void sdlscreen_draw_image(struct game_screen *screen_,
    uint8_t *src, int src_width, int src_height,
    int x, int y)
{
    struct sdl_screen *screen = sdl_screen(screen_);
    int sy;
    SDL_LockMutex(screen->mutex);
    for (sy = 0; sy < src_height; ++sy) {
        memcpy(screen->rows[sy + y] + x, &src[src_width * sy], src_width);
    }
    screen->buffer_dirty = true;
    SDL_UnlockMutex(screen->mutex);
}

static void sdlscreen_get_image(struct game_screen *screen_,
    int sx, int sy, int width, int height,
    const uint8_t **image_ptr, unsigned *bytes_per_line)
{
    struct sdl_screen *screen = sdl_screen(screen_);
    /* This does not need a mutex because we're the only ones writing
     * to the screen, and will always read back what we wrote. The mutex is for
     * synchronizing with the render thread, something we don't have to do here.
     */
    if (sx < 0 || sy < 0 || (sx + width) > SCREEN_WIDTH || (sy + height) > SCREEN_HEIGHT) {
        psys_debug("get_image: out-of-screen access\n");
        return;
    }
    *image_ptr      = screen->rows[sy] + sx;
    *bytes_per_line = SCREEN_WIDTH;
}

static void sdlscreen_draw_sprite(struct game_screen *screen_,
    int x, int y, const uint8_t *pattern,
    const uint8_t *colors,
    int width, int height, unsigned bytes_per_line)
{
    struct sdl_screen *screen = sdl_screen(screen_);
    int cx, cy;
    if (x < 0 || y < 0 || (x + width) > SCREEN_WIDTH || (y + height) > SCREEN_HEIGHT) {
        psys_debug("draw_sprite: out-of-screen access\n");
        return;
    }
    SDL_LockMutex(screen->mutex);
    for (cy = 0; cy < height; ++cy) {
        for (cx = 0; cx < width; ++cx) {
            if (pattern[cy] & (1 << (~cx & 7))) {
                screen->rows[y + cy][x + cx] = colors[bytes_per_line * cy + cx];
            }
        }
    }
    screen->buffer_dirty = true;
    SDL_UnlockMutex(screen->mutex);
}

static void sdlscreen_move(struct game_screen *screen_,
    int dx, int dy, int sx, int sy,
    int width, int height)
{
    struct sdl_screen *screen = sdl_screen(screen_);
    int cx, cy;
    if (dx < 0 || dy < 0 || (dx + width) > SCREEN_WIDTH || (dy + height) > SCREEN_HEIGHT || sx < 0 || sy < 0 || (sx + width) > SCREEN_WIDTH || (sy + height) > SCREEN_HEIGHT) {
        psys_debug("move: out-of-screen access\n");
        return;
    }
    SDL_LockMutex(screen->mutex);
    /* [dest > src]
     * a[x+1] = a[x];
     * [forward]          [reverse]
     * a[2] = a[1];       a[4] = a[3];
     * a[3] = a[2];       a[3] = a[2];
     * a[4] = a[3];       a[2] = a[1];
     * (wrong)            (ok)
     */
    if (dx < sx) {
        if (dy < sy) {
            for (cy = 0; cy < height; ++cy) {
                for (cx = 0; cx < width; ++cx) {
                    screen->rows[dy + cy][dx + cx] = screen->rows[sy + cy][sx + cx];
                }
            }
        } else { /* dy >= sy */
            for (cy = height - 1; cy >= 0; --cy) {
                for (cx = 0; cx < width; ++cx) {
                    screen->rows[dy + cy][dx + cx] = screen->rows[sy + cy][sx + cx];
                }
            }
        }
    } else { /* dx >= sx */
        if (dy < sy) {
            for (cy = 0; cy < height; ++cy) {
                for (cx = width - 1; cx >= 0; --cx) {
                    screen->rows[dy + cy][dx + cx] = screen->rows[sy + cy][sx + cx];
                }
            }
        } else { /* dy >= sy */
            for (cy = height - 1; cy >= 0; --cy) {
                for (cx = width - 1; cx >= 0; --cx) {
                    screen->rows[dy + cy][dx + cx] = screen->rows[sy + cy][sx + cx];
                }
            }
        }
    }
    screen->buffer_dirty = true;
    SDL_UnlockMutex(screen->mutex);
}

static void sdlscreen_vblank_interrupt(struct game_screen *screen_)
{
    struct sdl_screen *screen = sdl_screen(screen_);
    (void)screen;
    if (screen->vblank_cb) {
        screen->vblank_cb(screen_, screen->vblank_cb_arg);
    }
}

static void sdlscreen_add_vblank_cb(struct game_screen *screen_, game_screen_vblank_func *f, void *arg)
{
    struct sdl_screen *screen = sdl_screen(screen_);

    screen->vblank_cb     = f;
    screen->vblank_cb_arg = arg;
}

static void sdlscreen_draw_points(struct game_screen *screen_, unsigned vr_mode, struct game_screen_point *points, unsigned npoints)
{
    struct sdl_screen *screen = sdl_screen(screen_);
    unsigned i;
    const struct rect *clip = &fullscreen;
    SDL_LockMutex(screen->mutex);
    for (i = 0; i < npoints; ++i) {
        if (points[i].y < clip->y0 || points[i].y > clip->y1
            || points[i].x < clip->x0 || points[i].x > clip->x1) {
            continue;
        }
        draw_pixel(vr_mode, screen->rows[points[i].y], points[i].x, 1, 0, points[i].color);
    }
    screen->buffer_dirty = true;
    SDL_UnlockMutex(screen->mutex);
}

struct game_screen *new_game_screen(void)
{
    struct sdl_screen *screen = CALLOC_STRUCT(sdl_screen);
    int i;
    screen->base.v_pline          = &sdlscreen_v_pline;
    screen->base.v_ellarc         = &sdlscreen_v_ellarc;
    screen->base.vr_recfl         = &sdlscreen_vr_recfl;
    screen->base.v_show_c         = &sdlscreen_v_show_c;
    screen->base.vs_clip          = &sdlscreen_vs_clip;
    screen->base.vro_cpyfm        = &sdlscreen_vro_cpyfm;
    screen->base.vrt_cpyfm        = &sdlscreen_vrt_cpyfm;
    screen->base.vsc_form         = &sdlscreen_vsc_form;
    screen->base.vq_mouse         = &sdlscreen_vq_mouse;
    screen->base.set_color        = &sdlscreen_set_color;
    screen->base.draw_image       = &sdlscreen_draw_image;
    screen->base.get_image        = &sdlscreen_get_image;
    screen->base.draw_sprite      = &sdlscreen_draw_sprite;
    screen->base.move             = &sdlscreen_move;
    screen->base.vblank_interrupt = &sdlscreen_vblank_interrupt;
    screen->base.add_vblank_cb    = &sdlscreen_add_vblank_cb;
    screen->base.draw_points      = &sdlscreen_draw_points;
    screen->base.destroy          = &sdlscreen_destroy;

    screen->mutex = SDL_CreateMutex();

    /* Set up row pointers for easy access */
    for (i = 0; i < SCREEN_HEIGHT; ++i) {
        screen->rows[i] = &screen->buffer[i * SCREEN_WIDTH];
    }

    /* Initial clip rectangle */
    screen->clip.x0 = 0;
    screen->clip.y0 = 0;
    screen->clip.x1 = SCREEN_WIDTH - 1;
    screen->clip.y1 = SCREEN_HEIGHT - 1;
#if 0
    /* silly starting pattern for testing */
    for (i=0; i<SCREEN_COLORS; ++i) {
        screen->palette[i] = i | (i<<4) | (i<<8);
    }
    screen->palette_dirty = true;
    for (i=0; i<SCREEN_HEIGHT; ++i) {
        int j;
        for (j=0; j<SCREEN_WIDTH; ++j) {
            screen->rows[i][j] = (i/4) % 16;
        }
    }
    screen->buffer_dirty = true;
#endif
    return &screen->base;
}

void game_sdlscreen_update_textures(struct game_screen *screen_, unsigned scr_tex, unsigned pal_tex)
{
    struct sdl_screen *screen = sdl_screen(screen_);
    uint8_t palette_img[SCREEN_COLORS * 4];
    int x;
    SDL_LockMutex(screen->mutex);
    if (screen->buffer_dirty) {
        /* Build screen texture */
        glBindTexture(GL_TEXTURE_2D, scr_tex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_LUMINANCE, GL_UNSIGNED_BYTE, screen->buffer);
        screen->buffer_dirty = false;
    }

    if (screen->palette_dirty) {
        /* Update palette texture, after converting ST format to RGBA */
        glBindTexture(GL_TEXTURE_2D, pal_tex);
        for (x = 0; x < SCREEN_COLORS; ++x) {
            /* atari ST paletter color is 0x0rgb, convert to RGBA */
            unsigned red           = (screen->palette[x] >> 8) & 7;
            unsigned green         = (screen->palette[x] >> 4) & 7;
            unsigned blue          = (screen->palette[x] >> 0) & 7;
            palette_img[x * 4 + 0] = (red << 5) | (red << 2) | (red >> 1);
            palette_img[x * 4 + 1] = (green << 5) | (green << 2) | (green >> 1);
            palette_img[x * 4 + 2] = (blue << 5) | (blue << 2) | (blue >> 1);
            palette_img[x * 4 + 3] = 255;
        }
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCREEN_COLORS, 1, GL_RGBA, GL_UNSIGNED_BYTE, palette_img);
        screen->palette_dirty = false;
    }
    SDL_UnlockMutex(screen->mutex);
}

void game_sdlscreen_update_cursor(struct game_screen *screen_, void **cursor)
{
    struct sdl_screen *screen = sdl_screen(screen_);
    SDL_Cursor *oldcursor     = (SDL_Cursor *)*cursor;
    SDL_Cursor *newcursor;
    SDL_LockMutex(screen->mutex);
    if (screen->cursor_dirty) { /* Only create a new cursor if it was updated */
        bool cursor_set = false;
        int i;
        /* First, make sure a cursor is actually set */
        for (i = 0; i < CURSOR_SIZE; ++i) {
            if (screen->cursor_data[i] || screen->cursor_mask[i]) {
                cursor_set = true;
            }
        }
        if (cursor_set) {
            newcursor = SDL_CreateCursor(screen->cursor_data, screen->cursor_mask, CURSOR_WIDTH, CURSOR_WIDTH, screen->cursor_hot_x, screen->cursor_hot_y);
        } else { /* Back to system cursor */
            newcursor = NULL;
        }
        SDL_SetCursor(newcursor);
        *cursor = newcursor;
        if (oldcursor) {
            SDL_FreeCursor(oldcursor);
        }
    }
    SDL_UnlockMutex(screen->mutex);
}

int game_sdlscreen_save_state(struct game_screen *screen_, int fd)
{
    /* Must be called without the interpreter thread running,
     * so no locking is needed.
     */
    struct sdl_screen *screen = sdl_screen(screen_);
    uint32_t id               = GAME_SDLSCREEN_STATE_ID;
    /* Save screen state */
    if (FD_WRITE(fd, id)
        || FD_WRITE(fd, screen->buffer)
        || FD_WRITE(fd, screen->palette)
        || FD_WRITE(fd, screen->cursor_data)
        || FD_WRITE(fd, screen->cursor_mask)
        || FD_WRITE(fd, screen->cursor_hot_x)
        || FD_WRITE(fd, screen->cursor_hot_y)
        || FD_WRITE(fd, screen->clip)) {
        return -1;
    }
    return 0;
}

int game_sdlscreen_load_state(struct game_screen *screen_, int fd)
{
    /* Must be called without the interpreter thread running,
     * so no locking is needed.
     */
    struct sdl_screen *screen = sdl_screen(screen_);
    uint32_t id;
    /* Load screen state */
    if (FD_READ(fd, id)) {
        return -1;
    }
    if (id != GAME_SDLSCREEN_STATE_ID) {
        psys_debug("Invalid game screen state record %08x\n", id);
        return -1;
    }
    if (FD_READ(fd, screen->buffer)
        || FD_READ(fd, screen->palette)
        || FD_READ(fd, screen->cursor_data)
        || FD_READ(fd, screen->cursor_mask)
        || FD_READ(fd, screen->cursor_hot_x)
        || FD_READ(fd, screen->cursor_hot_y)
        || FD_READ(fd, screen->clip)) {
        return -1;
    }
    /* Mark everything as dirty after load */
    screen->buffer_dirty  = true;
    screen->palette_dirty = true;
    screen->cursor_dirty  = true;
    return 0;
}
