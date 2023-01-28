libtestutil_sources = files(
    'test_util.c',
)
libtestutil = library('testutil', libtestutil_sources,
          include_directories: ['..'])

e = executable('set_tests', 'set_tests.c',
           include_directories: ['..'],
           link_with: [libpsys, libtestutil])
test('set_tests', e)
e = executable('inst_tests', 'inst_tests.c',
           include_directories: ['..'],
           link_with: [libpsys, libtestutil])
test('inst_tests', e)
e = executable('img_tests', 'img_tests.c',
           include_directories: ['..'],
           link_with: [libpsys, libgame, libtestutil])
test('img_tests', e, workdir: meson.project_source_root())
