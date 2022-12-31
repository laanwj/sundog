#ifndef H_GLUTIL
#define H_GLUTIL

#include <GLES2/gl2.h>

/** Compile GLSL shader from source. */
void shader_source(GLuint shader, const char *name, const char *shader_str);

#endif
