/*
 * Copyright (c) 2017-2023 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "sundog.h"

#include "game/game_gembind.h"
#include "game/game_screen.h"
#include "game/game_shiplib.h"
#include "game/game_sound.h"
#include "game_renderer.h"
#include "glutil.h"
#include "psys/psys_bootstrap.h"
#include "psys/psys_constants.h"
#include "psys/psys_debug.h"
#include "psys/psys_helpers.h"
#include "psys/psys_interpreter.h"
#include "psys/psys_opcodes.h"
#include "psys/psys_rsp.h"
#include "psys/psys_save_state.h"
#include "psys/psys_task.h"
#include "game/game_debug.h"
#ifdef PSYS_DEBUGGER
#include "util/debugger.h"
#endif
#include "util/memutil.h"
#include "util/util_minmax.h"
#include "util/util_save_state.h"
#ifdef ENABLE_DEBUGUI
#include "debugui/debugui.h"
#endif
#include "sundog_resources.h"
#include "swoosh.h"

#include <SDL.h>
#include <SDL_mixer.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* Time between vblanks in ms */
#define VBLANK_TIME (1000 / 50)

/** Mouse area on upper right corner to consider "cancel area", where clicks are interpreted as right clicks. */
#define CANCEL_AREA_W (24)
#define CANCEL_AREA_H (16)

/** User-defined event types */
enum {
    EVC_TIMER    = 0x1000,
    EVC_DEBUGGER = 0x1001
};

/** HACK: Sundog has a complex, extremely paranoid integrity checking system.
 * This may be part copy protection or to deter other kinds of tampering with the game.
 * In case an error is detected it sets the screen to red and locks up the game
 * (just google for "Sundog red screen" to find plenty of examples of people stumbling on it,
 * both in emulation and on real STs).
 * In any case it trips up while running in this interpreter, and until someone finds
 * a more elegant way around it, we persistently turn it off by resetting its state.
 */
#define INTEGRITY_CHECK_ADDR(gs) (W((gs)->gembind_ofs + 8, 0x238)) /* global GEMBIND_238 */
#ifdef DEBUG_INTEGRITY_CHECK
/** Watch ??? state */
static void watch_integrity_check(struct game_state *gs)
{
    if (!gs->gembind_ofs) {
        return;
    }
    static psys_word last_state = 0;
    psys_word new_state         = psys_ldw(gs->psys, INTEGRITY_CHECK_ADDR(gs));
    if (new_state != last_state) {
        psys_debug("\x1b[38;5;196;48;5;235m??? state changed to 0x%04x\x1b[0m\n", new_state);
        psys_print_traceback(s);
        last_state = new_state;
    }
}
#else
/** Neuter ??? state */
static void watch_integrity_check(struct game_state *gs)
{
    if (!gs->gembind_ofs) {
        return;
    }
    psys_stw(gs->psys, INTEGRITY_CHECK_ADDR(gs), 0);
}
#endif

static unsigned get_60hz_time()
{
    return SDL_GetTicks() / 17;
}

/** Procedures to ignore in tracing because they're called often.
 * Make sure this is sorted by (name, func_id).
 */
