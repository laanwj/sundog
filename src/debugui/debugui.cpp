#include "debugui.h"

#include "game/game_gembind.h"
#include "game/game_screen.h"
#include "game/game_shiplib.h"
#include "psys/psys_bootstrap.h"
#include "psys/psys_debug.h"
#include "psys/psys_helpers.h"
#include "psys/psys_interpreter.h"
#include "psys/psys_opcodes.h"
#include "psys/psys_rsp.h"
#include "psys/psys_save_state.h"
#include "psys/psys_task.h"
#include "sundog.h"

#include <imgui.h>
#include <imgui_impl_sdl_gles2.h>
#include <imgui_memory_editor.h>

#include <stdio.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#include <SDL_opengles2.h>
#endif

static bool show_window = false;
static bool show_test_window = false;
static bool show_palette_window = false;
static bool show_memory_window = false;
static struct game_state *gamestate;

void debugui_init(SDL_Window *window, struct game_state *gs)
{
    ImGui_ImplSdlGLES2_Init(window);
    gamestate = gs;
}

void debugui_newframe(SDL_Window *window)
{
    ImGui_ImplSdlGLES2_NewFrame(window);

    if (show_window)
    {
        ImGui::Begin("Debug");
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f,0.5f,0.5f,1.0f));
        ImGui::TextUnformatted("Su");
        ImGui::PopStyleColor();
        ImGui::SameLine(0,0);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f,1.0f,0.5f,1.0f));
        ImGui::TextUnformatted("nd");
        ImGui::PopStyleColor();
        ImGui::SameLine(0,0);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f,0.5f,1.0f,1.0f));
        ImGui::TextUnformatted("og");
        ImGui::PopStyleColor();

        if (ImGui::Button("Palette")) show_palette_window ^= 1;
        ImGui::SameLine();
        if (ImGui::Button("Memory")) show_memory_window ^= 1;
        ImGui::SameLine();
        if (ImGui::Button("Test Window")) show_test_window ^= 1;

        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    if (show_palette_window)
    {
        ImGui::Begin("Palette", &show_palette_window);
        for (int x=0; x<16; ++x) {
            ImGui::Image((ImTextureID)gamestate->pal_tex, ImVec2(5,5), ImVec2(x/256.0,0.0), ImVec2((x+1)/256.0,1.0));
            if (x != 15) {
                ImGui::SameLine(0, 2);
            }
        }
        ImGui::End();
    }

    if (show_memory_window)
    {
        // TODO: open or jump to certain segment's data
        static MemoryEditor mem_edit;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f,0.0f,0.0f,0.95f)); // Less transparent
        ImGui::Begin("Memory");
        mem_edit.DrawContents(gamestate->psys->memory, gamestate->psys->mem_size, 0);
        ImGui::End();
        ImGui::PopStyleColor();
    }

    if (show_test_window)
    {
        ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
        ImGui::ShowTestWindow(&show_test_window);
    }
}

bool debugui_processevent(SDL_Event *event)
{
    switch (event->type) {
    case SDL_KEYDOWN:
        switch (event->key.keysym.sym) {
        case SDLK_BACKQUOTE: /* Debug window */
            show_window = !show_window;
            game_sdlscreen_set_input_bypass(gamestate->screen, show_window);
            return true;
        }
        break;
    default:
        break;
    }
    if (show_window) { /* Send events to debug window if it's visible */
        return ImGui_ImplSdlGLES2_ProcessEvent(event);
    } else {
        return false;
    }
}

void debugui_render(void)
{
    ImGui::Render();
}

void debugui_shutdown(void)
{
    ImGui_ImplSdlGLES2_Shutdown();
}

