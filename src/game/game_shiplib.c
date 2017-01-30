/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "game_shiplib.h"

#include "game/game_screen.h"
#include "psys/psys_constants.h"
#include "psys/psys_debug.h"
#include "psys/psys_helpers.h"
#include "psys/psys_state.h"
#include "util/util_save_state.h"

#include "util/memutil.h"

#include <string.h>

/* Header for savestates */
#define GAME_SHIPLIB_STATE_ID 0x53484950

#define SHIPLIB_NUM_PROC 0x26

struct shiplib_priv {
    struct game_screen *screen;
};

/* VDI 1,2,6,4 */
static const unsigned star_colors[4] = { 0xf, 0xb, 0x3, 0x4 };
/* VDI 0,8,4,12,12,13,13,1 */
static const unsigned obj_colors[8] = { 0x0, 0x8, 0x4, 0xc, 0xc, 0xe, 0xe, 0xf };

/* shiplib_16() */
static void shiplib_16(struct psys_state *s, struct shiplib_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
#if 0
    psys_debug("shiplib_16\n");
#endif
    /* Draw starfield */
    psys_word startofs = psys_ldsw(s, W(env_priv + PSYS_MSCW_VAROFS, 0x10));
    psys_sword startx  = psys_ldsw(s, W(env_priv + PSYS_MSCW_VAROFS, 0x11));
    psys_sword numrows = psys_ldsw(s, W(env_priv + PSYS_MSCW_VAROFS, 0x12));
    psys_word bufofs   = psys_ldsw(s, W(env_priv + PSYS_MSCW_VAROFS, 0x6));
    psys_byte *xbuf    = psys_bytes(s, bufofs);
    psys_byte *rows    = xbuf + 0x100;
    unsigned xbufofs;
    struct game_screen_point point[SCREEN_HEIGHT * 2];
    int ptn = 0;
    int x, y;
    if (numrows > SCREEN_HEIGHT) {
        psys_debug("shiplib_16: too many rows\n");
        return;
    }
    for (y = 0; y < numrows; ++y) {
        if (rows[y]) { /* if set for row, clear pixel first */
            point[ptn].x     = rows[y];
            point[ptn].y     = 6 + y;
            point[ptn].color = 0;
            ptn += 1;
        }
        xbufofs = (startofs + 6 + y) & 0xff;
        x       = (startx + xbuf[xbufofs]) & 0xff;
        if (x >= 0x60) { /* beyond left edge of viewscreen */
            point[ptn].x     = x;
            point[ptn].y     = y + 6;
            point[ptn].color = star_colors[xbufofs & 3];
            ptn += 1;
            rows[y] = x;
        } else { /* set to 0 which means entry unused */
            rows[y] = 0;
        }
    }
    /* Draw queued pixels */
    priv->screen->draw_points(priv->screen, 1 /*Replace*/, point, ptn);
}

/** shiplib_18() */
static void shiplib_18(struct psys_state *s, struct shiplib_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    psys_debug("shiplib_18\n");
    psys_word count      = psys_ldsw(s, W(env_priv + PSYS_MSCW_VAROFS, 0xc));
    psys_sword ship_dx   = psys_ldsw(s, W(env_priv + PSYS_MSCW_VAROFS, 0xe));
    psys_sword ship_dy   = psys_ldsw(s, W(env_priv + PSYS_MSCW_VAROFS, 0xd));
    psys_byte *points_x  = psys_bytes(s, W(env_priv + PSYS_MSCW_VAROFS, 0x13));
    psys_byte *points_y  = points_x + 0x10;
    psys_byte *points_dx = psys_bytes(s, W(env_priv + PSYS_MSCW_VAROFS, 0x23));
    psys_byte *points_dy = points_y + 0x10;
    struct game_screen_point point[16 * 4 * 2];
    int ptn                  = 0, i, xx, yy;
    static int obj_color_idx = 0; /* keep track of current color index */
    unsigned color;
    /* Decrease count and store new value in global */
    count -= 1;
    psys_stw(s, W(env_priv + PSYS_MSCW_VAROFS, 0xc), count);
    /* Ship coordinate deltas are divided by four */
    ship_dx >>= 2;
    ship_dy >>= 2;
    /* Pick the next color from the object color array. This is done globally,
     * not per object, causing them to all change color at the same time. */
    color = obj_colors[obj_color_idx++];
    if (obj_color_idx == 8) {
        obj_color_idx = 0;
    }
    for (i = 0; i < 16; ++i) {
        uint8_t x = points_x[i];
        uint8_t y = points_y[i];
        if (x) { /* Only process if x coordinate is non-zero */
            /* Clear 2x2 block first */
            for (yy = 0; yy < 2; ++yy) {
                for (xx = 0; xx < 2; ++xx) {
                    point[ptn].x     = x + xx;
                    point[ptn].y     = y + yy;
                    point[ptn].color = 0;
                    ptn += 1;
                }
            }

            x += points_dx[y] + ship_dx;
            y += points_dy[y] + ship_dy;

            if (count == 0 || x <= 96 || y <= 7 || y >= 116) { /* if counter expired or out of viewscreen, nuke it */
                points_dx[i] = 0;
            } else { /* store new position and redraw */
                points_dx[i] = x;
                points_dy[i] = y;
                for (yy = 0; yy < 2; ++yy) {
                    for (xx = 0; xx < 2; ++xx) {
                        point[ptn].x     = x + xx;
                        point[ptn].y     = y + yy;
                        point[ptn].color = color;
                        ptn += 1;
                    }
                }
            }
        }
    }
    /* Draw queued pixels */
    priv->screen->draw_points(priv->screen, 1 /*Replace*/, point, ptn);
}