static const struct psys_function_id trace_ignore_procs[] = {
    { { { "GEMBIND " } }, 0xff }, /* all GEMBIND functions */
    { { { "KERNEL  " } }, 0x0f }, /* moveleft */
    { { { "KERNEL  " } }, 0x10 }, /* moveright */
    { { { "KERNEL  " } }, 0x14 }, /* time */
    { { { "MAINLIB " } }, 0x03 }, /* fore_sound */
    { { { "MAINLIB " } }, 0x04 }, /* back_sound */
    { { { "MAINLIB " } }, 0x06 }, /* color_value */
    { { { "MAINLIB " } }, 0x0b }, /* random */
    { { { "MAINLIB " } }, 0x24 }, /* put_time */
    { { { "MAINLIB " } }, 0x25 }, /* tick_less */
    { { { "MAINLIB " } }, 0x26 }, /* add_ticks */
    { { { "MAINLIB " } }, 0x28 }, /* timeout */
    { { { "MAINLIB " } }, 0x29 }, /* set_update */
    { { { "MAINLIB " } }, 0x2a }, /* need_update */
    { { { "MAINLIB " } }, 0x32 }, /* title_color */
    { { { "MAINLIB " } }, 0x33 }, /* util_color */
    { { { "MAINLIB " } }, 0x35 }, /* the_time */
    { { { "MAINLIB " } }, 0x36 }, /* screen_pos */
    { { { "MAINLIB " } }, 0x3c }, /* get_window */
    { { { "MAINLIB " } }, 0x3f }, /* in_r_window */
    { { { "MAINLIB " } }, 0x50 }, /* put_time:make_str */
    { { { "SHIPLIB " } }, 0x0f }, /* running_lights */
    { { { "SHIPLIB " } }, 0x10 }, /* whoosh_whoosh_lights */
    { { { "SHIPLIB " } }, 0x12 }, /* update_ship */
    { { { "SHIPLIB " } }, 0x1f }, /* update_movement */
    { { { "SHIPLIB " } }, 0x23 }, /* update_levels */
    { { { "SHIPLIB " } }, 0x24 }, /* update_levels:check_fuel */
    { { { "XDOFIGHT" } }, 0x0a }, /* replot_screen */
    { { { "XDOFIGHT" } }, 0x0c }, /* update_fight */
    { { { "XDOUSERM" } }, 0x04 }, /* update_status */
    { { { "XMOVEINB" } }, 0x02 }, /* handle_bartender */
    { { { "XMOVEINB" } }, 0x1b }, /* handle_player */
    { { { "XMOVEINB" } }, 0x1c }, /* handle_customers */
};

/** Locations to insert artificial delays (in microseconds) or waits for mouse
 * release (-1), to compensate for emulation speed. Make sure this is sorted by
 * (name, address).
 */
static const struct {
    const char *seg_name;
    uint16_t address;
    int32_t delay_us;
} artificial_delays[] = {
    { "WINDOWLI", 0x070e, -1 }, /* WINDOWLI:0x12 entry point on creating a dialog (see issue #18) */
    { "WINDOWLI", 0x09e1, -1 }, /* WINDOWLI:0x15 make_zoom return */
    { "XDOINTER", 0x0fa2, -1 }, /* XDOINTER:0x1f new_frame, on return after drawing a dialog */
};

/* Called before every instruction executed.
 * Put debug hooks and tracing here.
 * Enable with: state->trace = psys_trace;
 */
