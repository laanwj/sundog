/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "sundog.h"

#include "game/game_gembind.h"
#include "game/game_screen.h"
#include "game/game_shiplib.h"
#include "game/game_sound.h"
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
#ifdef PSYS_DEBUGGER
#include "game/game_debug.h"
#include "util/debugger.h"
#endif
#include "util/memutil.h"
#include "util/util_save_state.h"
#ifdef ENABLE_DEBUGUI
#include "debugui/debugui.h"
#endif
#include "sundog_resources.h"
#include "swoosh.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
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
    psys_stw(gs->psys, INTEGRITY_CHECK_ADDR(gs), 0);
}
#endif

static unsigned get_60hz_time()
{
    return SDL_GetTicks() / 17;
}

/** Procedures to ignore in tracing.
 * Make sure this is sorted by (name, func_id).
 */
static const struct psys_function_id trace_ignore_procs[] = {
    { { { "GEMBIND " } }, 0xff },
    { { { "KERNEL  " } }, 0x0f }, /* moveleft */
    { { { "KERNEL  " } }, 0x10 }, /* moveright */
    { { { "KERNEL  " } }, 0x14 }, /* time */
    { { { "MAINLIB " } }, 0x03 }, /* Sound(x) */
    { { { "MAINLIB " } }, 0x04 }, /* */
    { { { "MAINLIB " } }, 0x06 }, /* PaletteColor */
    { { { "MAINLIB " } }, 0x0b }, /* PseudoRandom */
    { { { "MAINLIB " } }, 0x24 }, /* DrawTime */
    { { { "MAINLIB " } }, 0x25 }, /* */
    { { { "MAINLIB " } }, 0x26 }, /* */
    { { { "MAINLIB " } }, 0x28 }, /* */
    { { { "MAINLIB " } }, 0x29 }, /* */
    { { { "MAINLIB " } }, 0x2a }, /* */
    { { { "MAINLIB " } }, 0x32 }, /* PaletteChange */
    { { { "MAINLIB " } }, 0x33 }, /* Nop */
    { { { "MAINLIB " } }, 0x35 }, /* */
    { { { "MAINLIB " } }, 0x36 }, /* */
    { { { "MAINLIB " } }, 0x3c }, /* */
    { { { "MAINLIB " } }, 0x3f }, /* */
    { { { "MAINLIB " } }, 0x50 }, /* TimeDigit */
    { { { "SHIPLIB " } }, 0x0f }, /* RedFlashToggle */
    { { { "SHIPLIB " } }, 0x10 }, /* */
    { { { "SHIPLIB " } }, 0x12 }, /* */
    { { { "SHIPLIB " } }, 0x1f }, /* */
    { { { "SHIPLIB " } }, 0x23 }, /* */
    { { { "SHIPLIB " } }, 0x24 }, /* */
    { { { "XDOFIGHT" } }, 0x0a }, /* */
    { { { "XDOFIGHT" } }, 0x0c }, /* */
    { { { "XDOUSERM" } }, 0x04 }, /* */
    { { { "XMOVEINB" } }, 0x02 }, /* */
    { { { "XMOVEINB" } }, 0x1b }, /* */
    { { { "XMOVEINB" } }, 0x1c }, /* */
};

/** Locations to insert artificial delays, to compensate for emulation speed.
 * Make sure this is sorted by (name, address).
 */
static const struct {
    const char *seg_name;
    uint16_t address;
    uint32_t delay_us;
} artificial_delays[] = {
    { "WINDOWLI", 0x070e, 100000 }, /* WINDOWLI:0x12 entry point on creating a dialog (see issue #18) */
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
            usleep(artificial_delays[idx].delay_us);
        }
    }

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
    int fd;
    psys_byte *disk_data;
    size_t disk_size, track_size;
    struct psys_bootstrap_info boot;
    psys_word ext_memsize     = 786; /* Memory size in kB */
    psys_fulladdr ext_membase = 0x000337ac;
    struct psys_binding *rspb;

    /* allocate memory */
    state->mem_size = (ext_memsize + 64) * 1024;
    state->memory   = malloc(state->mem_size);
    memset(state->memory, 0, state->mem_size);

    /* load disk image */
    track_size = 9 * 512;
    disk_size  = 80 * track_size;
    disk_data  = malloc(disk_size);
    fd         = open(imagename, O_RDONLY);
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
    boot.isp           = 0xfdec;
    boot.real_size     = 0;
    boot.mem_fake_base = 0x000237ac;
    boot.ext_mem_base  = boot.mem_fake_base + boot.isp;
    boot.ext_mem_size  = 0;
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

/** Initialize GL state:
 * textures, shaders, and buffers.
 */