/** shiplib_19(x) */
static void shiplib_19(struct psys_state *s, struct shiplib_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    psys_word x = psys_pop(s);
    const psys_byte *src;
    unsigned color;
    int i, f, y;

    psys_debug("shiplib_19 0x%04x\n", x);
    /* Draw weapon fire */
    if (x) {
        src   = psys_bytes(s, segment + 0x1646);
        color = 0xc;
    } else {
        src   = psys_bytes(s, segment + 0x1b32);
        color = 0xe;
    }

    /* Draw, wait, and draw again later to remove again */
    for (f = 0; f < 2; ++f) {
        /* Source is 63 rows times 20*8=160 pixels, 10 words per row */
        y = 117;
        for (i = 0; i < 63; ++i) {
            priv->screen->vrt_cpyfm(priv->screen, 3 /*XOR*/, 0, color,
                src, 160, 63, 10,
                0, i, 159, i,
                96, y, 255, y);
            y -= 1;
        }

        /* Wait two frames */
        usleep(2000000 / 50);
    }
}

/** shiplib_1A(a,b,c,d) */
static void shiplib_1A(struct psys_state *s, struct shiplib_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    psys_word d = psys_pop(s);
    psys_word c = psys_pop(s);
    psys_word b = psys_pop(s);
    psys_word a = psys_pop(s);
    psys_debug("shiplib_1A 0x%04x 0x%04x 0x%04x 0x%04x (stub)\n", a, b, c, d);
    /* Warp animation, no clue what the parameters mean */
}

static int shiplib_save_state(struct psys_binding *b, int fd)
{
    uint32_t id = GAME_SHIPLIB_STATE_ID;
    /* Save shiplib state (dummy) */
    if (FD_WRITE(fd, id)) {
        return -1;
    }
    return 0;
}

static int shiplib_load_state(struct psys_binding *b, int fd)
{
    uint32_t id;
    /* Load shiplib state (dummy) */
    if (FD_READ(fd, id)) {
        return -1;
    }
    if (id != GAME_SHIPLIB_STATE_ID) {
        psys_debug("Invalid shiplib state record %08x\n", id);
        return -1;
    }
    return 0;
}

struct psys_binding *new_shiplib(struct psys_state *state, struct game_screen *screen)
{
    struct psys_binding *b    = CALLOC_STRUCT(psys_binding);
    struct shiplib_priv *priv = CALLOC_STRUCT(shiplib_priv);

    priv->screen = screen;
    (void)state;
    b->userdata     = priv;
    b->num_handlers = SHIPLIB_NUM_PROC;
    memcpy(b->seg.name, "SHIPLIB ", 8);
    b->handlers       = calloc(SHIPLIB_NUM_PROC, sizeof(psys_bindingfunc *));
    b->handlers[0x16] = (psys_bindingfunc *)&shiplib_16;
    b->handlers[0x18] = (psys_bindingfunc *)&shiplib_18;
    b->handlers[0x19] = (psys_bindingfunc *)&shiplib_19;
    b->handlers[0x1a] = (psys_bindingfunc *)&shiplib_1A;

    b->save_state = &shiplib_save_state;
    b->load_state = &shiplib_load_state;

    return b;
}

void destroy_shiplib(struct psys_binding *b)
{
    struct shiplib_priv *priv = (struct shiplib_priv *)b->userdata;
    free(b->handlers);
    free(priv);
    free(b);
}
