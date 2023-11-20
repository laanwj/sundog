project('sundog', 'c', 'cpp',
        default_options: ['default_library=static', 'c_std=c99', 'cpp_std=c++11', 'warning_level=2'])

sdl2_dep = dependency('SDL2')
sdl2main_dep = dependency('SDL2main', required: false)
m_lib = meson.get_compiler('c').find_library('m', required: false)

# Flip p-system endian (XXX should only be required on little-endian systems)
add_project_arguments(
    '-DFLIP_ENDIAN_HACK',
    language: ['c', 'cpp']
)
if get_option('psys_debugger')
    add_project_arguments(
        '-DPSYS_DEBUGGER',
        language: ['c', 'cpp']
    )
    readline_dep = dependency('readline')
else
    readline_dep = []
endif
if get_option('debug_ui')
    add_project_arguments(
        '-DENABLE_DEBUGUI',
        language: ['c', 'cpp']
    )
endif
if get_option('game_cheats')
    add_project_arguments(
        '-DGAME_CHEATS',
        language: ['c', 'cpp']
    )
endif

# C-standard settings.
if host_machine.system() == 'darwin'
    add_project_arguments(
        '-D_DARWIN_C_SOURCE=1', # MacOS memset_pattern4 for SDL
        language: ['c'])
endif

# Warning overrides.
if meson.get_compiler('c').get_id() == 'msvc'
    # XXX MSVC specific warning disables
else
    add_project_arguments(
        '-Wmissing-declarations',
        '-Wno-unused-parameter',
        '-Werror=implicit-function-declaration',
        '-Wmissing-prototypes',
        language: ['c'])
    # shut up some warnings for imgui build
    add_project_arguments(
        '-Wno-unused-parameter',
        '-Wno-class-memaccess',
        '-Wno-missing-declarations',
        language: ['cpp']
    )
endif

resources = files(
    'swoosh/frame000.bmp',
    'swoosh/frame001.bmp',
    'swoosh/frame002.bmp',
    'swoosh/frame003.bmp',
    'swoosh/frame004.bmp',
    'swoosh/frame005.bmp',
    'swoosh/frame006.bmp',
    'swoosh/frame007.bmp',
    'swoosh/frame008.bmp',
    'swoosh/frame009.bmp',
    'swoosh/frame010.bmp',

    'shaders/screen-hq4x.vert',
    'shaders/screen-hqish.vert',
    'shaders/screen.vert',
    'shaders/screen.frag',
    'shaders/screen-hq4x.frag',
    'shaders/screen-hqish.frag',
    'shaders/hq4x.bmp.gz',
)
if get_option('builtin_image')
    add_project_arguments(
        '-DDISK_IMAGE_AS_RESOURCE',
        language: ['c', 'cpp']
    )
    resources += files('game/sundog.st')
endif

prog_python = import('python').find_installation('python3')
resource_comp = files('tools/resource_compiler.py')
debug_info = files('tools/gen_debug_info.py', 'tools/libcalls_list.py', 'tools/appcalls_list.py')

subdir('thirdparty')
subdir('src')
