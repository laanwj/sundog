/*
 * Copyright (c) 2023 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
/* GL utilities. */
#ifndef H_GLUTIL
#define H_GLUTIL

#include <GLES3/gl3.h>

#include <stdbool.h>
#include <unistd.h>

/** Compile GLSL shader from resource. */
GLuint load_shader_resource(const char *name, GLenum shader_type);

/** Load texture from resource. */
GLuint load_texture_resource(const char *name);

/** Load paletted image of fixed size as-is into byte array (one byte per element). */
bool load_paletted(const char *name, void *data, size_t width, size_t height, uint8_t *colors);

/** Return true if OpenGL ES is at least version 3. */
bool check_for_GLES3(void);

/* Compute viewport of maximum size that preserves aspect ratio.
 * Returns {xbase, ybase, width, height}.
 */
void compute_viewport_fixed_ratio(int width, int height, int desired_width, int desired_height, int viewport_out[4]);

/** Make viewport maximum size that preserves aspect ratio. */
void gl_viewport_fixed_ratio(int width, int height, int desired_width, int desired_height);

#endif
