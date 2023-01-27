libtestutil_sources = files(
    'test_util.c',
)
libtestutil = library('testutil', libtestutil_sources)

executable('set_tests', 'set_tests.c',
           include_directories: ['..'],
           link_with: [libpsys, libtestutil])
executable('inst_tests', 'inst_tests.c',
           include_directories: ['..'],
           link_with: [libpsys, libtestutil])
executable('img_tests', 'img_tests.c',
           include_directories: ['..'],
           link_with: [libpsys, libgame, libtestutil])
