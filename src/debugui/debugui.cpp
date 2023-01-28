#include "debugui.h"

#include "game/game_gembind.h"
#include "game/game_screen.h"
#include "game/game_shiplib.h"
#include "psys/psys_bootstrap.h"
#include "psys/psys_constants.h"
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
static bool show_palette_window = false;
static bool show_memory_window = false;
static bool show_segments_window = false;
static struct game_state *gamestate;
static MemoryEditor mem_edit;

void debugui_init(SDL_Window *window, struct game_state *gs)
{
    ImGui_ImplSdlGLES2_Init(window);
    gamestate = gs;
}

bool debugui_is_visible(void)
{
    return show_window || show_palette_window || show_memory_window || show_segments_window;
}

static bool is_segment_resident(struct psys_state *s, psys_word erec)
{
    psys_word sib = psys_ldw(s, erec + PSYS_EREC_Env_SIB);
    /* seg_base will be 0 if not resident */
    psys_word seg_base = psys_ldw(s, sib + PSYS_SIB_Seg_Base);
    return seg_base != 0 ? true : false;
}
static void debugui_list_segments(struct psys_state *s)
{
    psys_word erec = psys_debug_first_erec_ptr(s);
    static psys_word selected;
    ImGui::Columns(6, NULL, false);
    ImGui::SetColumnWidth(0, 40);
    ImGui::SetColumnWidth(1, 40);
    ImGui::SetColumnWidth(2, 40);
    ImGui::SetColumnWidth(3, 100);
    ImGui::SetColumnWidth(4, 40);
    ImGui::SetColumnWidth(5, 40);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
    ImGui::TextUnformatted("erec");
    ImGui::NextColumn();
    ImGui::TextUnformatted("sib");
    ImGui::NextColumn();
    ImGui::TextUnformatted("flg");
    ImGui::NextColumn();
    ImGui::TextUnformatted("segname");
    ImGui::NextColumn();
    ImGui::TextUnformatted("base");
    ImGui::NextColumn();
    ImGui::TextUnformatted("size");
    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::PopStyleColor();
    while (erec) {
        psys_word sib = psys_ldw(s, erec + PSYS_EREC_Env_SIB);
        psys_word data_base = psys_ldw(s, erec + PSYS_EREC_Env_Data); /* globals start */
        psys_word data_size = psys_ldw(s, sib + PSYS_SIB_Data_Size) * 2; /* globals size in bytes */
        if (data_size) { /* Add size of MSCW/activation record */
            data_size += 0x0a;
        }

        /* Print main segment */
        char buf[16];
        sprintf(buf, "%04x", erec);
        if (ImGui::Selectable(buf, selected == erec, ImGuiSelectableFlags_SpanAllColumns)) {
            selected = erec;
            // jump and highlight segment's data in hex editor
            mem_edit.GotoAddrAndHighlight(data_base, data_base + data_size);
        }
        ImGui::NextColumn();
        ImGui::Text("%04x", sib);
        ImGui::NextColumn();
        ImGui::Text("%c", is_segment_resident(s, erec) ? 'R':'-');
        ImGui::NextColumn();
        ImGui::Text("%-8.8s", psys_bytes(s, sib + PSYS_SIB_Seg_Name));
        ImGui::NextColumn();
        ImGui::Text("%04x", data_base);
        ImGui::NextColumn();
        ImGui::Text("%04x", data_size);
        ImGui::NextColumn();

        psys_word evec = psys_ldw(s, erec + PSYS_EREC_Env_Vect);
        psys_word num_evec = psys_ldw(s, W(evec, 0));
        /* Subsidiary segments. These will be referenced in the segment's evec
         * and have the same evec pointer (and the same BASE, but that's less reliable
         * as some segments have no globals).
         */
        for (int i=1; i<num_evec; ++i) {
            psys_word serec = psys_ldw(s, W(evec,i));
            if (serec) {
                /* Print subsidiary segment */
                psys_word ssib = psys_ldw(s, serec + PSYS_EREC_Env_SIB);
                psys_word sevec = psys_ldw(s, serec + PSYS_EREC_Env_Vect);
                if (serec != erec && sevec == evec) {
                    char buf2[16];
                    sprintf(buf2, "%04x", serec);
                    if (ImGui::Selectable(buf2, selected == serec, ImGuiSelectableFlags_SpanAllColumns)) {
                        selected = serec;
                        // jump and highlight segment's data in hex editor
                        mem_edit.GotoAddrAndHighlight(data_base, data_base + data_size);
                    }
                    ImGui::NextColumn();
                    ImGui::Text("%04x", ssib);
                    ImGui::NextColumn();
                    ImGui::Text("%c", is_segment_resident(s, serec) ? 'R':'-');
                    ImGui::NextColumn();
                    ImGui::Text(" %-8.8s", psys_bytes(s, ssib + PSYS_SIB_Seg_Name));
                    ImGui::NextColumn();
                    ImGui::NextColumn();
                    ImGui::NextColumn();
                }
            }
        }
        erec = psys_ldw(s, erec + PSYS_EREC_Next_Rec);
    }
    ImGui::Columns(1);
}

