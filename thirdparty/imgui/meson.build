libimgui = library('imgui',
                   sources: ['imgui.cpp', 'imgui_draw.cpp', 'imgui_impl_sdl_gles2.cpp'],
                   dependencies: [libglxw_idep, sdl2_dep])

libimgui_idep = declare_dependency(
    include_directories: include_directories('.'),
    link_with: libimgui
)
