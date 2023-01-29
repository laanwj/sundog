/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "game_gembind.h"

#include "psys/psys_debug.h"
#include "psys/psys_helpers.h"
#include "psys/psys_state.h"
#include "util/memutil.h"
#include "util/util_img.h"
#include "util/util_minmax.h"
#include "util/util_save_state.h"
#include "util/util_time.h"
#include "util/write_bmp.h"

#include "game_screen.h"
#include "game_sound.h"

#include <stdio.h>
#include <string.h>

/* Header for savestates */
#define GAME_GEMBIND_STATE_ID 0x47454d42

/* GEMBIND memory ("data pool"): needs to be 0xac00 bytes at least, see
 * psys_rsp_unitstatus. */
#define GEMBIND_MEMSIZE 0x10000
#define GEMBIND_MEMBASE (0x100000000 - GEMBIND_MEMSIZE)

struct gembind_priv {
    struct game_screen *screen;
    struct game_sound *sound;
    /* Pointer to p-system for vblank handler */
    struct psys_state *psys;
    /* Debug message level, 0 is no debugger output */
    int debug_level;

    /* Drawing state */
    unsigned line_color;
    unsigned line_width;
    unsigned fill_color;
    unsigned vr_mode;
    /* Extra memory for graphics, from the point of the VM this is situated
     * before the p-system memory but we do a custom mapping for convenience.
     */
    uint8_t memory[GEMBIND_MEMSIZE];
    /* Work space for image decompression.
     */
    uint8_t workspace[SCREEN_WIDTH * SCREEN_HEIGHT];
    /* State to do with moving on the screen */
    bool movement_enable1;
    bool movement_enable2;
    unsigned env_priv;
    int movement_start; /* Half of the sprites are moved each vblank - this alternating flag determines which half */
/* GEMBIND globals */
#define SPRITE_SYNC_FLAG_OFS (8 + 0x2 * 2) /* not used */
#define SPRITE_DESC_OFS (8 + 0xec * 2)
#define UNPASSABLE_OFS (8 + 0x230 * 2)
/* Maximum number of sprites (2*4 per vsync) */
#define NUM_SPRITES 8
};

/* VDI to hardware palette entry mapping.
 * Atari ST Application Programming, figure 6-8 page 130.
 * Index 14 is 11, not a repeat of 4.
 * {black, white, dblue, dgreen, dred, brown, dcyan, orange, grey, dgrey, blue, green, red, yellow, cyan, magenta}
 */
unsigned vdi_color_map[16] = { 0, 15, 1, 2, 4, 6, 3, 5, 7, 8, 9, 10, 12, 14, 11, 13 };

/* Sprite color map. Subtly different to the VDI color map.
 */
unsigned sprite_color_map[16] = { 0, 15, 1, 2, 4, 6, 3, 5, 8, 7, 9, 10, 12, 14, 11, 13 };

/* Hardcoded sprite patterns for rendering */
uint8_t sprite_patterns[10][7] = {
    { 0x00, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38 },
    { 0x00, 0x08, 0x1c, 0x3e, 0x7c, 0x38, 0x10 },
    { 0x00, 0x00, 0x7e, 0x7e, 0x7e, 0x00, 0x00 },
    { 0x00, 0x10, 0x38, 0x7c, 0x3e, 0x1c, 0x08 },
    { 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00 },
    { 0x00, 0x00, 0x3c, 0x3c, 0x3c, 0x00, 0x00 },
    { 0x00, 0x06, 0x3e, 0x1e, 0x3a, 0x50, 0xa0 },
    { 0x06, 0x3e, 0x5c, 0x3c, 0x54, 0xa8, 0x40 },
    { 0x0a, 0x1e, 0x1c, 0x3c, 0x58, 0xb0, 0x40 },
    { 0x06, 0x3e, 0x0c, 0x3c, 0xd4, 0x14, 0x20 },
};

/* Hardcoded sprite patterns for collision detection */
uint8_t collision_patterns[10][7] = {
    { 0x00, 0x38, 0x38, 0x28, 0x28, 0x38, 0x38 },
    { 0x00, 0x08, 0x1c, 0x36, 0x6c, 0x38, 0x10 },
    { 0x00, 0x00, 0x7e, 0x6e, 0x7e, 0x00, 0x00 },
    { 0x00, 0x10, 0x38, 0x6c, 0x36, 0x1c, 0x08 },
    { 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00 },
    { 0x00, 0x00, 0x3c, 0x3c, 0x3c, 0x00, 0x00 },
    { 0x06, 0x06, 0x3e, 0x5e, 0x3a, 0x50, 0xa0 },
    { 0x06, 0x3e, 0x5c, 0x3c, 0x54, 0xa8, 0x40 },
    { 0x0a, 0x1e, 0x1c, 0x3c, 0x58, 0xb0, 0x40 },
    { 0x06, 0x3e, 0x0c, 0x3c, 0xd4, 0x14, 0x20 },
};

enum {
    GEMBIND_VDI                  = 0x02,
    GEMBIND_AES                  = 0x03,
    GEMBIND_MemoryCopy           = 0x04,
    GEMBIND_GetAbsoluteAddress   = 0x05,
    GEMBIND_SetScreen            = 0x06,
    GEMBIND_DecompressImage      = 0x07,
    GEMBIND_DoSound              = 0x08,
    GEMBIND_Native09             = 0x09,
    GEMBIND_FlopFmt              = 0x0a,
    GEMBIND_SetColor             = 0x0b,
    GEMBIND_Native0C             = 0x0c,
    GEMBIND_ScrollScreen         = 0x0d,
    GEMBIND_CollisionDetect      = 0x28,
    GEMBIND_DrawSprite           = 0x29,
    GEMBIND_SpriteMovementEnable = 0x2a,
    GEMBIND_SetSprite            = 0x2b,
    GEMBIND_InstallVBlankHandler = 0x2c,
    GEMBIND_NUM_PROC             = 0x31,
};

