gen_glxw = custom_target('gen-glxw',
                         input: ['../glxw_gen.py'],
                         output: ['glxw_es3.h', 'glxw_es3.c'],
                         command: [prog_python,
                                   '@INPUT0@',
                                   '--api', 'gles3',
                                   '-I', '@SOURCE_ROOT@/thirdparty/opengl',
                                   '-o', '@OUTDIR@',
                                  ])

