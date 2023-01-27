gen_debuginfo = custom_target('gen-debuginfo',
                             input: debug_info,
                             output: 'game_debuginfo.h',
                             command: [prog_python,
                                       '@INPUT0@',
                                       '@OUTPUT@'
                                      ])