static void psys_trace(struct psys_state *s, void *gs_)
{
    struct game_state *gs = (struct game_state *)gs_;
    if (PDBG(s, TRACE)) {
        psys_print_info(s);
    }
    if (PDBG(s, TRACE_CALLS)) {
        psys_print_call_info(s,
            trace_ignore_procs, ARRAY_SIZE(trace_ignore_procs),
            get_game_debuginfo());
    }
    if (SDL_AtomicGet(&gs->stop_trigger)) {
        psys_debug("Interpreter thread stopped\n");
        psys_stop(s);
    }

    /* Insert artificial delays at set points. The reason for this is that the emulation speed
     * is so much higher than an Atari ST. The way the user interaction code is written,
     * this sometimes can give problems.
     * XXX if there's more entries switch to a bsearch like in psys_print_call_info.
     */
    const char *curseg_name = (const char *)psys_bytes(s, s->curseg + PSYS_SEG_NAME);
    size_t curaddr          = s->ipc - s->curseg;
    for (size_t idx = 0; idx < ARRAY_SIZE(artificial_delays); ++idx) {
        if (strncmp(curseg_name, artificial_delays[idx].seg_name, 8) == 0 && curaddr == artificial_delays[idx].address) {
            if (artificial_delays[idx].delay_us >= 0) { /* wait microseconds */
                usleep(artificial_delays[idx].delay_us);
            } else { /* wait for mouse release */
                unsigned buttons = 1;
                int x, y;
                while (buttons && !SDL_AtomicGet(&gs->stop_trigger)) {
                    gs->screen->vq_mouse(gs->screen, &buttons, &x, &y);
                    usleep(10000);
                }
            }
        }
    }

    /* Look up globals for important game segments once they become available. */
    if (!gs->gembind_ofs && !strncmp(curseg_name, "GEMBIND ", 8)) {
        gs->gembind_ofs = s->base;
        printf("GEMBIND globals at 0x%08x\n", gs->gembind_ofs);
    }
#ifdef GAME_CHEATS
    if (!gs->mainlib_ofs && !strncmp(curseg_name, "MAINLIB ", 8)) {
        gs->mainlib_ofs = s->base;
        printf("MAINLIB globals at 0x%08x\n", gs->mainlib_ofs);
    }
#endif

    /* Ideally this would be triggered some other way instead of polling
     * every instruction. The event should be triggered every 4 vsyncs, which
     * is 15 times a second on NTSC and 12.5 on PAL on the original Atari ST
     * version, but I'm not sure how much the exact timing matters.
     */
    if (SDL_AtomicGet(&gs->vblank_trigger)) {
        SDL_AtomicSet(&gs->vblank_trigger, 0);
        /* First, do sprite movement etc */
        gs->screen->vblank_interrupt(gs->screen);
        /* Then pass one in four events to interpreter */
        gs->vblank_count += 1;
        if (gs->vblank_count == 4) {
            psys_rsp_event(gs->rspb, 0, true);
            gs->vblank_count = 0;
        }
        /* 60hz timer */
        psys_rsp_settime(gs->rspb, get_60hz_time() - gs->time_offset);
    }
    watch_integrity_check(gs);
#ifdef PSYS_DEBUGGER
    if (psys_debugger_trace(gs->debugger)) {
        SDL_Event event;
        /* Stop interpreter loop, break into debugger */
        psys_stop(s);
        event.type      = SDL_USEREVENT;
        event.user.type = SDL_USEREVENT;
        event.user.code = EVC_DEBUGGER;
        SDL_PushEvent(&event);
    }
#endif
}

static struct psys_state *setup_state(struct game_screen *screen, struct game_sound *sound, const char *imagename, struct psys_binding **rspb_out)
{
    struct psys_state *state = CALLOC_STRUCT(psys_state);
    psys_byte *disk_data;
    size_t disk_size, track_size;
    struct psys_bootstrap_info boot;
    psys_word ext_memsize     = 786; /* Ext memory size in kB, note that this is way more than the game needs. */
    psys_fulladdr ext_membase = 0x000337ac;
    struct psys_binding *rspb;

    /* allocate memory */
    state->mem_size = (ext_memsize + 64) * 1024;
    state->memory   = malloc(state->mem_size);
    memset(state->memory, 0, state->mem_size);

    track_size = 9 * 512;
#ifdef DISK_IMAGE_AS_RESOURCE
    const void *disk_data_ro = load_resource("game/sundog.st", &disk_size);
    if (!disk_data_ro || disk_size != 80 * track_size) {
        fprintf(stderr, "Could not read disk image from resource /game/sundog.st\n");
        exit(1);
    }
    /* Make a swizzled read-write copy. */
    disk_data = malloc(disk_size);
    memcpy(disk_data + 0 * track_size, disk_data_ro + 3 * track_size, 77 * track_size);
    memcpy(disk_data + 77 * track_size, disk_data_ro + 0 * track_size, 3 * track_size);
    unload_resource(disk_data_ro);
#else
    int fd;
    /* load disk image */
    disk_size = 80 * track_size;
    disk_data = malloc(disk_size);
    fd        = open(imagename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        fprintf(stderr, "Could not open disk image %s\n", imagename);
        exit(1);
    }
    if (read(fd, disk_data + 77 * track_size, 3 * track_size) < (ssize_t)(3 * track_size)
        || read(fd, disk_data, 77 * track_size) < (ssize_t)(77 * track_size)) {
        perror("read");
        fprintf(stderr, "Could not read disk image\n");
        exit(1);
    }
    close(fd);
#endif