/** GEM<->psys address translation */
static inline psys_fulladdr psys_lda(struct psys_state *s, psys_fulladdr addr)
{
    psys_fulladdr value = (psys_ldw(s, W(addr, 0)) << 16) | psys_ldw(s, W(addr, 1));
    /* Wraps around to GEMBIND_MEMBASE, but 0 stays 0. */
    if (value != 0) {
        value -= s->mem_fake_base;
    }
    return value;
}

static inline void psys_sta(struct psys_state *s, psys_fulladdr addr, psys_fulladdr value)
{
    /* Wraps around from GEMBIND_MEMBASE, but 0 stays 0. */
    if (value != 0) {
        value += s->mem_fake_base;
    }
    psys_stw(s, W(addr, 0), value >> 16);
    psys_stw(s, W(addr, 1), value & 0xffff);
}

static inline uint8_t *gem_bytes(struct psys_state *s, struct gembind_priv *priv, psys_fulladdr addr)
{
    if (addr < 0x80000000) {
        return psys_bytes(s, addr);
    } else {
        return &priv->memory[addr - GEMBIND_MEMBASE];
    }
}

/* Swap if dimensions are the wrong way around, so that
 * x0 <= x1 and y0 <= y1.
 * XXX should we flip/mirror in this case?
 */
static void sanitize_rect(int *x0, int *y0, int *x1, int *y1)
{
    if (*x1 < *x0) {
        int temp = *x0;
        *x0      = *x1;
        *x1      = temp;
    }
    if (*y1 < *y0) {
        int temp = *y0;
        *y0      = *y1;
        *y1      = temp;
    }
}

#define MAX_PLINE_VERTICES 256