static void init_gl(struct game_state *gs)
{
    /* Simple textured quad, for use with triangle strip */
    static float quad[] = {
        -1.0f, -1.0f, 0.0f, 1.0f,
        +1.0f, -1.0f, 1.0f, 1.0f,
        -1.0f, +1.0f, 0.0f, 0.0f,
        +1.0f, +1.0f, 1.0f, 0.0f
    };
    /* Create textures */
    /*  Screen texture is 320x200 8-bit luminance texture */
    glGenTextures(1, &gs->scr_tex);
    glBindTexture(GL_TEXTURE_2D, gs->scr_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

    /*  Palette is 256x1 32-bit RGBA texture. The width is kept at
     *  256 despite there being 16 colors to have direct byte value mapping without needing
     *  scaling in the fragment shading. The other 240 colors will be left unused.
     */
    glGenTextures(1, &gs->pal_tex);
    glBindTexture(GL_TEXTURE_2D, gs->pal_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    /* Create shaders */
    GLuint p1 = glCreateProgram();
    GLuint s2 = glCreateShader(GL_VERTEX_SHADER);
    shader_source(s2, "vertex",
        "attribute mediump vec4 vertexCoord;\n"
        "attribute mediump vec2 vertexTexCoord;\n"
        "varying mediump vec2 texcoord;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vertexCoord;\n"
        "    texcoord = vertexTexCoord;\n"
        "}\n");
    glAttachShader(p1, s2);
    glDeleteShader(s2);
    GLuint s3 = glCreateShader(GL_FRAGMENT_SHADER);
    shader_source(s3, "fragment",
        "varying mediump vec2 texcoord;\n"
        "uniform sampler2D scr_tex;\n"
        "uniform sampler2D pal_tex;\n"
        "uniform mediump vec4 tint;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    mediump vec2 idx = vec2(texture2D(scr_tex, texcoord).x, 0.0);\n"
        "    gl_FragColor = texture2D(pal_tex, idx) * tint;\n"
        "}\n");
    glAttachShader(p1, s3);
    glDeleteShader(s3);
    glBindAttribLocation(p1, 0, "vertexCoord");
    glBindAttribLocation(p1, 1, "vertexTexCoord");
    glLinkProgram(p1);
    glUseProgram(p1);
    GLint u_scr_tex = glGetUniformLocation(p1, "scr_tex");
    glUniform1i(u_scr_tex, 0);
    GLint u_pal_tex = glGetUniformLocation(p1, "pal_tex");
    glUniform1i(u_pal_tex, 1);
    gs->scr_program_tint = glGetUniformLocation(p1, "tint");
    assert(glGetError() == GL_NONE);
    gs->scr_program = p1;

    /* Build vertex buffer */
    glGenBuffers(1, &gs->vtx_buf);
    glBindBuffer(GL_ARRAY_BUFFER, gs->vtx_buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/** Draw a frame */
static void draw(struct game_state *gs)
{
    int width, height;
    SDL_GetWindowSize(gs->window, &width, &height);
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    /* No need to glClear, we'll render a full screen quad */

    glUseProgram(gs->scr_program);

    if (gs->thread) {
        glUniform4f(gs->scr_program_tint, 1.0f, 1.0f, 1.0f, 1.0f);
    } else { /* Darken if paused */
        glUniform4f(gs->scr_program_tint, 0.5f, 0.5f, 0.5f, 0.5f);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gs->scr_tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gs->pal_tex);

    glBindBuffer(GL_ARRAY_BUFFER, gs->vtx_buf);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (const GLvoid *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (const GLvoid *)(sizeof(float) * 2));
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
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
                break;
            }
            break;
        case SDL_USEREVENT:
            switch (event.user.code) {
            case EVC_TIMER: /* Timer event */
#ifdef ENABLE_DEBUGUI
                debugui_newframe(gs->window);
#endif
                /* Update textures from VM state/thread */
                game_sdlscreen_update_textures(gs->screen, gs->scr_tex, gs->pal_tex);
                /* Trigger vblank interrupt in interpreter thread */
                SDL_AtomicSet(&gs->vblank_trigger, 1);
                /* Draw a frame */
                draw(gs);
#ifdef ENABLE_DEBUGUI
                debugui_render();
#endif
                SDL_GL_SwapWindow(gs->window);
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

int main(int argc, char **argv)
{
    struct psys_state *state;
    struct game_state *gs = CALLOC_STRUCT(game_state);
    const char *imagename;

    if (argc == 2) {
        imagename = argv[1];
    } else {
        fprintf(stderr, "Usage: %s <image.st>\n", argv[0]);
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
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 400,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!gs->window) {
        psys_panic("Unable to create window: %s\n", SDL_GetError());
    }
    gs->context = SDL_GL_CreateContext(gs->window);
    if (!gs->context) {
        psys_panic("Unable to create context: %s\n", SDL_GetError());
    }

    printf("GL version: %s\n", glGetString(GL_VERSION));

    swoosh(gs->window, "swoosh/");

    init_gl(gs);

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
        gs->sound = new_sdl_sound((game_sound_loader_func*)&load_resource_sdl, "sounds/");
        if (!gs->sound) {
            printf("Warning: could not load samples, there will be no sound.\n");
        }
    }

    /* Create object to manage rendering from interpreter */
    gs->screen = new_game_screen();
    gs->psys = state      = setup_state(gs->screen, gs->sound, imagename, &gs->rspb);
    state->trace_userdata = gs;

#ifdef PSYS_DEBUGGER
    /* Set up debugger */
    gs->debugger = psys_debugger_new(state);
#endif
#ifdef ENABLE_DEBUGUI
    debugui_init(gs->window, gs);
#endif
    /* Game segment globals offsets.
     * TODO: look up get_globals("MAINLIB") i.s.o. hardcoding
     */
    gs->gembind_ofs = 0x348e;
#ifdef GAME_CHEATS
    gs->mainlib_ofs = 0x1f66;
#endif

    start_interpreter_thread(gs);

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