    /* override memory size and offset in SYSTEM.MISCINFO */
    /* This is sneaky: at boot, SUNDOG writes amount of memory and memory
     * offset to SYSTEM.MISCINFO, which is read later by the p-machine.
     * Emulate this.
     */
    *((psys_word *)&disk_data[0x1e00 + 0x22]) = F(ext_memsize);
    *((psys_word *)&disk_data[0x1e00 + 0x24]) = F(ext_membase >> 16);
    *((psys_word *)&disk_data[0x1e00 + 0x26]) = F(ext_membase & 0xffff);

    /* Bootstrap */
    boot.boot_unit_id  = PSYS_UNIT_DISK0;
    boot.isp           = 0xfdec; /* initial stack pointer - top of p-system base memory */
    boot.real_size     = 0;
    boot.mem_fake_base = ext_membase - 0x10000; /* this is where (virtually) 0x10000 bytes of p-system base memory start */
    /* was: 0x000237ac in hatari emulation */
    /* so we have:
     *
     * psys addr    "atari" addr    size     description
     * -----------------------------------------------------------------------
     * 0xffff0000                   64 kB    GEMBIND memory (game specific)
     *   0xffff0000                  21 kB    (unused, contains OS/68000 code on atari)
     *   0xffff5400
     *     =-0xac00 data_pool        31 kB    "data pool" area used by game (offset queried using psys_rsp_unitstatus 128, size fixed)
     *   0xffffd000                  12 kB    (unused, contains 68000 code on atari)
     * 0x00000000   mem_fake_base   64 kB    p-system base memory (16-bit addressable: globals, heap)
     * 0x00010000   ext_membase     768 kB   p-system ext memory (offset/size passed in SYSTEM.MISCINFO)
     *                                         used for code segments storage
     *
     * where psys_addr is the address from the viewpoint of the p-system,
     * and "atari" addr the address from more low-level kind of code (like GEM calls)
     * we've made all these relative to ext_membase which is more or less an arbitrary number to match a certain run of hatari
     *
     * XXX theoretically the p-system can address 128kB of memory directly starting from mem_fake_base, because addresses are word-based,
     * i'm not sure how this matches the 64kB here, but in any case maybe it's because a lot of internal bookkeeping (like the stack pointer)
     * does use byte addresses?
     */

    /** These two are dummy (legacy?) values and not used on Atari ST: */
    boot.ext_mem_base = boot.mem_fake_base + boot.isp;
    boot.ext_mem_size = 0;

    psys_bootstrap(state, &boot, &disk_data[track_size]);

    /* Debug setting */
    state->debug = PSYS_DBG_WARNING;
    {
        const char *x = getenv("PSYS_DEBUG");
        if (x) {
            state->debug = strtol(x, NULL, 0);
        }
    }

    /* set up RSP and disk */
    rspb = psys_new_rsp(state);
    psys_rsp_set_disk(rspb, 0, disk_data, disk_size, track_size, true);
    if (rspb_out) {
        *rspb_out = rspb;
    }

    /* Set up bindings */
    state->num_bindings = 3;
    state->bindings     = calloc(state->num_bindings, sizeof(struct binding *));
    state->bindings[0]  = rspb;
    state->bindings[1]  = new_shiplib(state, screen, sound);
    state->bindings[2]  = new_gembind(state, screen, sound);

    /* Debugging */
    state->trace = psys_trace;

    return state;
}

/* Header for savestates */
#define PSYS_SUND_STATE_ID 0x53554e44

static int game_load_state(struct game_state *gs, int fd)
{
    uint32_t id;
    if (FD_READ(fd, id)) {
        return -1;
    }
    if (id != PSYS_SUND_STATE_ID) {
        psys_debug("Invalid sundog state record %08x\n", id);
        return -1;
    }
    /* get_60hz_time() - gs->time_offset should return the same as at saving time */
    if (FD_READ(fd, gs->saved_time)) {
        return -1;
    }

    if (psys_load_state(gs->psys, fd) < 0) {
        psys_debug("Error loading p-system state\n");
        return -1;
    }
    if (game_sdlscreen_load_state(gs->screen, fd) < 0) {
        psys_debug("Error screen state\n");
        return -1;
    }
    return 0;
}

