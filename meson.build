project('sundog', 'c', 'cpp',
        default_options: ['default_library=static', 'c_std=c99', 'warning_level=2'])

sdl2_dep = dependency('SDL2')
sdl2_mixer_dep = dependency('SDL2_mixer')
m_lib = meson.get_compiler('c').find_library('m', required: false)

add_project_arguments(
    '-Wmissing-declarations',
    '-Wno-unused-parameter',
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

add_project_arguments(
    '-Werror=implicit-function-declaration',
    '-Wmissing-prototypes',
    '-D_POSIX_C_SOURCE=200809L',
    language: ['c'])

# shut up some warnings for imgui build
add_project_arguments(
    '-Wno-class-memaccess',
    '-Wno-missing-declarations',
    language: ['cpp']
)

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

    'sounds/atck_siren.ogg',
    'sounds/cannon.ogg',
    'sounds/disk_dead.ogg',
    'sounds/disk_warn.ogg',
    'sounds/distress.ogg',
    'sounds/explosion.ogg',
    'sounds/fuel_here.ogg',
    'sounds/hail.ogg',
    'sounds/hit_hull.ogg',
    'sounds/hit_pirate.ogg',
    'sounds/hit_shield.ogg',
    'sounds/jackpot.ogg',
    'sounds/jettison.ogg',
    'sounds/laser.ogg',
    'sounds/menu_click.ogg',
    'sounds/nme_laser.ogg',
    'sounds/off.ogg',
    'sounds/part_dies.ogg',
    'sounds/phase_done.ogg',
    'sounds/pir_laser.ogg',
    'sounds/pod.ogg',
    'sounds/rpair_bay.ogg',
    'sounds/ship_arivs.ogg',
    'sounds/stinger.ogg',
    'sounds/tele_down.ogg',
    'sounds/tele_up.ogg',
    'sounds/tractor.ogg',
    'sounds/warp1_speed2.ogg',
    'sounds/warp1_speed4.ogg',
    'sounds/warp1_speed8.ogg',
    'sounds/warp2.ogg',
    'sounds/warp3.ogg',
    'sounds/warp_fail.ogg',

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

subdir('src')