/** VDI(handle,opcode,numptsin,numintin) */
static void gembind_VDI(struct psys_state *s, struct gembind_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    psys_word numintin   = psys_pop(s);
    psys_word numptsin   = psys_pop(s);
    psys_word opcode     = psys_pop(s);
    psys_word ctx        = psys_pop(s);
    psys_fulladdr contrl = psys_lda(s, W(ctx, 0));
    psys_fulladdr intin  = psys_lda(s, W(ctx, 2));
    psys_fulladdr ptsin  = psys_lda(s, W(ctx, 4));
    psys_fulladdr intout = psys_lda(s, W(ctx, 6));
    psys_fulladdr ptsout = psys_lda(s, W(ctx, 8));
    (void)numintin;
    /* Need calls:
     * 0x0001   v_opnwk     http://toshyp.atari.org/en/00700a.html#v_opnwk
     * 0x0002   v_clswk     http://toshyp.atari.org/en/00700a.html#v_clswk
     * 0x0006   v_pline     http://toshyp.atari.org/en/007005.html#v_pline
     * 0x000b   gdp         http://toshyp.atari.org/en/007005.html#v_bar
     * 0x0010   vsl_width   http://toshyp.atari.org/en/007004.html#vsl_width
     * 0x0011   vsl_color   http://toshyp.atari.org/en/007004.html#vsl_color
     * 0x0019   vsf_color   http://toshyp.atari.org/en/007004.html#vsf_color
     * 0x0020   vswr_mode   http://toshyp.atari.org/en/007004.html#vswr_mode
     * 0x006d   vro_cpyfm   http://toshyp.atari.org/en/00700b.html#vro_cpyfm
     * 0x006f   vsc_form    http://toshyp.atari.org/en/007007.html#vsc_form
     * 0x0072   vr_recfl    http://toshyp.atari.org/en/007005.html#vr_recfl
     * 0x0079   vrt_cpyfm   http://toshyp.atari.org/en/00700b.html#vrt_cpyfm
     * 0x007a   v_show_c    http://toshyp.atari.org/en/007007.html#v_show_c
     * 0x007b   v_hide_c    http://toshyp.atari.org/en/007007.html#v_hide_c
     * 0x007c   vq_mouse    http://toshyp.atari.org/en/007007.html#vq_mouse
     * 0x0081   vs_clip     http://toshyp.atari.org/en/00700a.html#vs_clip
     */
    /* TOO noisy.
    psys_debug("gembind_VDI 0x%04x 0x%04x 0x%04x 0x%04x [0x%05x 0x%05x 0x%05x 0x%05x 0x%05x]\n",
        ctx, opcode, numptsin, numintin,
        contrl, intin, ptsin, intout, ptsout);
    */
    switch (opcode) {
    case 0x0001: /* v_opnwk */
        break;
    case 0x0002: /* v_clswk */
        break;
    case 0x0006: { /* v_pline */
        psys_word count = numptsin;
        int i;
        int coordinates[MAX_PLINE_VERTICES * 2];
        if (count > MAX_PLINE_VERTICES) {
            psys_debug("gembind_VDI: Too many vertices specified to v_pline call - truncating\n");
            count = MAX_PLINE_VERTICES;
        }
        for (i = 0; i < count * 2; ++i) {
            coordinates[i] = psys_ldsw(s, W(ptsin, i));
        }
        priv->screen->v_pline(priv->screen, priv->vr_mode, vdi_color_map[priv->line_color], priv->line_width, count, coordinates);
    } break;
    case 0x000b: { /* gdp */
        psys_word subop = psys_ldw(s, W(contrl, 5));
        switch (subop) {
        case 6: /* v_ellarc */
            priv->screen->v_ellarc(priv->screen,
                priv->vr_mode, vdi_color_map[priv->line_color], priv->line_width,
                psys_ldsw(s, W(ptsin, 0)), psys_ldsw(s, W(ptsin, 1)), psys_ldsw(s, W(ptsin, 2)), psys_ldsw(s, W(ptsin, 3)),
                psys_ldsw(s, W(intin, 0)), psys_ldsw(s, W(intin, 1)));
            break;
        default:
            psys_debug("gembind_VDI: Unhandled gdp subop %d\n", subop);
        }
    } break;
    case 0x0010: /* vsl_width */
        priv->line_width = psys_ldw(s, W(ptsin, 0));
        psys_stw(s, W(ptsout, 0), priv->line_width); /* actual line width set */
        break;
    case 0x0011: /* vsl_color */
        priv->line_color = psys_ldw(s, W(intin, 0));
        psys_stw(s, W(intout, 0), priv->line_color); /* actual color set */
        break;
    case 0x0019: /* vsf_color */
        priv->fill_color = psys_ldw(s, W(intin, 0));
        psys_stw(s, W(intout, 0), priv->fill_color); /* actual color set */
        break;
    case 0x0020: /* vswr_mode */
        priv->vr_mode = psys_ldw(s, W(intin, 0));
        psys_stw(s, W(intout, 0), priv->vr_mode); /* actual write mode set */
        break;
    case 0x006d: { /* vro_cpyfm */
        unsigned vr_mode      = psys_ldw(s, W(intin, 0));
        int sx0               = psys_ldsw(s, W(ptsin, 0));
        int sy0               = psys_ldsw(s, W(ptsin, 1));
        int sx1               = psys_ldsw(s, W(ptsin, 2));
        int sy1               = psys_ldsw(s, W(ptsin, 3));
        int dx0               = psys_ldsw(s, W(ptsin, 4));
        int dy0               = psys_ldsw(s, W(ptsin, 5));
        int dx1               = psys_ldsw(s, W(ptsin, 6));
        int dy1               = psys_ldsw(s, W(ptsin, 7));
        psys_fulladdr srcMFDB = psys_lda(s, W(contrl, 7));
        psys_fulladdr dstMFDB = psys_lda(s, W(contrl, 9));
        psys_fulladdr src     = psys_lda(s, W(srcMFDB, 0));
        unsigned src_width    = psys_ldw(s, W(srcMFDB, 2));
        unsigned src_height   = psys_ldw(s, W(srcMFDB, 3));
        unsigned src_wdwidth  = psys_ldw(s, W(srcMFDB, 4));
        psys_fulladdr dst     = psys_lda(s, W(dstMFDB, 0));
        unsigned dst_wdwidth  = psys_ldw(s, W(dstMFDB, 4));
#if 0
        psys_debug("screen vro_cpyfm vr=%d %08x[%d %d %d] (%d,%d,%d,%d) -> %08x[%d] (%d,%d,%d,%d)\n",
                vr_mode,
                src, src_width, src_height, src_wdwidth,
                sx0, sy0, sx1, sy1,
                dst, dst_wdwidth,
                dx0, dy0, dx1, dy1);
#endif

        sanitize_rect(&sx0, &sy0, &sx1, &sy1);
        sanitize_rect(&dx0, &dy0, &dx1, &dy1);

        if (src == 0) {
            if (dst != 0) { /* screen to mem. This is used to save the background behind windows */
                const uint8_t *image_ptr;
                unsigned bytes_per_line;
                int width  = sx1 - sx0 + 1;
                int height = sy1 - sy0 + 1;
                priv->screen->get_image(priv->screen, sx0, sy0, width, height, &image_ptr, &bytes_per_line);
                util_img_planarize(gem_bytes(s, priv, dst), dst_wdwidth, image_ptr, width, height, bytes_per_line);
            } else { /* screen to screen (ignoring vr_mode) */
                priv->screen->move(priv->screen, dx0, dy0, sx0, sy0, sx1 - sx0 + 1, dy1 - dy0 + 1);
            }
            return;
        } else {
            if (dst != 0) { /* mem to mem (ignoring vr_mode) - used in XSLOTS */
                /* de-planarize source area to temporary buffer */
                int width  = sx1 - sx0 + 1;
                int height = sy1 - sy0 + 1;
#if 0
                psys_debug("mem to mem (%d,%d) from src (%d,%d) to dst (%d,%d)\n", width, height,
                        sx0, sy0, dx0, dy0);
#endif
                if (width * height > (SCREEN_WIDTH * SCREEN_HEIGHT)) { /* Enough space in staging area? */
                    psys_debug("Image too large for scratch space (%dx%d)\n", width, height);
                    return;
                }
                if (dx0) {
                    psys_debug("vro_cpyfm: mem to mem: cannot handle destination x != 0\n");
                    return;
                }
                util_img_unplanarize(priv->workspace, width, gem_bytes(s, priv, src), sx0, sy0, width, height, src_wdwidth);
                /* planarize temporary buffer to destination */
                util_img_planarize(gem_bytes(s, priv, dst) + dy0 * dst_wdwidth * 8,
                    dst_wdwidth, priv->workspace, width, height, width);
            } else { /* mem to screen */
                priv->screen->vro_cpyfm(priv->screen,
                    vr_mode,
                    gem_bytes(s, priv, src), src_width, src_height, src_wdwidth,
                    sx0, sy0, sx1, sy1, dx0, dy0, dx1, dy1);
            }
        }
    } break;
    case 0x006f: { /* vsc_form */
        uint16_t mform[36];
        int x;
        for (x = 0; x < 36; ++x) {
            mform[x] = psys_ldw(s, W(intin, x));
        }
        priv->screen->vsc_form(priv->screen, mform);
    } break;
    case 0x0072: /* vr_recfl */
        priv->screen->vr_recfl(priv->screen,
            priv->vr_mode, vdi_color_map[priv->fill_color],
            psys_ldsw(s, W(ptsin, 0)), psys_ldsw(s, W(ptsin, 1)), psys_ldsw(s, W(ptsin, 2)), psys_ldsw(s, W(ptsin, 3)));
        break;
    case 0x0079: { /* vrt_cpyfm */
        unsigned vr_mode      = psys_ldw(s, W(intin, 0));
        unsigned col1         = psys_ldw(s, W(intin, 1));
        unsigned col0         = psys_ldw(s, W(intin, 2));
        int sx0               = psys_ldsw(s, W(ptsin, 0));
        int sy0               = psys_ldsw(s, W(ptsin, 1));
        int sx1               = psys_ldsw(s, W(ptsin, 2));
        int sy1               = psys_ldsw(s, W(ptsin, 3));
        int dx0               = psys_ldsw(s, W(ptsin, 4));
        int dy0               = psys_ldsw(s, W(ptsin, 5));
        int dx1               = psys_ldsw(s, W(ptsin, 6));
        int dy1               = psys_ldsw(s, W(ptsin, 7));
        psys_fulladdr srcMFDB = psys_lda(s, W(contrl, 7));
        psys_fulladdr dstMFDB = psys_lda(s, W(contrl, 9));
        psys_fulladdr src     = psys_lda(s, W(srcMFDB, 0));
        unsigned src_width    = psys_ldw(s, W(srcMFDB, 2));
        unsigned src_height   = psys_ldw(s, W(srcMFDB, 3));
        unsigned src_wdwidth  = psys_ldw(s, W(srcMFDB, 4));
        psys_fulladdr dst     = psys_lda(s, W(dstMFDB, 0));

        sanitize_rect(&sx0, &sy0, &sx1, &sy1);
        sanitize_rect(&dx0, &dy0, &dx1, &dy1);

        if (src == 0) {
            psys_debug("vrt_cpyfm: Cannot copy from screen, ignoring\n");
            return;
        }
        if (dst != 0) {
            psys_debug("vrt_cpyfm: Can only copy pixels to screen, ignoring\n");
            return;
        }
        priv->screen->vrt_cpyfm(priv->screen,
            vr_mode, vdi_color_map[col0], vdi_color_map[col1],
            gem_bytes(s, priv, src), src_width, src_height, src_wdwidth,
            sx0, sy0, sx1, sy1, dx0, dy0, dx1, dy1);
    } break;
    case 0x007a: /* v_show_c */
        priv->screen->v_show_c(priv->screen, true);
        break;
    case 0x007b: /* v_hide_c */
        priv->screen->v_show_c(priv->screen, false);
        break;
    case 0x007c: { /* vq_mouse */
        unsigned buttons;
        int x, y;
        priv->screen->vq_mouse(priv->screen, &buttons, &x, &y);
        psys_stw(s, W(intout, 0), buttons); /* buttons */
        psys_stw(s, W(ptsout, 0), x);       /* x */
        psys_stw(s, W(ptsout, 1), y);       /* y */
        util_msleep(10);
        /* UI loop delay - ideally this would be a conditional wait on mouse state change or vsync,
         * but seems to work well enough.
         */
    } break;
    case 0x0081: /* vs_clip */
        priv->screen->vs_clip(priv->screen,
            psys_ldsw(s, W(intin, 0)), psys_ldsw(s, W(ptsin, 0)), psys_ldsw(s, W(ptsin, 1)), psys_ldsw(s, W(ptsin, 2)), psys_ldsw(s, W(ptsin, 3)));
        break;
    default:
        psys_debug("unsupported VDI call 0x%04x\n", opcode);
    }
}

