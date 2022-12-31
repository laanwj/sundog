#include "glutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void shader_source(GLuint shader, const char *name, const char *shader_str)
{
    GLint size = strlen(shader_str);
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
}
