/*
 * Copyright (c) 2023 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
/* hq4x renderer -- needs GLES 3. */
#include "game_renderer.h"

#include "game/game_screen.h"
#include "glutil.h"
#include "util/memutil.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

struct renderer_hq4x {
    struct game_renderer base;

    GLuint scr_program;
    GLint scr_program_pal;
    GLint scr_program_tint;
    GLuint scr_tex;
    GLuint aux_tex;
    GLuint vtx_buf;
};

static inline struct renderer_hq4x *renderer_hq4x(struct game_renderer *base)
{
    return (struct renderer_hq4x *)base;
}

static void hq4x_draw(struct game_renderer *renderer_, const float tint[4])
{
    struct renderer_hq4x *renderer = (struct renderer_hq4x *)renderer_;

    glUseProgram(renderer->scr_program);

    glUniform4fv(renderer->scr_program_tint, 1, tint);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->scr_tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderer->aux_tex);

    glBindBuffer(GL_ARRAY_BUFFER, renderer->vtx_buf);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (const GLvoid *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (const GLvoid *)(sizeof(float) * 2));
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

static void hq4x_update_texture(struct game_renderer *renderer_, const uint8_t *buffer)
{
    struct renderer_hq4x *gs = (struct renderer_hq4x *)renderer_;
    /* Build screen texture */
    glBindTexture(GL_TEXTURE_2D, gs->scr_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_BYTE, buffer);
}

static void hq4x_update_palette(struct game_renderer *renderer_, const uint16_t *palette)
{
    struct renderer_hq4x *renderer = (struct renderer_hq4x *)renderer_;
    /* Update palette uniforms, after converting ST format to RGBA */
    float palette_img[SCREEN_COLORS * 4];
    for (int x = 0; x < SCREEN_COLORS; ++x) {
        /* atari ST paletter color is 0x0rgb, convert to RGBA */
        unsigned red           = (palette[x] >> 8) & 7;
        unsigned green         = (palette[x] >> 4) & 7;
        unsigned blue          = (palette[x] >> 0) & 7;
        palette_img[x * 4 + 0] = red / 7.0;
        palette_img[x * 4 + 1] = green / 7.0;
        palette_img[x * 4 + 2] = blue / 7.0;
        palette_img[x * 4 + 3] = 1.0;
    }
    glUseProgram(renderer->scr_program);
    glUniform4fv(renderer->scr_program_pal, SCREEN_COLORS, palette_img);
    glUseProgram(0);
}

static void hq4x_destroy(struct game_renderer *renderer_)
{
    struct renderer_hq4x *renderer = renderer_hq4x(renderer_);
    glDeleteShader(renderer->scr_program);
    glDeleteTextures(1, &renderer->scr_tex);
    glDeleteTextures(1, &renderer->aux_tex);
    glDeleteBuffers(1, &renderer->vtx_buf);
    free(renderer);
}

static void init_gl(struct renderer_hq4x *renderer, const char *vert_name, const char *frag_name, const char *param_texture)
{
    /* Simple textured quad, for use with triangle strip */
    static float quad[] = {
        -1.0f, -1.0f, 0.0f, SCREEN_HEIGHT,
        +1.0f, -1.0f, SCREEN_WIDTH, SCREEN_HEIGHT,
        -1.0f, +1.0f, 0.0f, 0.0f,
        +1.0f, +1.0f, SCREEN_WIDTH, 0.0f
    };
    /* Create textures */
    /*  Screen texture is 320x200 8-bit integer texture */
    glGenTextures(1, &renderer->scr_tex);
    glBindTexture(GL_TEXTURE_2D, renderer->scr_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, NULL);
    assert(glGetError() == GL_NONE);

    if (param_texture) {
        /* Param texture for HQ4x shader. */
        renderer->aux_tex = load_texture_resource(param_texture);
    }

    /* Create shaders */
    GLuint p1 = glCreateProgram();
    GLuint s2 = load_shader_resource(vert_name, GL_VERTEX_SHADER);
    GLuint s3 = load_shader_resource(frag_name, GL_FRAGMENT_SHADER);
    glAttachShader(p1, s2);
    glDeleteShader(s2);
    glAttachShader(p1, s3);
    glDeleteShader(s3);
    glBindAttribLocation(p1, 0, "vertexCoord");
    glBindAttribLocation(p1, 1, "vertexTexCoord");
    glLinkProgram(p1);
    glUseProgram(p1);
    GLint u_scr_tex = glGetUniformLocation(p1, "scr_tex");
    glUniform1i(u_scr_tex, 0);
    GLint u_aux_tex = glGetUniformLocation(p1, "aux_tex");
    glUniform1i(u_aux_tex, 1);
    renderer->scr_program_pal  = glGetUniformLocation(p1, "pal");
    renderer->scr_program_tint = glGetUniformLocation(p1, "tint");
    assert(glGetError() == GL_NONE);
    renderer->scr_program = p1;

    /* Build vertex buffer */
    glGenBuffers(1, &renderer->vtx_buf);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vtx_buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

struct game_renderer *new_renderer_hq4x(void)
{
    if (!check_for_GLES3()) {
        return NULL;
    }
    struct renderer_hq4x *renderer = CALLOC_STRUCT(renderer_hq4x);
    renderer->base.draw            = hq4x_draw;
    renderer->base.update_texture  = hq4x_update_texture;
    renderer->base.update_palette  = hq4x_update_palette;
    renderer->base.destroy         = hq4x_destroy;

    init_gl(renderer, "shaders/screen-hq4x.vert", "shaders/screen-hq4x.frag", "shaders/hq4x.bmp");

    return &renderer->base;
}

struct game_renderer *new_renderer_hqish(void)
{
    if (!check_for_GLES3()) {
        return NULL;
    }
    struct renderer_hq4x *renderer = CALLOC_STRUCT(renderer_hq4x);
    renderer->base.draw            = hq4x_draw;
    renderer->base.update_texture  = hq4x_update_texture;
    renderer->base.update_palette  = hq4x_update_palette;
    renderer->base.destroy         = hq4x_destroy;

    init_gl(renderer, "shaders/screen-hqish.vert", "shaders/screen-hqish.frag", NULL);

    return &renderer->base;
}