/** AES(ctx,opcode,numintin,numintout,numaddrin,numaddrout) */
static void gembind_AES(struct psys_state *s, struct gembind_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    psys_word numaddrout      = psys_pop(s);
    psys_word numaddrin       = psys_pop(s);
    psys_word numintout       = psys_pop(s);
    psys_word numintin        = psys_pop(s);
    psys_word opcode          = psys_pop(s);
    psys_word ctx             = psys_pop(s);
    psys_fulladdr cb_pcontrol = psys_lda(s, W(ctx, 0));
    psys_fulladdr cb_pglobal  = psys_lda(s, W(ctx, 2));
    psys_fulladdr cb_pintin   = psys_lda(s, W(ctx, 4));
    psys_fulladdr cb_pintout  = psys_lda(s, W(ctx, 6));
    psys_fulladdr cb_padrin   = psys_lda(s, W(ctx, 8));
    psys_fulladdr cb_padrout  = psys_lda(s, W(ctx, 10));
    /* Need calls:
     * 0x000a   appl_init       http://toshyp.atari.org/en/Application.html#appl_init
     * 0x004d   graf_handle     http://toshyp.atari.org/en/00800c.html#graf_handle
     * 0x004e   graf_mouse      http://toshyp.atari.org/en/00800c.html#graf_mouse
     */
    if (priv->debug_level) {
        psys_debug("gembind_AES 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x [0x%05x 0x%05x 0x%05x 0x%05x 0x%05x 0x%05x]\n",
            ctx, opcode, numintin, numintout, numaddrin, numaddrout,
            cb_pcontrol, cb_pglobal, cb_pintin, cb_pintout, cb_padrin, cb_padrout);
    }
    switch (opcode) {
    case 0x000a: /* appl_init */
        break;
    case 0x004d:                          /* graf_handle */
        psys_stw(s, W(cb_pintout, 0), 1); /* VDI handle */
        break;
    case 0x004e: /* graf_mouse */
        break;
    default:
        psys_debug("unsupported AES call 0x%04x\n", opcode);
    }
}