bool debugui_newframe(SDL_Window *window)
{
    ImGui_ImplSdlGLES2_NewFrame(window);

    if (show_window)
    {
        ImGui::Begin("Debug");
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f,0.5f,0.0f,1.0f));
        ImGui::TextUnformatted("Su");
        ImGui::PopStyleColor();
        ImGui::SameLine(0,0);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f,0.75f,0.0f,1.0f));
        ImGui::TextUnformatted("nd");
        ImGui::PopStyleColor();
        ImGui::SameLine(0,0);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f,1.0f,0.0f,1.0f));
        ImGui::TextUnformatted("og");
        ImGui::PopStyleColor();

        if (ImGui::Button("Palette")) show_palette_window ^= 1;
        ImGui::SameLine();
        if (ImGui::Button("Memory")) show_memory_window ^= 1;
        ImGui::SameLine();
        if (ImGui::Button("Segments")) show_segments_window ^= 1;

        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

#if 0 /* XXX needs to be updated for new renderer system */
    if (show_palette_window)
    {
        ImGui::Begin("Palette", &show_palette_window);
        for (int x=0; x<16; ++x) {
            ImGui::Image((ImTextureID)(intptr_t)gamestate->pal_tex, ImVec2(5,5), ImVec2(x/256.0,0.0), ImVec2((x+1)/256.0,1.0));
            if (x != 15) {
                ImGui::SameLine(0, 2);
            }
        }
        ImGui::End();
    }
#endif

    if (show_memory_window)
    {
        assert(gamestate->psys);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f,0.0f,0.0f,0.95f)); // Less transparent
        ImGui::Begin("Memory", &show_memory_window);
        mem_edit.DrawContents(gamestate->psys->memory, gamestate->psys->mem_size, 0);
        ImGui::End();
        ImGui::PopStyleColor();
    }

    if (show_segments_window)
    {
        ImGui::SetNextWindowSize(ImVec2(320,280), ImGuiCond_FirstUseEver);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f,0.0f,0.0f,0.90f)); // Less transparent
        ImGui::Begin("Segments", &show_segments_window);
        assert(gamestate->psys);
        debugui_list_segments(gamestate->psys);
        ImGui::End();
        ImGui::PopStyleColor();
    }

    // TODO: instruction view/single step, when VM is stopped

    // If ImGui wants to capture mouse, block game from processing mouse position/buttons
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse;
}

bool debugui_processevent(SDL_Event *event)
{
    ImGuiIO& io = ImGui::GetIO();
    switch (event->type) {
    case SDL_KEYDOWN:
        switch (event->key.keysym.sym) {
        case SDLK_BACKQUOTE: /* Debug window */
            show_window = !show_window;
            return true;
        }
        break;
    default:
        break;
    }
    if (io.WantCaptureKeyboard || io.WantCaptureMouse) { /* Send events to debug window? */
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

