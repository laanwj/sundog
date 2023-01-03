#ifndef H_SUNDOG
#define H_SUNDOG

#include <SDL.h>

#include <stdbool.h>

struct psys_state;
struct psys_binding;
struct game_screen;
struct game_renderer;

struct game_state {
    bool running;
    SDL_Window *window;
    SDL_GLContext context;
    SDL_Thread *thread;
    struct game_screen *screen;
    struct game_sound *sound;
    SDL_Cursor *cursor;
    SDL_TimerID timer;

    struct psys_state *psys;
    struct psys_binding *rspb;
#ifdef PSYS_DEBUGGER
    struct psys_debugger *debugger;
#endif
    uint32_t gembind_ofs;
#ifdef GAME_CHEATS
    uint32_t mainlib_ofs;
#endif

    SDL_atomic_t timer_queued;
    SDL_atomic_t stop_trigger;
    SDL_atomic_t vblank_trigger;
    unsigned vblank_count;
    unsigned time_offset;
    uint32_t saved_time;

    struct game_renderer *renderer;
};

#endif