/** MemoryCopy(from,to,bytes) */
static void gembind_MemoryCopy(struct psys_state *s, struct gembind_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    psys_word bytes        = psys_pop(s);
    psys_word to           = psys_pop(s);
    psys_word from         = psys_pop(s);
    psys_fulladdr fromaddr = psys_lda(s, from);
    psys_fulladdr toaddr   = psys_lda(s, to);
    if (priv->debug_level) {
        psys_debug("gembind_MemoryCopy 0x%04x[0x%05x] 0x%04x[0x%05x] 0x%04x\n",
            from, fromaddr, to, toaddr, bytes);
    }
#if 0
    psys_debug_hexdump(s, fromaddr, bytes);
#endif
    memcpy(gem_bytes(s, priv, toaddr), gem_bytes(s, priv, fromaddr), bytes);
}

/** GetAbsoluteAddress(addr,dst) */
static void gembind_GetAbsoluteAddress(struct psys_state *s, struct gembind_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    psys_word dst  = psys_pop(s);
    psys_word addr = psys_pop(s);
    if (priv->debug_level) {
        psys_debug("gembind_GetAbsoluteAddress 0x%04x 0x%04x\n", addr, dst);
    }
    psys_sta(s, dst, addr);
}

/** Unused, thus unimplemented */
static void gembind_SetScreen(struct psys_state *s, struct gembind_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    psys_word x = psys_pop(s);
    if (priv->debug_level) {
        psys_debug("gembind_SetScreen 0x%04x\n", x);
    }
}

/** DecompressImage(addr1,addr2,val1,val2)
 * This copies a compressed color image to either the screen or a bitmap to be rendered later
 */
static void gembind_DecompressImage(struct psys_state *s, struct gembind_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    psys_word y          = psys_pop(s);
    psys_word x          = psys_pop(s);
    psys_word addr2      = psys_pop(s);
    psys_word addr1      = psys_pop(s);
    psys_fulladdr addr1l = psys_lda(s, addr1);
    psys_fulladdr addr2l = psys_lda(s, addr2);
    psys_word addr1w     = psys_ldw(s, W(addr1l, 0)); /* what if addr1l is a negative address? */
    psys_word addr1h     = psys_ldw(s, W(addr1l, 1));
    unsigned srcsize;
    if (priv->debug_level) {
        psys_debug("gembind_DecompressImage 0x%04x[0x%08x](%d,%d) -> 0x%04x[%08x] (%d,%d)\n",
            addr1, addr1l, addr1w, addr1h, addr2, addr2l, x, y);
    }

    if (addr1w * addr1h > (SCREEN_WIDTH * SCREEN_HEIGHT)) { /* Enough space in staging area? */
        psys_debug("Image too large for scratch space (%dx%d)\n", addr1w, addr1h);
        return;
    }

    /* Decompress image into staging area */
    util_img_decompress_image(priv->workspace, gem_bytes(s, priv, addr1l + 4), addr1w, addr1h, &srcsize);

    /* Perform copy */
    if (addr2l == 0) { /* if to screen */
        priv->screen->draw_image(priv->screen, priv->workspace, addr1w, addr1h, x, y);
    } else { /* if not to screen, planarize it at destination - ignore x and y */
        uint8_t *dst_planar = gem_bytes(s, priv, addr2l);
        util_img_planarize(dst_planar, (addr1w + 15) >> 4, priv->workspace, addr1w, addr1h, addr1w);
    }

    /* This call leaves the stack in a really weird state, probably a bug */
    s->sp               = W(s->sp, -8);
    s->local_init_base  = 0;
    s->local_init_count = 8;
}

/** DoSound(ptr0).
 * Play sound. This does not block for the sound to complete.
 * Only one sound can be playing at a time, a new sound will replace the old one.
 * Sound is in XBIOS DoSound format: http://toshyp.atari.org/en/004011.html#Dosound
 * Details of the routine can be found here: http://st-news.com/issues/st-news-volume-2-issue-3/education/the-xbios-dosound-function/
 * Emulating this would involve
 * emulating the YM2149F PSG sound chip.
 */
static void gembind_DoSound(struct psys_state *s, struct gembind_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    psys_word a         = psys_pop(s);
    const uint8_t *data = psys_bytes(s, a);
    if (priv->sound) {
        /* XXX this should be safe as we're always passed a buffer of 128 bytes,
         * but it would be better to do some kind of bounds checking.
         */
        priv->sound->play_sound(priv->sound, data, 128);
    }
}

/* Unused, so not implemented */
static void gembind_Native09(struct psys_state *s, struct gembind_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    psys_word b = psys_pop(s);
    psys_word a = psys_pop(s);
    psys_debug("gembind_Native09 0x%04x 0x%04x\n", a, b);
    /* Returns a flag */
}

/** FlopFmt(a,b,c,d,e,f,g,h,i).
 * Format a track of the disk. We don't emulate this, and really don't want to
 * emulate this.
 */
static void gembind_FlopFmt(struct psys_state *s, struct gembind_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    psys_word i = psys_pop(s);
    psys_word h = psys_pop(s);
    psys_word g = psys_pop(s);
    psys_word f = psys_pop(s);
    psys_word e = psys_pop(s);
    psys_word d = psys_pop(s);
    psys_word c = psys_pop(s);
    psys_word b = psys_pop(s);
    psys_word a = psys_pop(s);
    psys_debug("gembind_FlopFmt 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x\n",
        a, b, c, d, e, f, g, h, i);
}

/** SetColor(color,index).
 * Set palette entry "index" to "color".
 */
static void gembind_SetColor(struct psys_state *s, struct gembind_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    psys_word index = psys_pop(s);
    psys_word color = psys_pop(s);
#if 0
    psys_debug("gembind_SetColor 0x%04x 0x%04x\n", color, index);
#endif
    priv->screen->set_color(priv->screen, index, color);
}

/** Unused, so not implemented */
static void gembind_Native0C(struct psys_state *s, struct gembind_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    psys_word a = psys_pop(s);
    psys_debug("gembind_Native0C 0x%04x\n", a);
}

