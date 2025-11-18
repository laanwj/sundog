/*
 * Copyright (c) 2023 Wladimir J. van der Laan and FTL software.
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
/* Mike Newton's 68000 "Generic" SunDog Wowzo Hyperspace (shiplib_1A), ported to C.
 *
 * Note that this is meant to be a fairly direct reimplementation of the
 * assembly code and not necessarily as one would write idiomatic C code
 * nowadays. Please don't try to optimize this.
 */
#include "wowzo.h"

#include "game/game_screen.h"
#include "game/game_sound.h"
#include "util/memutil.h"
#include "util/util_time.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define MAXSTARS (32)

/* Screen offsets. */
#define GLEFT (96)
#define GTOP (6)
#define GRIGHT (256)
#define GBOTTOM (117)
#define XWIDE (160)
#define YHI (110)
#define GMIDX (175)
#define GMIDY (61)

#define NUMRXCELL (11) /* rect cell grid x size */
#define NUMRYCELL (11)
#define RXCELL (15) /* rectangle x cell size */
#define RYCELL (10) /* y */

#define RECTDELAY (120) /* max(rYCell,rXCell) * max(waitX/Y) */

/* Loop/count equates. */
#define S1STARS (25) /* stage 1 and 2 stars count */
#define S3STARS (30) /* stage 3 stars count */
#define S4STARS (5)  /* stage 4 stars count */

#define S1LPCNT (400)
#define S2LPCNT (600)
#define S3LPCNT (1400) /* >= numRectStages * RectDelay - 1 */
#define S4LPCNT (450)  /* minimum len of stage 4 */
#define S4LPMLT (110)  /* j-sec multiplier for stage 4 len */

#define S1WAIT (1200) /* initial delay time for stage 1 and 2 */
#define S3WAIT (45)
#define S4WAIT (150)

/**
 * rectangles to clear
 * FORMAT: lCell,tCell,rCell+1,bCell+1
 */
static const uint8_t rects[28][4] = {
    { 5, 3, 6, 8 },
    { 5, 2, 6, 9 },
    { 4, 4, 7, 7 },
    { 5, 1, 6, 10 },
    { 4, 3, 7, 8 },
    { 3, 4, 8, 7 },
    { 5, 0, 6, 11 },
    { 4, 2, 7, 9 },
    { 3, 3, 8, 8 },
    { 2, 4, 9, 7 },
    { 4, 1, 7, 10 },
    { 3, 2, 8, 9 },
    { 2, 3, 9, 8 },
    { 1, 4, 10, 7 },
    { 4, 0, 7, 11 },
    { 3, 1, 8, 10 },
    { 2, 2, 9, 9 },
    { 1, 3, 10, 8 },
    { 0, 4, 11, 7 },
    { 3, 0, 8, 11 },
    { 2, 1, 9, 10 },
    { 1, 2, 10, 9 },
    { 0, 3, 11, 8 },
    { 2, 0, 9, 11 },
    { 1, 1, 10, 10 },
    { 0, 2, 11, 9 },
    { 1, 0, 10, 11 },
    { 0, 1, 11, 10 },
};

/** Number of rectangles per rectangle stage, must sum to length of rects. */
static const uint8_t num_rects[9] = { 1, 2, 3, 4, 4, 5, 4, 3, 2 };

/** Initial rectangle per rectangle stage, this is a prefix sum of num_rects. */
static const uint8_t rect_list[9] = { 0, 1, 3, 6, 10, 14, 19, 23, 26 };