static int game_save_state(struct game_state *gs, int fd)
{
    uint32_t id = PSYS_SUND_STATE_ID;
    if (FD_WRITE(fd, id)
        || FD_WRITE(fd, gs->saved_time)) {
        return -1;
    }

    if (psys_save_state(gs->psys, fd) < 0) {
        psys_debug("Error loading p-system state\n");
        return -1;
    }
    if (game_sdlscreen_save_state(gs->screen, fd) < 0) {
        psys_debug("Error screen state\n");
        return -1;
    }
    return 0;
}

static int interpreter_thread(void *ptr)
{
    psys_interpreter((struct psys_state *)ptr);
    return 0;
}

/** Start the main interpreter thread */
static void start_interpreter_thread(struct game_state *gs)
{
    SDL_AtomicSet(&gs->stop_trigger, 0);
    gs->time_offset = get_60hz_time() - gs->saved_time; /* Set time to saved time when thread last stopped */
    gs->thread      = SDL_CreateThread(interpreter_thread, "interpreter_thread", gs->psys);
#if 0
    psys_debug("[%d] Interpreter thread started\n", get_60hz_time() - gs->time_offset);
#endif
}

/** Stop interpreter thread, and wait for it */
static void stop_interpreter_thread(struct game_state *gs)
{
    SDL_AtomicSet(&gs->stop_trigger, 1);
    SDL_WaitThread(gs->thread, NULL);
    gs->saved_time = get_60hz_time() - gs->time_offset; /* Write current time */
    gs->thread     = NULL;
#if 0
    psys_debug("[%d] Interpreter thread stopped\n", gs->saved_time);
#endif
}