/** ScrollScreen(amount,direction) */
static void gembind_ScrollScreen(struct psys_state *s, struct gembind_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    /* Used outside */
    /* 32 (a==0) or 40 (a==1) pixel scroll */
    /* b==0x0001 scroll up */
    /* b==0x0003 scroll right */
    /* b==0x0005 scroll left */
    /* b==0x0007 scroll down */
    psys_word direction = psys_pop(s);
    psys_word amount    = psys_pop(s) ? 40 : 32;
    if (priv->debug_level) {
        psys_debug("gembind_ScrollScreen %d 0x%04x\n", amount, direction);
    }
    switch (direction) {
    case 1: /* up */
        priv->screen->move(priv->screen, 0, 0, 0, amount, SCREEN_WIDTH, SCREEN_HEIGHT - amount);
        break;
    case 3: /* right */
        priv->screen->move(priv->screen, amount, 0, 0, 0, SCREEN_WIDTH - amount, SCREEN_HEIGHT);
        break;
    case 5: /* left */
        priv->screen->move(priv->screen, 0, 0, amount, 0, SCREEN_WIDTH - amount, SCREEN_HEIGHT);
        break;
    case 7: /* down */
        priv->screen->move(priv->screen, 0, amount, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT - amount);
        break;
    default:
        psys_debug("gembind_ScrollScreen: unknown direction %d\n", direction);
        break;
    }
}

/** Internal implementation for sprite collision detection. This is used both
 * from the vblank handler and from the GEMBIND binding.
 */