/** Sounds. */
static const uint8_t sound_warp1[] = { 0x07, 0x38, 0x08, 0x10, 0x09, 0x10, 0x0a, 0x10, 0x00, 0x64, 0x01, 0x05, 0x02, 0x6e, 0x03, 0x05, 0x04, 0x67, 0x05, 0x05, 0x0c, 0x05, 0x80, 0x00, 0x81, 0x0d, 0x02, 0x01 };
static const uint8_t sound_warp2[] = { 0x0c, 0xff, 0x07, 0x07, 0x08, 0x10, 0x09, 0x10, 0x0a, 0x10, 0x80, 0x00, 0x0d, 0x09, 0x81, 0x06, 0x01, 0x5a, 0x08, 0x0c, 0x09, 0x0c, 0x0a, 0x0b, 0xff, 0x00 };
static const uint8_t sound_warp3[] = { 0x0c, 0x04, 0x06, 0x00, 0x07, 0x08, 0x00, 0x64, 0x01, 0x00, 0x02, 0x67, 0x03, 0x00, 0x04, 0x5f, 0x05, 0x00, 0x08, 0x10, 0x09, 0x10, 0x0a, 0x10, 0x0d, 0x0f, 0xff, 0x00 };

static const uint32_t tick_ms = 10;

/** state structure */
struct wowzo {
    /* OS handles */
    struct game_screen *screen;
    struct game_sound *sound;

    /* arrays for stars */
    uint8_t xdelta[MAXSTARS]; /* xDirection */
    uint8_t ydelta[MAXSTARS]; /* yDirection */
    uint8_t oxwait[MAXSTARS]; /* original xwait */
    uint8_t oywait[MAXSTARS]; /* original ywait */

    uint8_t xwait[MAXSTARS];  /* wait-until-move time: head x */
    uint8_t xtwait[MAXSTARS]; /*  tail x */
    uint8_t ywait[MAXSTARS];  /*  head y */
    uint8_t ytwait[MAXSTARS]; /*  tail y */

    uint8_t xpos[MAXSTARS];  /* current xPos (head) */
    uint8_t xtpos[MAXSTARS]; /* current xPos (tail) */
    uint8_t ypos[MAXSTARS];  /* current yPos (head) */
    uint8_t ytpos[MAXSTARS]; /* current yPos (tail) */

    uint8_t sdelay[MAXSTARS];   /* delay for tail to move */
    uint8_t headmove[MAXSTARS]; /* TRUE (>0) if head still moving */

    /* various vars */
    uint32_t seed;      /* current random seed (=last random number generated) */
    uint32_t delay_acc; /* accumulated delay in microseconds */
};

/** Delay execution an amount in 68000 DIVS instructions. */
static void wowzo_wait(struct wowzo *data, unsigned int delay)
{
    /* 80-140 cycles at 8 Mhz */
    data->delay_acc += delay * 15;

    /* accumulate delays until we reach tick granularity, then wait a tick */
    while (data->delay_acc > (tick_ms * 1000)) {
        data->delay_acc -= tick_ms * 1000;
        util_msleep(tick_ms);
    }
}

/** Deterministic random number routine, used for star position generation. */
static uint32_t wowzo_rand(struct wowzo *data)
{
    data->seed = (data->seed * 31417 + 11 + ((data->seed >> 16) & 7)) >> 3;
    return data->seed;
}

static void wowzo_dosound(struct wowzo *data, const uint8_t *sound, size_t sound_len, int warp_speed)
{
    /* warp_speed is patched into second-to-last byte if warp1 to change the sound pitch. */
    uint8_t sound_tmp[128];
    if (sound_len > 128)
        return;
    memcpy(sound_tmp, sound, sound_len);
    if (warp_speed >= 0) {
        sound_tmp[sound_len - 2] = warp_speed;
    }

    if (data->sound) {
        data->sound->play_sound(data->sound, sound_tmp, sound_len);
    }
}

static void wowzo_cplot(struct wowzo *data, uint8_t x, uint8_t y, int idx)
{
    struct game_screen_point point;
    /* choose color based on index.
       note that values passed into screen are palette indices, not GEM color indices.

       where D3%4  0   1   =15  white
                   1   2   =1   blue
                   2   6   =3   cyan
                   3   4   =4   red
     */
    switch (idx % 4) {
    case 0: point.color = 15; break;
    case 1: point.color = 1; break;
    case 2: point.color = 3; break;
    default: point.color = 4; break;
    }
    point.x = x;
    point.y = y;
    data->screen->draw_points(data->screen, 1, &point, 1);
}

static void wowzo_unplot(struct wowzo *data, uint8_t x, uint8_t y)
{
    struct game_screen_point point;
    point.color = 0;
    point.x     = x;
    point.y     = y;
    data->screen->draw_points(data->screen, 1, &point, 1);
}