/** Draw a frame */
static void draw(struct game_state *gs)
{
    int width, height;
    SDL_GetWindowSize(gs->window, &width, &height);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(gs->viewport[0], gs->viewport[1], gs->viewport[2], gs->viewport[3]);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    float tint[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    if (!gs->thread) { /* darken tint if paused */
        tint[0] = 0.5f;
        tint[1] = 0.5f;
        tint[2] = 0.5f;
        tint[3] = 0.5f;
    }
    gs->renderer->draw(gs->renderer, tint);
}

/** Window was resized */
static void update_window_size(struct game_state *gs)
{
    int width, height;

    SDL_GetWindowSize(gs->window, &width, &height);
    compute_viewport_fixed_ratio(width, height, SCREEN_WIDTH, SCREEN_HEIGHT, gs->viewport);
    gs->force_redraw = true;
}

/** Mouse button pressed or mouse moved */
static void update_mouse_state(struct game_state *gs)
{
    if (gs->input_bypass) {
        return;
    }

    int sx, sy;
    uint32_t sb      = SDL_GetMouseState(&sx, &sy);
    unsigned buttons = 0;
    if (sb & SDL_BUTTON(SDL_BUTTON_LEFT)) {
        buttons |= 1;
    }
    if (sb & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
        buttons |= 2;
    }
    int x = (sx - gs->viewport[0]) * SCREEN_WIDTH / imax(gs->viewport[2], 1);
    int y = (sy - gs->viewport[1]) * SCREEN_HEIGHT / imax(gs->viewport[3], 1);

    /* Emulate right click action when clicking (or touching) in top right,
       to accomodate single mouse button devices.
      */
    if ((buttons == 1) && x >= (320 - CANCEL_AREA_W) && y < CANCEL_AREA_H) {
        buttons = 2;
    }
    game_sdlscreen_update_mouse(gs->screen, x, y, buttons);
}

/** SDL timer callback. This just sends an event to the main thread.
 */
static Uint32 timer_callback(Uint32 interval, void *param)
{
    struct game_state *gs = (struct game_state *)param;
    SDL_Event event;
    if (!SDL_AtomicGet(&gs->timer_queued)) {
        SDL_AtomicSet(&gs->timer_queued, 1);
        event.type      = SDL_USEREVENT;
        event.user.type = SDL_USEREVENT;
        event.user.code = EVC_TIMER;
        SDL_PushEvent(&event);
    }
    return interval;
}

static void event_loop(struct game_state *gs)
{
    /* TODO: how well does this work if we're not able to render
     * 50fps? Probably badly: there's no framedrop support.
     */
    SDL_Event event;
    bool need_redraw;
#ifdef GAME_CHEATS
    psys_byte gamestate[512];
    psys_byte hl[512];
#endif
    gs->running = true;
    while (gs->running && SDL_WaitEvent(&event)) {
#ifdef ENABLE_DEBUGUI
        if (debugui_processevent(&event)) {
            continue;
        }
#endif
        switch (event.type) {
        case SDL_QUIT:
            gs->running = false;
            break;
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
#ifdef GAME_CHEATS
            case SDLK_x: { /* Cheat */
                int x;
                psys_stw(gs->psys, W(gs->mainlib_ofs + 8, 0x33), 999);
                psys_stw(gs->psys, W(gs->mainlib_ofs + 8, 0x34), 999);
                /* Ship's stores */
                for (x = 0; x < 20; ++x) {
                    psys_stb(gs->psys, W(gs->mainlib_ofs + 8, 0xbd), x, x + 8);
                }
                /* Ship's locker */
                for (x = 0; x < 20; ++x) {
                    psys_stb(gs->psys, W(gs->mainlib_ofs + 8, 0xaf), x, x + 12);
                }
                psys_debug("\x1b[104;31mCHEATER\x1b[0m\n");
            } break;
            case SDLK_y: { /* Dump gamestate as hex */
                unsigned i;
                psys_byte *curstate = psys_bytes(gs->psys, W(gs->mainlib_ofs + 8, 0x1f));
                for (i = 0; i < 512; ++i) {
                    if (curstate[i] != gamestate[i]) {
                        hl[i]        = 0x02; /* Highlight changed bytes since last time in green */
                        gamestate[i] = curstate[i];
                    } else {
                        hl[i] = 0;
                    }
                }
                psys_debug_hexdump_ofshl(curstate, 0, 512, hl);
                psys_debug("\n");
            } break;
#endif
            case SDLK_t: /* Print timer */
                psys_debug("Time: %d\n", get_60hz_time() - gs->time_offset);
                break;
            case SDLK_d: /* Go to interactive debugger */
#ifdef PSYS_DEBUGGER
                stop_interpreter_thread(gs); /* stop interpreter thread while debugging */
                psys_debugger_run(gs->debugger, true);
                start_interpreter_thread(gs);
#else
                psys_debug("Internal debugger was not compiled in.\n");
#endif
                break;
            case SDLK_s: {                   /* Save state */
                stop_interpreter_thread(gs); /* stop interpreter thread while saving */
                int fd = open("sundog.sav", O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (fd < 0) {
                    psys_debug("Error opening game state file for writing\n");
                    break;
                }
                if (game_save_state(gs, fd) < 0) {
                    psys_debug("Error during save of game state\n");
                } else {
                    psys_debug("Game state succesfully saved\n");
                }
                start_interpreter_thread(gs);
                close(fd);
            } break;
            case SDLK_l: {                   /* Load state */
                stop_interpreter_thread(gs); /* stop interpreter thread while loading */
                int fd = open("sundog.sav", O_RDONLY);
                if (fd < 0) {
                    psys_debug("Error opening game state file for reading\n");
                    break;
                }
                if (game_load_state(gs, fd) < 0) {
                    psys_debug("Error during load of game state\n");
                    /* Don't bother restarting the interpreter after failed load... */
                } else {
                    psys_debug("Game state succesfully restored\n");
                    start_interpreter_thread(gs);
                }
                close(fd);
            } break;
            case SDLK_SPACE: /* Pause */
                if (!gs->thread) {
                    start_interpreter_thread(gs);
                } else {
                    stop_interpreter_thread(gs);
                }
                gs->force_redraw = true;
                break;
            }
            break;
        case SDL_MOUSEMOTION:
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN:
            update_mouse_state(gs);
            break;
        case SDL_WINDOWEVENT:
            switch (event.window.event) {
            case SDL_WINDOWEVENT_EXPOSED:
                gs->force_redraw = true;
                break;
            case SDL_WINDOWEVENT_RESIZED:
                update_window_size(gs);
                break;
            }
            break;
        case SDL_USEREVENT:
            switch (event.user.code) {
            case EVC_TIMER: /* Timer event */
                /* Update textures and uniforms from VM state/thread */
                need_redraw = game_sdlscreen_update_textures(gs->screen, gs->renderer, (update_texture_func *)gs->renderer->update_texture, (update_palette_func *)gs->renderer->update_palette);
#ifdef ENABLE_DEBUGUI
                need_redraw |= debugui_is_visible();
#endif
                if (need_redraw || gs->force_redraw) {
#ifdef ENABLE_DEBUGUI
                    if (debugui_newframe(gs->window)) {
                        gs->input_bypass = true;
                        game_sdlscreen_update_mouse(gs->screen, 0, 0, 0);
                    } else {
                        gs->input_bypass = false;
                    }
#endif
                    /* Draw a frame */
                    draw(gs);
#ifdef ENABLE_DEBUGUI
                    debugui_render();
#endif
                    SDL_GL_SwapWindow(gs->window);
                    gs->force_redraw = false;
                }
                /* Trigger vblank interrupt in interpreter thread */
                SDL_AtomicSet(&gs->vblank_trigger, 1);
                /* Change cursor (if needed) */
                game_sdlscreen_update_cursor(gs->screen, (void **)&gs->cursor);
                /* Congestion control */
                SDL_AtomicSet(&gs->timer_queued, 0);
                break;
#ifdef PSYS_DEBUGGER
            case EVC_DEBUGGER: /* Break into debugger */
                psys_debugger_run(gs->debugger, false);
                start_interpreter_thread(gs);
                break;
#endif
            }
            break;
        }
    }
}

/** List of supported renderers with instantiation function. */
const struct renderer_desc {
    const char *name;
    struct game_renderer *(*new_renderer)(void);
} renderer_names[] = {
    { "basic", new_renderer_basic },
    { "hq4x", new_renderer_hq4x },
};

int main(int argc, char **argv)
{
    struct psys_state *state;
    struct game_state *gs                     = CALLOC_STRUCT(game_state);
    const char *image_name                    = NULL;
    const struct renderer_desc *renderer_type = &renderer_names[0];
    bool print_usage                          = false;
    bool fullscreen                           = false;

    /** Command-line argument parsing. */
    for (int argidx = 1; argidx < argc; ++argidx) {
        const char *arg = argv[argidx];
        if (arg[0] == '-') {
            if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
                print_usage = true;
                break;
            } else if (strcmp(arg, "--fullscreen") == 0) {
                fullscreen = true;
            } else if (strcmp(arg, "--renderer") == 0) {
                argidx += 1;
                if (argidx == argc) {
                    fprintf(stderr, "Missing argument for %s\n", arg);
                    print_usage = true;
                    break;
                }
                const char *renderer_name = argv[argidx];
                renderer_type             = NULL;
                for (size_t renderidx = 0; renderidx < ARRAY_SIZE(renderer_names); ++renderidx) {
                    if (strcmp(renderer_name, renderer_names[renderidx].name) == 0) {
                        renderer_type = &renderer_names[renderidx];
                    }
                }
                if (renderer_type == NULL) {
                    fprintf(stderr, "Unknown renderer specified: %s\n", renderer_name);
                    print_usage = true;
                    break;
                }
            } else {
                fprintf(stderr, "Unknown argument: %s\n", arg);
                print_usage = true;
                break;
            }
        } else {
            /* Loose argument: consider as image name */
            if (image_name == NULL) {
                image_name = argv[argidx];
            } else {
                fprintf(stderr, "Surplus argument: %s\n", arg);
                print_usage = true;
                break;
            }
        }
    }
#ifndef DISK_IMAGE_AS_RESOURCE
    if (!image_name) {
        print_usage = true;
    }
#endif
    if (print_usage) {
        fprintf(stderr, "Usage: %s [--renderer (basic|hq4x)] <image.st>\n", argv[0]);
        fprintf(stderr, "\n");
        fprintf(stderr, "      --fullscreen Make window initially fullscreen.\n");
        fprintf(stderr, "      --renderer   Set renderer to use (\"basic\" or \"hq4x\"), default is \"basic\". Renderers other than \"basic\" require a OpenGL 3 compatible GPU.\n");
        fprintf(stderr, "      --help       Display this help and exit.\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "For copyright reasons this game does not come with the resources nor game code.\n");
        fprintf(stderr, "It requires the user to provide the 360K `.st` raw disk image of the game to run.\n");
        exit(1);
    }

#ifdef SDL_HINT_NO_SIGNAL_HANDLERS
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1"); /* Allow ctrl-c to quit */
#endif
    SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");
    if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
        psys_panic("Unable to initialize SDL: %s\n", SDL_GetError());
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    gs->window = SDL_CreateWindow("SunDog: Frozen Legacy",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 320 * 4, 200 * 4,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | (fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
    if (!gs->window) {
        psys_panic("Unable to create window: %s\n", SDL_GetError());
    }
    gs->context = SDL_GL_CreateContext(gs->window);
    if (!gs->context) {
        psys_panic("Unable to create context: %s\n", SDL_GetError());
    }

    if (!load_gles3_procs()) {
        psys_panic("Unable to load OpenGL ES 2/3 functions.\n");
    }
    printf("GL version: %s\n", glGetString(GL_VERSION));

    gs->renderer = renderer_type->new_renderer();
    if (gs->renderer == NULL) {
        fprintf(stderr, "Could not initialize renderer %s, it might need GPU features that are not available.\n", renderer_type->name);
        exit(1);
    }

    swoosh(gs->window, gs->renderer, "swoosh/");

    /* Set up "vblank" timer */
    gs->vblank_count = 0;
    gs->time_offset  = get_60hz_time();
    gs->saved_time   = 0;
    gs->timer        = SDL_AddTimer(VBLANK_TIME, &timer_callback, gs);

    /* Create object to manage sound */
    if (Mix_Init(MIX_INIT_OGG) < 0 || Mix_OpenAudio(44100, AUDIO_S16SYS, 1, 512) < 0 || Mix_AllocateChannels(1) < 0) {
        printf("Warning: unable to initialize SDLMixer (%s) for mono ogg playback at 44100Hz, there will be no sound.\n", SDL_GetError());
        gs->sound = 0;
    } else {
        gs->sound = new_sdl_sound((game_sound_loader_func *)&load_resource_sdl, "sounds/");
        if (!gs->sound) {
            printf("Warning: could not load samples, there will be no sound.\n");
        }
    }

    /* Create object to manage rendering from interpreter */
    gs->screen = new_game_screen();
    gs->psys = state      = setup_state(gs->screen, gs->sound, image_name, &gs->rspb);
    state->trace_userdata = gs;

#ifdef PSYS_DEBUGGER
    /* Set up debugger */
    gs->debugger = psys_debugger_new(state);
#endif
#ifdef ENABLE_DEBUGUI
    debugui_init(gs->window, gs);
#endif
    start_interpreter_thread(gs);

    update_window_size(gs);
    event_loop(gs);

    stop_interpreter_thread(gs);

/* Destroy everything */
#ifdef ENABLE_DEBUGUI
    debugui_shutdown();
#endif
    SDL_GL_DeleteContext(gs->context);
    gs->screen->destroy(gs->screen);
    gs->sound->destroy(gs->sound);
    /* TODO these leak:
     * rsp
     * bindings
     */
    free(state);
    free(gs);

    Mix_CloseAudio();
    Mix_Quit();
    return 0;
}