static psys_word collision_detect_internal(struct psys_state *s, struct gembind_priv *priv,
    psys_word pattern, psys_sword x, psys_sword y)
{
    const uint8_t *image_ptr;
    unsigned bytes_per_line;
    const psys_byte *unpassable = psys_bytes(s, priv->env_priv + UNPASSABLE_OFS);
    int cx, cy;

    x -= 4;
    y -= 4;
    priv->screen->get_image(priv->screen, x, y, 7, 7, &image_ptr, &bytes_per_line);

    for (cy = 0; cy < 7; ++cy) {
        for (cx = 0; cx < 7; ++cx) {
            if (collision_patterns[pattern][cy] & (1 << (~cx & 7))) {
                uint8_t pixel = image_ptr[cy * bytes_per_line + cx];
                if (unpassable[pixel]) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/** Internal handler for drawing and undrawing sprites.
 * This is used both by the vblank handler and the GEMBIND binding.
 */
static void draw_sprite_internal(struct psys_state *s, struct gembind_priv *priv, unsigned flag, psys_word back_addr, int x, int y, psys_word pattern, psys_word color)
{
    const uint8_t *image_ptr;
    unsigned bytes_per_line;
    uint8_t colors[7 * 7]; /* colors image */
    x -= 4;
    y -= 4;
    if (flag) {
#if 0
        psys_debug("Draw %d at %d,%d\n", pattern, x, y);
#endif
        /* Save background */
        priv->screen->get_image(priv->screen, x, y, 7, 7, &image_ptr, &bytes_per_line);
        util_img_planarize(psys_bytes(s, back_addr), 1, image_ptr, 7, 7, bytes_per_line);
        /* Draw opaque-colored */
        memset(colors, sprite_color_map[color], 7 * 7);
    } else {
#if 0
        psys_debug("Undraw %d at %d,%d\n", pattern, x, y);
#endif
        /* Load back background */
        util_img_unplanarize(colors, 7, psys_bytes(s, back_addr), 0, 0, 7, 7, 1);
    }
    priv->screen->draw_sprite(priv->screen, x, y, sprite_patterns[pattern], colors, 7, 7, 7);
}

/** CollisionDetect(pattern,x,y): integer */
static void gembind_CollisionDetect(struct psys_state *s, struct gembind_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    psys_sword y      = psys_spop(s);
    psys_sword x      = psys_spop(s);
    psys_word pattern = psys_pop(s);
    if (priv->debug_level) {
        psys_debug("gembind_CollisionDetect 0x%04x 0x%04x 0x%04x\n", pattern, x, y);
    }

    priv->env_priv = env_priv;
    psys_stw(s, s->sp, collision_detect_internal(s, priv, pattern, x, y));
    /* HACK: time delay to make combat playable, otherwise bullets are invisible
     * because they effectively move at light speed.
     */
    util_msleep(3);
}

/** DrawSprite(flag,back_addr,x,y,pattern,color) */
static void gembind_DrawSprite(struct psys_state *s, struct gembind_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    psys_word color     = psys_pop(s);
    psys_word pattern   = psys_pop(s);
    psys_sword y        = psys_spop(s);
    psys_sword x        = psys_spop(s);
    psys_word back_addr = psys_pop(s);
    psys_word flag      = psys_pop(s);

    if (priv->debug_level) {
        psys_debug("gembind_DrawSprite 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x\n", flag, back_addr, x, y, pattern, color);
    }
    draw_sprite_internal(s, priv, flag, back_addr, x, y, pattern, color);
    /* HACK: time delay to make combat playable, otherwise bullets are invisible
     * because they effectively move at light speed.
     */
    util_msleep(3);
}

/** SpriteMovementEnable(flag) */
static void gembind_SpriteMovementEnable(struct psys_state *s, struct gembind_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    psys_word flag = psys_pop(s);
#if 0
    psys_debug("gembind_SpriteMovementEnable %04x\n", flag);
#endif
    priv->movement_enable2 = flag & 1;
#if 0
    psys_debug_hexdump(s, env_priv + SPRITE_DESC_OFS, 0x10);
#endif
}

/** SetSprite(x,y,pattern,color,back,idx) */
static void gembind_SetSprite(struct psys_state *s, struct gembind_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    psys_word f   = psys_pop(s);
    psys_word e   = psys_pop(s);
    psys_word d   = psys_pop(s);
    psys_word c   = psys_pop(s);
    psys_word b   = psys_pop(s);
    psys_word a   = psys_pop(s);
    unsigned base = env_priv + SPRITE_DESC_OFS + f * 0x10;
    if (priv->debug_level) {
        psys_debug("gembind_SetSprite x=%d y=%d pattern=0x%04x color=0x%04x back=0x%04x id=0x%04x\n", a, b, c, d, e, f);
    }
    psys_stw(s, base + 0x00, c); /* pattern */
    psys_stw(s, base + 0x02, a); /* cur x coord */
    psys_stw(s, base + 0x04, b); /* cur y coord */
    /* 0x06 destination x coord - filled in by VM */
    /* 0x08 destination y coord - filled in by VM */
    psys_stw(s, base + 0x0a, 0); /* flag */
    psys_stw(s, base + 0x0c, d); /* color */
    psys_stw(s, base + 0x0e, e); /* address */
}

/** InstallVBlankHandler(flag) */
static void gembind_InstallVBlankHandler(struct psys_state *s, struct gembind_priv *priv, psys_fulladdr segment, psys_fulladdr env_priv)
{
    psys_word flag = psys_pop(s);
    if (priv->debug_level) {
        psys_debug("gembind_InstallVBlankHandler 0x%04x\n", flag);
    }
    priv->env_priv         = env_priv;
    priv->movement_enable1 = flag;
}

/** Try movement in direction dx/dy.
 * This will change the position, as well as the pattern for directional
 * sprites as needed, but only if succesful.
 * However, collision detection will always be done with the appropriate
 * directional sprite.
 */
static bool try_move(struct psys_state *s, struct gembind_priv *priv, psys_word *pattern_ptr,
    psys_sword *x, psys_sword *y, psys_sword dx, psys_sword dy)
{
    psys_word pattern = *pattern_ptr;
    /* Change sprite by direction.
     *             0-3  6-9
     * dx==0        0    6
     * dy==0        2    8
     * dx<0 && dy<0 3    9    !(dx<0 ^ dy<0)
     * dx>0 && dy>0 3    9
     * dx<0 && dy>0 1    7    dx<0 ^ dy<0
     * dx>0 && dy<0 1    7
     */
    if (pattern <= 3 || (pattern >= 6 && pattern <= 9)) {
        unsigned pattern_base;
        if (pattern <= 3) {
            pattern_base = 0;
        } else {
            pattern_base = 6;
        }
        if (dx == 0) {
            pattern = pattern_base;
        } else if (dy == 0) {
            pattern = pattern_base + 2;
        } else if ((dx < 0) ^ (dy < 0)) {
            pattern = pattern_base + 1;
        } else {
            pattern = pattern_base + 3;
        }
    }
    if (!collision_detect_internal(s, priv, pattern, *x + dx, *y + dy)) {
        *pattern_ptr = pattern;
        *x += dx;
        *y += dy;
        return true;
    }
    return false;
}

/** Vblank callback from screen. */
static void gembind_vblank_cb(struct game_screen *screen, void *arg)
{
    struct gembind_priv *priv = (struct gembind_priv *)arg;
    struct psys_state *s      = priv->psys;
    int n;
    if (priv->movement_enable1 && priv->movement_enable2) {
        for (n = priv->movement_start; n < NUM_SPRITES; n += 2) {
            psys_word base      = priv->env_priv + SPRITE_DESC_OFS + n * 0x10;
            psys_word pattern   = psys_ldw(s, base + 0x00);
            psys_sword curx     = psys_ldsw(s, base + 0x02);
            psys_sword cury     = psys_ldsw(s, base + 0x04);
            psys_sword destx    = psys_ldsw(s, base + 0x06);
            psys_sword desty    = psys_ldsw(s, base + 0x08);
            psys_sword flag     = psys_ldsw(s, base + 0x0a);
            psys_word color     = psys_ldw(s, base + 0x0c);
            psys_word back_addr = psys_ldw(s, base + 0x0e);
            psys_sword dx, dy;
            psys_sword dx_c, dy_c;
            if (flag <= 0) {
                continue;
            }
#if 0
            psys_debug("Visiting sprite %d pattern %d curx %d cury %d destx %d desty %d flag %d color %d\n",
                    n, pattern, curx, cury, destx, desty, flag, color);
#endif
            /* Delta x and y, clamped to -2..2 */
            dx = imax(imin(destx - curx, 2), -2);
            dy = imax(imin(desty - cury, 2), -2);
            /* Delta x and y, clamped to -1..1 */
            dx_c = imax(imin(dx, 1), -1);
            dy_c = imax(imin(dy, 1), -1);

            if (dx != 0 || dy != 0) { /* Any movement at all? */
                draw_sprite_internal(s, priv, 0, back_addr, curx, cury, pattern, color);
#if 0
                psys_debug("  dx %d dy %d\n", dx, dy);
#endif
                /* - Try with full movement
                 * - Try with dx=0 (if dy!=0)
                 * - Try with dy=0 (if dx!=0)
                 * - Try with dx and dy clamped to -1..1 iso -2..2
                 * - Try with that and dx=0 (if dy!=0)
                 * - Try with that and dy=0 (if dx!=0)
                 * If all fail, give up and set flag to 0xffff.
                 */
                if (try_move(s, priv, &pattern, &curx, &cury, dx, dy)
                    || ((dy != 0) && try_move(s, priv, &pattern, &curx, &cury, 0, dy))
                    || ((dx != 0) && try_move(s, priv, &pattern, &curx, &cury, dx, 0))
                    || try_move(s, priv, &pattern, &curx, &cury, dx_c, dy_c)
                    || ((dy != 0) && try_move(s, priv, &pattern, &curx, &cury, 0, dy_c))
                    || ((dx != 0) && try_move(s, priv, &pattern, &curx, &cury, dx_c, 0))) {
#if 0
                    psys_debug("  movement success to %d %d pattern %d\n", curx, cury, pattern);
#endif
                } else { /* Cannot reach destination, set flag */
#if 0
                    psys_debug("  movement fail\n");
#endif
                    flag = 0xffff;
                }
#if 0
                psys_debug("%d %d %d -> %d %d %04x pattern=%d\n", flag, curx, cury, destx, desty, back_addr, pattern);
#endif
                /* Store new sprite info, render sprite */
                draw_sprite_internal(s, priv, 1, back_addr, curx, cury, pattern, color);
                psys_stw(s, base + 0x00, pattern);
                psys_stw(s, base + 0x02, curx);
                psys_stw(s, base + 0x04, cury);
                psys_stw(s, base + 0x0a, flag);
            } else { /* At destination, clear flag */
                psys_stw(s, base + 0x0a, 0);
            }
        }
        priv->movement_start = 1 - priv->movement_start;
    }
}

static int gembind_save_state(struct psys_binding *b, int fd)
{
    struct gembind_priv *priv = (struct gembind_priv *)b->userdata;
    uint32_t id               = GAME_GEMBIND_STATE_ID;
    /* Save GEMBIND state */
    if (FD_WRITE(fd, id)
        || FD_WRITE(fd, priv->line_color)
        || FD_WRITE(fd, priv->line_width)
        || FD_WRITE(fd, priv->fill_color)
        || FD_WRITE(fd, priv->vr_mode)
        || FD_WRITE(fd, priv->memory)
        || FD_WRITE(fd, priv->movement_enable1)
        || FD_WRITE(fd, priv->movement_enable2)
        || FD_WRITE(fd, priv->env_priv)
        || FD_WRITE(fd, priv->movement_start)) {
        return -1;
    }
    return 0;
}

static int gembind_load_state(struct psys_binding *b, int fd)
{
    struct gembind_priv *priv = (struct gembind_priv *)b->userdata;
    uint32_t id;
    /* Load GEMBIND state */
    if (FD_READ(fd, id)) {
        return -1;
    }
    if (id != GAME_GEMBIND_STATE_ID) {
        psys_debug("Invalid GEMBIND state record %08x\n", id);
        return -1;
    }

    if (FD_READ(fd, priv->line_color)
        || FD_READ(fd, priv->line_width)
        || FD_READ(fd, priv->fill_color)
        || FD_READ(fd, priv->vr_mode)
        || FD_READ(fd, priv->memory)
        || FD_READ(fd, priv->movement_enable1)
        || FD_READ(fd, priv->movement_enable2)
        || FD_READ(fd, priv->env_priv)
        || FD_READ(fd, priv->movement_start)) {
        return -1;
    }
    return 0;
}

struct psys_binding *new_gembind(struct psys_state *state, struct game_screen *screen, struct game_sound *sound)
{
    struct psys_binding *b    = CALLOC_STRUCT(psys_binding);
    struct gembind_priv *priv = CALLOC_STRUCT(gembind_priv);

    priv->screen = screen;
    priv->sound  = sound;
    priv->psys   = state;
    /* Have screen call us for every vblank */
    screen->add_vblank_cb(screen, &gembind_vblank_cb, priv);

    b->userdata     = priv;
    b->num_handlers = GEMBIND_NUM_PROC;
    memcpy(b->seg.name, "GEMBIND ", 8);
    b->handlers                               = calloc(GEMBIND_NUM_PROC, sizeof(psys_bindingfunc *));
    b->handlers[GEMBIND_VDI]                  = (psys_bindingfunc *)&gembind_VDI;
    b->handlers[GEMBIND_AES]                  = (psys_bindingfunc *)&gembind_AES;
    b->handlers[GEMBIND_MemoryCopy]           = (psys_bindingfunc *)&gembind_MemoryCopy;
    b->handlers[GEMBIND_GetAbsoluteAddress]   = (psys_bindingfunc *)&gembind_GetAbsoluteAddress;
    b->handlers[GEMBIND_SetScreen]            = (psys_bindingfunc *)&gembind_SetScreen;
    b->handlers[GEMBIND_DecompressImage]      = (psys_bindingfunc *)&gembind_DecompressImage;
    b->handlers[GEMBIND_DoSound]              = (psys_bindingfunc *)&gembind_DoSound;
    b->handlers[GEMBIND_Native09]             = (psys_bindingfunc *)&gembind_Native09;
    b->handlers[GEMBIND_FlopFmt]              = (psys_bindingfunc *)&gembind_FlopFmt;
    b->handlers[GEMBIND_SetColor]             = (psys_bindingfunc *)&gembind_SetColor;
    b->handlers[GEMBIND_Native0C]             = (psys_bindingfunc *)&gembind_Native0C;
    b->handlers[GEMBIND_ScrollScreen]         = (psys_bindingfunc *)&gembind_ScrollScreen;
    b->handlers[GEMBIND_CollisionDetect]      = (psys_bindingfunc *)&gembind_CollisionDetect;
    b->handlers[GEMBIND_DrawSprite]           = (psys_bindingfunc *)&gembind_DrawSprite;
    b->handlers[GEMBIND_SpriteMovementEnable] = (psys_bindingfunc *)&gembind_SpriteMovementEnable;
    b->handlers[GEMBIND_SetSprite]            = (psys_bindingfunc *)&gembind_SetSprite;
    b->handlers[GEMBIND_InstallVBlankHandler] = (psys_bindingfunc *)&gembind_InstallVBlankHandler;

    b->save_state = &gembind_save_state;
    b->load_state = &gembind_load_state;

    /* Initial graphics attributes */
    priv->line_width = 1;

    return b;
}

void destroy_gembind(struct psys_binding *b)
{
    struct gembind_priv *priv = (struct gembind_priv *)b->userdata;
    free(b->handlers);
    free(priv);
    free(b);
}
