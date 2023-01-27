subdir('game')

gen_resource = custom_target('gen-resource',
                             input: [resource_comp, resources],
                             output: 'sundog_resource_data.h',
                             command: [prog_python,
                                       resource_comp,
                                       '-o',
                                       '@OUTPUT@',
                                       '-b',
                                       meson.project_source_root(),
                                       resources,
                                      ])

libpsys_sources = files(
    'psys/psys_bootstrap.c',
    'psys/psys_debug.c',
    'psys/psys_interpreter.c',
    'psys/psys_opcodes.c',
    'psys/psys_registers.c',
    'psys/psys_rsp.c',
    'psys/psys_save_state.c',
    'psys/psys_set.c',
    'psys/psys_task.c',
)
libpsys = library('psys', sources: libpsys_sources)

libgame_sources = files(
    'game/game_gembind.c',
    'game/game_screen.c',
    'game/game_shiplib.c',
    'game/game_sound.c',
    'game/game_debug.c',
    'game/wowzo.c',
    'util/debugger.c',
    'util/util_img.c',
    'util/util_time.c',
)
libgame = library('game', sources: [libgame_sources, gen_debuginfo],
                  dependencies: [sdl2_dep, readline_dep])

if get_option('debug_ui')
    libdebugui_sources = files(
        'debugui/debugui.cpp',
        'imgui/imgui.cpp',
        'imgui/imgui_demo.cpp',
        'imgui/imgui_draw.cpp',
        'imgui/imgui_impl_sdl_gles2.cpp',
    )
    libdebugui = library('debugui', sources: libdebugui_sources,
                      include_directories: ['imgui'],
                      dependencies: [sdl2_dep])
else
    libdebugui = []
endif

sundog_sources = files(
    'sundog.c',
    'sundog_resources.c',
    'swoosh.c',
    'glutil.c',
    'glxw/glxw_es3.c',
    'renderer_basic.c',
    'renderer_hq4x.c',
)
executable('sundog', [sundog_sources, gen_resource],
           link_with: [libpsys, libgame, libdebugui],
           win_subsystem: 'windows',
           dependencies: [sdl2_dep, sdl2_mixer_dep, m_lib])

executable('sundog_compare_trace', 'sundog_compare_trace.c',
           link_with: [libpsys, libgame],
           win_subsystem: 'windows',
           dependencies: [sdl2_dep, m_lib])

executable('rip_images', ['rip_images.c', 'util/write_bmp.c'],
           link_with: [libpsys, libgame],
           dependencies: [m_lib])

subdir('test')
