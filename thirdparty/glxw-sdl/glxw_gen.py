#!/usr/bin/env python3
# Copyright (c) 2012 Riku Salminen
# Copyright (c) 2014 Daniel Scharrer
# Copyright (c) 2023 W. J. van der Laan
# SPDX-License-Identifier: Zlib

import argparse
import os
import re

def parse_funcs(filename, regex_string, blacklist):
    print('Parsing header %s' % os.path.basename(filename))
    regex = re.compile(regex_string)
    group_re = re.compile(r'^#ifndef ((GL|WGL|GLX|EGL)_\w+)')
    with open(filename) as f:
        funcs = []
        group = None
        for line in f:
            match = group_re.match(line)
            if match is not None:
                group = match.group(1)
            match = regex.match(line)
            if match is not None:
                if group not in blacklist:
                    funcs.append(match.group(1))
        return funcs

def generate_header(api, funcs, api_includes, prefix, suffix, filename):
    print('Generating header %s' % filename)

    header = '''#ifndef glxw%(suffix)s_h
#define glxw%(suffix)s_h

''' % { 'suffix': suffix }

    common = '''

#ifdef __cplusplus
extern "C" {
#endif

int load%(suffix)s_procs(void);
''' % {
    'upper_suffix': suffix[1:].upper() if api == 'glx' or api == 'wgl' or api == 'egl' else '',
    'suffix': suffix
    }

    footer = '''
#ifdef __cplusplus
}
#endif

#endif
'''

    with open(filename, 'w') as f:
        f.write(header)
        if api == 'glx':
            f.write('#include <GL/%s.h>\n' % api)
        if api == 'wgl':
            f.write('#include <GL/glcorearb.h>\n')

        for include in api_includes:
            f.write('#include <%s>\n' % include)
        f.write(common);

        f.write('\nstruct glxw%s {\n' % suffix)
        for func in funcs:
            funptr = ('PFN%sPROC' % func.upper())
            f.write('    %s _%s;\n' % (funptr, func));
        f.write('};\n');

        f.write('\nextern struct glxw%s glxw%s;\n\n' % (suffix, suffix))

        for func in funcs:
            f.write('#define %s (glxw%s._%s)\n' % (func, suffix, func))
        f.write(footer)


def generate_library(api, funcs, api_includes, prefix, suffix, filename):
    print('Generating library source %s' % filename)

    body = '''
#include <SDL.h>

struct glxw%(suffix)s glxw%(suffix)s;

int load%(suffix)s_procs(void)
{
    struct glxw%(suffix)s *ctx = &glxw%(suffix)s;
''' % {
    'upper_suffix': suffix[1:].upper() if api == 'glx' or api == 'wgl' or api == 'egl' else '',
    'suffix': suffix
    }

    with open(filename, 'w') as f:
        f.write('#include <glxw/glxw%s.h>\n' % suffix)

        f.write(body)

        for func in funcs:
            f.write('    ctx->_%s = (%s)SDL_GL_GetProcAddress(\"%s\");\n' % (func, ('PFN%sPROC' % func.upper()), func))

        f.write('    return ctx->_glGetString != NULL;\n')
        f.write('}\n')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='''
OpenGL extension loader generator.
This script downloads OpenGL, OpenGL ES, EGL, WGL, GLX extension headers from
official sources and generates an extension loading library.
        ''')
    parser.add_argument('-I', '--include', type=str, metavar='DIR',
        help='Look for include files in directory')
    parser.add_argument('-o', '--output', type=str, metavar='DIR',
        help='Output directory')
    parser.add_argument('--api', type=str, metavar='API',
        choices=['opengl', 'wgl', 'glx', 'gles2', 'gles3', 'egl', 'khr'],
        help='Download only specified API')
    parser.add_argument('--all', action='store_true',
        help='Download and generate all APIs')
    args = parser.parse_args()

    all_apis = args.all or args.api is None
    include_dir = args.include if args.include is not None else \
        os.path.join(os.getcwd(), 'include')
    output_dir = args.output if args.output is not None else \
        os.getcwd()

    apis = [
        ('opengl', 'gl', '', 'GL', r'GLAPI.*APIENTRY\s+(\w+)',
            [],
            [(True, 'glcorearb.h', 'http://www.opengl.org/registry/api/glcorearb.h')]),
        ('wgl', 'wgl', '_wgl', 'GL', r'extern.*WINAPI\s+(\w+)',
            [],
            [(True, 'wglext.h', 'http://www.opengl.org/registry/api/wglext.h')]),
        ('glx', 'glX', '_glx', 'GL', r'extern\s*\S*\s+(\w+)\s*\(',
            ['GLX_SGIX_video_source',
                'GLX_SGIX_fbconfig',
                'GLX_SGIX_dmbuffer',
                'GLX_VERSION_1_4',
                'GLX_ARB_get_proc_address'],
            [(True, 'glxext.h', 'http://www.opengl.org/registry/api/glxext.h')]),
        ('gles2', 'gl', '_es2', 'GLES2', r'GL_APICALL.*GL_APIENTRY\s+(\w+)',
            [],
            [(False, 'gl2.h', 'http://www.khronos.org/registry/gles/api/GLES2/gl2.h'),
            (False, 'gl2platform.h', 'http://www.khronos.org/registry/gles/api/GLES2/gl2platform.h'),
            (True, 'gl2ext.h', 'http://www.khronos.org/registry/gles/api/GLES2/gl2ext.h')]),
        ('gles3', 'gl', '_es3', 'GLES3',  r'GL_APICALL.*GL_APIENTRY\s+(\w+)',
            [],
            [(True, 'gl3.h', 'http://www.khronos.org/registry/gles/api/GLES3/gl3.h'),
            (False, 'gl3platform.h', 'http://www.khronos.org/registry/gles/api/GLES3/gl3platform.h')]),
        ('egl', 'egl', '_egl', 'EGL', r'EGLAPI.*EGLAPIENTRY\s+(\w+)',
            [],
            [(False, 'egl.h', 'http://www.khronos.org/registry/egl/api/EGL/egl.h'),
            (True, 'eglext.h', 'http://www.khronos.org/registry/egl/api/EGL/eglext.h'),
            (False, 'eglplatform.h', 'http://www.khronos.org/registry/egl/api/EGL/eglplatform.h')]),
        ('khr', None, '_khr', 'KHR', None,
            [],
            [(False, 'khrplatform.h', 'http://www.khronos.org/registry/egl/api/KHR/khrplatform.h')])

        ]

    platform_regex = re.compile('^.*platform\.h')

    for (api, prefix, suffix, directory, regex, blacklist, headers) in apis:
        if not all_apis and (api != args.api and \
            not (api == 'khr' and (args.api == 'gles2' or args.api == 'gles3' or args.api == 'egl'))):
            continue

        funcs = []

        if regex is not None:
            for (contains_funcs, filename, source_url) in headers:
                if contains_funcs:
                    funcs += parse_funcs(os.path.join(args.include, directory, filename), regex, blacklist)

        if len(funcs) > 0:
            api_includes = ['%s/%s' % (directory, header) for (f, header, url) in headers]

            include_file = os.path.join(output_dir, 'glxw%s.h' % suffix)
            source_file = os.path.join(output_dir, 'glxw%s.c' % suffix)

            generate_header(api, funcs, api_includes, prefix, suffix, include_file)
            generate_library(api, funcs, api_includes, prefix, suffix, source_file)