static void wowzo_clear_rect(struct wowzo *data, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    data->screen->vr_recfl(data->screen, 1, 0, x0, y0, x1, y1);
}

/** This subroutine creates a new star! */
static void wowzo_new_star(struct wowzo *data, int idx, uint16_t ytop, uint16_t ywide)
{
    /* y stuff */
    uint8_t yrand    = (wowzo_rand(data) & 0x7f) % ywide + 5 + ytop;
    data->ypos[idx]  = yrand;
    data->ytpos[idx] = yrand;

    uint8_t dy, my;
    if (yrand <= GMIDY) {
        dy = 0xff;
        my = GMIDY - yrand;
    } else {
        dy = 1;
        my = yrand - GMIDY;
    }
    data->ydelta[idx] = dy;

    /* x stuff */
    uint8_t xrand    = (wowzo_rand(data) & 0xff) % (XWIDE - 10) + 5 + GLEFT;
    data->xpos[idx]  = xrand;
    data->xtpos[idx] = xrand;

    uint8_t dx, mx;
    if (xrand <= GMIDX) {
        dx = 0xff;
        mx = GMIDX - xrand;
    } else {
        dx = 1;
        mx = xrand - GMIDX;
    }
    data->xdelta[idx] = dx;

    /* adapt slopes */
    while (mx > 7 || my > 4) {
        mx >>= 1;
        my >>= 1;
    }
    mx += 1; /* make 1..8 */
    my += 1;

    /* mx is stored as yWaits and my is stored as xWaits */
    data->oxwait[idx] = data->xwait[idx] = data->xtwait[idx] = my;
    data->oywait[idx] = data->ywait[idx] = data->ytwait[idx] = mx;

    data->sdelay[idx]   = (wowzo_rand(data) & 0x3f) | 8;
    data->headmove[idx] = 1;
}

/* create random star, delay appearance */
static void wowzo_stage4_new_star(struct wowzo *data, int idx)
{
    wowzo_new_star(data, idx, GTOP + YHI / 4, YHI / 2 - 10);

    uint8_t rand_offset = wowzo_rand(data) & 0x3f;
    data->sdelay[idx] += rand_offset;
    data->xwait[idx] += rand_offset;
    data->ywait[idx] += rand_offset;
}

/** Entry point for warp effect.
 *
 * warp_failed: warp failed flag
 * distance: warp distance (1-12?)
 * seed: universe seed
 * */
