/*
 * Copyright (c) 2023 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "glutil.h"

#include "sundog_resources.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GLuint load_shader_resource(const char *name, GLenum shader_type)
{
    GLuint shader = glCreateShader(shader_type);

    size_t size_res;
    const char *shader_str = load_resource(name, &size_res);

    if (!shader_str) {
        printf("Error: shader %s load failed!\n", name);
        exit(1);
    }

    GLint size = size_res;
    GLint ret;

    glShaderSource(shader, 1, &shader_str, &size);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ret);

    if (!ret) {
        char *log;
        printf("Error: shader %s compilation failed!:\n", name);
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &ret);

        if (ret > 1) {
            log = malloc(ret);
            glGetShaderInfoLog(shader, ret, NULL, log);
            printf("%s", log);
        }
        exit(1);
    }
    unload_resource(shader_str);
    return shader;
}
