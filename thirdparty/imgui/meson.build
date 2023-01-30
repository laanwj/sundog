libimgui = library('imgui',
                   sources: ['imgui.cpp', 'imgui_draw.cpp', 'imgui_tables.cpp', 'imgui_widgets.cpp', 'imgui_impl_opengl3.cpp', 'imgui_impl_sdl.cpp'],
                   dependencies: [libglxw_idep, sdl2_dep])

libimgui_idep = declare_dependency(
    include_directories: include_directories('.'),
    link_with: libimgui
)