void wowzo(struct game_screen *screen, struct game_sound *sound, bool warp_failed, uint16_t distance, uint16_t seed)
{
    struct wowzo data_;
    struct wowzo *data = &data_;

    memset(data, 0, sizeof(struct wowzo));
    data->screen = screen;
    data->sound  = sound;
    data->seed   = 0x03000000 | seed;

    wowzo_dosound(data, sound_warp1, sizeof(sound_warp1), 2);

    /* create a random star table loop and draw the stars that
       appear in the first stage */
    for (int idx = MAXSTARS - 1; idx >= 0; --idx) {
        wowzo_new_star(data, idx, GTOP, YHI - 10);
        if (idx <= S1STARS) {
            wowzo_cplot(data, data->xpos[idx], data->ypos[idx], idx);
        }
    }

    /* Stage 1 and 2 of Wowzo
       stars rush by, getting faster
       (check for warpFailed flag at end or in middle)
    */
    uint16_t delay1 = S1WAIT; /* delay cycle */
    int passes      = S1LPCNT;
    for (int stage = 1; stage <= 2; ++stage) {
        for (int pass = 0; pass <= passes; ++pass) {
            if (stage == 1) { /* skip time dec stuff in stage 2 */
                if (delay1 > 35) {
                    delay1 -= 4;
                } else {
                    if (warp_failed) {
                        return;
                    }
                }
            }
            wowzo_wait(data, delay1); /* dramatic pause */

            for (int idx = S1STARS; idx > 0; --idx) {
                data->xwait[idx] -= 1;
                if (data->xwait[idx] == 0) {
                    data->xwait[idx] = data->oxwait[idx];
                    if (stage == 1) { /* don't erase if stage 2 */
                        wowzo_unplot(data, data->xpos[idx], data->ypos[idx]);
                    }
                    data->xpos[idx] += data->xdelta[idx];
                    /* no compare against GRIGHT because X overflows byte on increasing, making it < left too */
                    if (data->xpos[idx] <= GLEFT) {
                        goto star_dead_1;
                    } else {
                        wowzo_cplot(data, data->xpos[idx], data->ypos[idx], idx);
                    }
                }

                data->ywait[idx] -= 1;
                if (data->ywait[idx] == 0) {
                    data->ywait[idx] = data->oywait[idx];
                    if (stage == 1) { /* don't erase if stage 2 */
                        wowzo_unplot(data, data->xpos[idx], data->ypos[idx]);
                    }
                    data->ypos[idx] += data->ydelta[idx];
                    if (data->ypos[idx] <= GTOP || data->ypos[idx] >= GBOTTOM) {
                        goto star_dead_1;
                    } else {
                        wowzo_cplot(data, data->xpos[idx], data->ypos[idx], idx);
                    }
                }
                continue;
            star_dead_1:
                /* star moved out of display area, in stage1 and 2 it will be replaced with a new star */
                wowzo_new_star(data, idx, GTOP, YHI - 10);
            } /* stars */
        } /* passes */

        if (stage == 1) {
            wowzo_dosound(data, sound_warp1, sizeof(sound_warp1), 4);
            passes = S2LPCNT;
        }
    } /* stages */

    /* Stage 3 of wowzo
       Stars streak by & diamond clears

       same as stage 1, except that:
       1) old star pos isn't erased
       2) stars that go out of bounds become inactive (xWait:= -1)
       3) every RectDelay cycles of the outer loop, the rectangle stage is
          incremented by 1 and a new set of rectangles are cleared
    */
    wowzo_dosound(data, sound_warp1, sizeof(sound_warp1), 8);
    uint16_t delay2    = RECTDELAY; /* current delay time */
    uint16_t rectstage = 0;
    for (int pass = 0; pass <= S3LPCNT; ++pass) {
        wowzo_wait(data, S3WAIT); /* dramatic pause */

        /* diamond rectangle stuff */
        delay2 -= 1;
        if (delay2 == 0) {
            delay2 = RECTDELAY;
            if (rectstage == 9) {
                /* end stage if we're out of rectangles */
                break;
            }
            for (int idx = 0; idx < num_rects[rectstage]; ++idx) {
                const uint8_t *cur_rect = rects[rect_list[rectstage] + idx];
                wowzo_clear_rect(data,
                    GLEFT + ((int)cur_rect[0]) * RXCELL,
                    GTOP + ((int)cur_rect[1]) * RYCELL,
                    GLEFT + (cur_rect[2] == NUMRXCELL ? XWIDE : ((int)cur_rect[2]) * RXCELL),
                    GTOP + (cur_rect[3] == NUMRYCELL ? YHI : ((int)cur_rect[3]) * RYCELL));
            }
            rectstage += 1; /* XXX in the original code, this is increased first, causing it to skip over stage 0 */
        }

        /* advance stars */
        for (int idx = S3STARS; idx > 0; --idx) {
            if (data->xwait[idx] == 0xff) {
                continue; /* skip dead stars */
            }
            data->xwait[idx] -= 1;
            if (data->xwait[idx] == 0) {
                data->xwait[idx] = data->oxwait[idx];
                data->xpos[idx] += data->xdelta[idx];
                /* no compare against GRIGHT because X overflows byte on increasing, making it < left too */
                if (data->xpos[idx] <= GLEFT) {
                    goto star_dead_3;
                } else {
                    wowzo_cplot(data, data->xpos[idx], data->ypos[idx], idx);
                }
            }

            data->ywait[idx] -= 1;
            if (data->ywait[idx] == 0) {
                data->ywait[idx] = data->oywait[idx];
                data->ypos[idx] += data->ydelta[idx];
                if (data->ypos[idx] <= GTOP || data->ypos[idx] >= GBOTTOM) {
                    goto star_dead_3;
                } else {
                    wowzo_cplot(data, data->xpos[idx], data->ypos[idx], idx);
                }
            }
            continue;
        star_dead_3:
            /* star moved out of display area, mark as dead */
            data->xwait[idx] = 0xff;
        } /* stars */
    } /* passes */

    /* Stage 4
       stars streak, with their tails running behind them
       stage length depends on warpDistance)
    */
    /* clear the whole grafDisp */
    wowzo_clear_rect(data, GLEFT, GTOP, GRIGHT, GBOTTOM);
    wowzo_dosound(data, sound_warp2, sizeof(sound_warp2), -1);

    /* create random stars, delay appearance */
    for (int idx = MAXSTARS - 1; idx >= 0; --idx) {
        wowzo_stage4_new_star(data, idx);
    }

    /* number of active passes, stars will die off after this */
    passes = S4LPCNT + distance * S4LPMLT;

    for (int pass = 0; pass <= 0xfff; ++pass) {
        bool stars_extinct = true;
        wowzo_wait(data, S4WAIT);

        for (int idx = S4STARS; idx > 0; --idx) {
            if (data->xwait[idx] == 0xff) {
                continue; /* skip dead stars */
            }
            stars_extinct = false;

            /* do the tail */
            if (data->sdelay[idx] == 0) { /* is tail moving? */
                data->xtwait[idx] -= 1;
                if (data->xtwait[idx] == 0) {
                    data->xtwait[idx] = data->oxwait[idx];
                    data->xtpos[idx] += data->xdelta[idx];
                    /* no compare against GRIGHT because X overflows byte on increasing, making it < left too */
                    if (data->xtpos[idx] <= GLEFT) {
                        goto star_dead_4;
                    } else { /* erase at tail pos */
                        wowzo_unplot(data, data->xtpos[idx], data->ytpos[idx]);
                    }
                }

                data->ytwait[idx] -= 1;
                if (data->ytwait[idx] == 0) {
                    data->ytwait[idx] = data->oywait[idx];
                    data->ytpos[idx] += data->ydelta[idx];
                    if (data->ytpos[idx] <= GTOP || data->ytpos[idx] >= GBOTTOM) {
                        goto star_dead_4;
                    } else { /* erase at tail pos */
                        wowzo_unplot(data, data->xtpos[idx], data->ytpos[idx]);
                    }
                }
            } else {
                data->sdelay[idx] -= 1;
            }

            /* do the head */
            if (data->headmove[idx] != 0) { /* is head moving? */
                data->xwait[idx] -= 1;
                if (data->xwait[idx] == 0) {
                    data->xwait[idx] = data->oxwait[idx];
                    data->xpos[idx] += data->xdelta[idx];
                    /* no compare against GRIGHT because X overflows byte on increasing, making it < left too */
                    if (data->xpos[idx] <= GLEFT) {
                        goto star_dead_4a;
                    } else {
                        wowzo_cplot(data, data->xpos[idx], data->ypos[idx], idx);
                    }
                }

                data->ywait[idx] -= 1;
                if (data->ywait[idx] == 0) {
                    data->ywait[idx] = data->oywait[idx];
                    data->ypos[idx] += data->ydelta[idx];
                    if (data->ypos[idx] <= GTOP || data->ypos[idx] >= GBOTTOM) {
                        goto star_dead_4a;
                    } else {
                        wowzo_cplot(data, data->xpos[idx], data->ypos[idx], idx);
                    }
                }
            }
            continue;
        star_dead_4:
            if (pass < passes) {
                wowzo_stage4_new_star(data, idx);
            } else {
                data->xwait[idx] = 0xff;
            }
            continue;
        star_dead_4a:
            /* kill star's head */
            data->headmove[idx] = 0;
            continue;
        } /* stars */

        /* exit if the stage is done, and stars are extinct */
        if (pass > passes && stars_extinct) {
            break;
        }
    } /* passes */

    wowzo_dosound(data, sound_warp3, sizeof(sound_warp3), -1);
}
