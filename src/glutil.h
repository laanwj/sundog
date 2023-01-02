/*
 * Copyright (c) 2023 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
/* GL utilities. */
#ifndef H_GLUTIL
#define H_GLUTIL

#include <GLES2/gl2.h>

/** Compile GLSL shader from resource. */
GLuint load_shader_resource(const char *name, GLenum shader_type);

#endif
