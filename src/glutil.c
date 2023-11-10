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
    SDL_RWops *resource    = load_resource_sdl(name);
    const char *shader_str = NULL;
    if (resource) {
        shader_str = (const char *)SDL_LoadFile_RW(resource, &size_res, true);
    }

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
    SDL_free((void *)shader_str);
    return shader;
}

GLuint load_texture_resource(const char *name)
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    SDL_RWops *resource = load_resource_sdl(name);
    if (!resource) {
        glDeleteTextures(1, &tex);
        printf("Error: texture %s load failed!\n", name);
        return 0;
    }
    SDL_Surface *load = SDL_LoadBMP_RW(resource, 1);
    if (!load) {
        glDeleteTextures(1, &tex);
        printf("Error: texture %s decode failed!\n", name);
        return 0;
    }
    if (load->format->format == SDL_PIXELFORMAT_RGBA4444) { /* Match GL format) directly. */
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA4, load->w, load->h, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, load->pixels);
    } else { /* Convert to RGBA32 */
        SDL_PixelFormat *target = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32);
        SDL_Surface *convert    = SDL_ConvertSurface(load, target, 0);
        SDL_FreeSurface(load);
        if (!convert) {
            glDeleteTextures(1, &tex);
            printf("Error: texture %s conversion failed!\n", name);
            return 0;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, convert->w, convert->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, convert->pixels);
        SDL_FreeSurface(convert);
        SDL_FreeFormat(target);
    }
    return tex;
}

/* XXX this is not actually a GL utilitity function. */
bool load_paletted(const char *name, void *data, size_t width, size_t height, uint8_t *colors)
{
    SDL_RWops *resource = load_resource_sdl(name);
    if (!resource) {
        printf("Error: texture %s load failed!\n", name);
        return false;
    }
    SDL_Surface *load = SDL_LoadBMP_RW(resource, 1);
    if (!load) {
        printf("Error: texture %s decode failed!\n", name);
        return false;
    }

    if (!load->format->palette || (size_t)load->w != width || (size_t)load->h != height) {
        printf("Error: texture %s is not paletted or wrong size\n", name);
        SDL_FreeSurface(load);
        return false;
    }
    SDL_PixelFormat *target = SDL_AllocFormat(SDL_PIXELFORMAT_INDEX8);
    SDL_SetPixelFormatPalette(target, load->format->palette);
    SDL_Surface *convert = SDL_ConvertSurface(load, target, 0);
    SDL_FreeSurface(load);
    if (!convert) {
        printf("Error: texture %s conversion failed!\n", name);
        return false;
    }

    /* Export bitmap. */
    memcpy(data, convert->pixels, width * height);

    /* Export palette. */
    for (int idx = 0; idx < 16; ++idx) {
        if (idx < target->palette->ncolors) {
            *(colors++) = target->palette->colors[idx].r;
            *(colors++) = target->palette->colors[idx].g;
            *(colors++) = target->palette->colors[idx].b;
            *(colors++) = target->palette->colors[idx].a;
        } else {
            *(colors++) = 0;
            *(colors++) = 0;
            *(colors++) = 0;
            *(colors++) = 0;
        }
    }
    SDL_FreeSurface(convert);
    SDL_FreeFormat(target);
    return true;
}

bool check_for_GLES3(void)
{
    int maj;
    glGetError(); /* clear error flag first */
    glGetIntegerv(GL_MAJOR_VERSION, &maj);
    if (glGetError() || maj < 3) {
        return false;
    }
    return true;
}

void compute_viewport_fixed_ratio(int width, int height, int desired_width, int desired_height, int viewport_out[4])
{
    float ratio     = (float)desired_width / (float)desired_height;
    float altheight = width / ratio;
    float altwidth  = ratio * height;
    if (altheight <= height) {
        viewport_out[0] = 0;
        viewport_out[1] = (height - altheight) / 2.0;
        viewport_out[2] = width;
        viewport_out[3] = altheight;
    } else {
        viewport_out[0] = (width - altwidth) / 2.0;
        viewport_out[1] = 0;
        viewport_out[2] = altwidth;
        viewport_out[3] = height;
    }
}

void gl_viewport_fixed_ratio(int width, int height, int desired_width, int desired_height)
{
    int viewport[4];
    compute_viewport_fixed_ratio(width, height, desired_width, desired_height, viewport);
    glViewport(viewport[0], viewport[1], viewport[2] * PLATFORM_ASPECT_RATIO_CORRECTION,
               viewport[3] * PLATFORM_ASPECT_RATIO_CORRECTION);
}
