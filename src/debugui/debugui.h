#ifndef H_DEBUGUI_DEBUGUI
#define H_DEBUGUI_DEBUGUI

#include <SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

struct game_state;

void debugui_init(SDL_Window *window, struct game_state *gs);
bool debugui_is_visible(void);
bool debugui_newframe(SDL_Window *window);
bool debugui_processevent(SDL_Event *event);
void debugui_render(void);
void debugui_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
