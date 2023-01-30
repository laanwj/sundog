subdir('glxw')

libglxw = library('glxw', sources: gen_glxw,
                  include_directories: ['../opengl'],
                  dependencies: [sdl2_dep])

libglxw_idep = declare_dependency(
    sources: gen_glxw[0],
    include_directories: include_directories('.', '../opengl'),
    link_with: libglxw
)
