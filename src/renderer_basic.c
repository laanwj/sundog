/*
 * Copyright (c) 2023 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
/* Basic renderer with nearest pixel filter */
#include "game_renderer.h"

#include "game/game_screen.h"
#include "glutil.h"
#include "util/memutil.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

struct renderer_basic {
    struct game_renderer base;

    GLuint scr_program;
    GLint scr_program_tint;
    GLuint scr_tex;
    GLuint pal_tex;
    GLuint vtx_buf;
};

static inline struct renderer_basic *renderer_basic(struct game_renderer *base)
{
    return (struct renderer_basic *)base;
}

static void basic_draw(struct game_renderer *renderer_, const float tint[4])
{
    struct renderer_basic *renderer = (struct renderer_basic *)renderer_;

    glUseProgram(renderer->scr_program);

    glUniform4fv(renderer->scr_program_tint, 1, tint);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->scr_tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderer->pal_tex);

    glBindBuffer(GL_ARRAY_BUFFER, renderer->vtx_buf);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (const GLvoid *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (const GLvoid *)(sizeof(float) * 2));
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}

static void basic_update_texture(struct game_renderer *renderer_, const uint8_t *buffer)
{
    struct renderer_basic *renderer = (struct renderer_basic *)renderer_;
    glBindTexture(GL_TEXTURE_2D, renderer->scr_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer);
}

static void basic_update_palette(struct game_renderer *renderer_, const uint16_t *palette)
{
    struct renderer_basic *renderer = (struct renderer_basic *)renderer_;
    uint8_t palette_img[SCREEN_COLORS * 4];

    glBindTexture(GL_TEXTURE_2D, renderer->pal_tex);
    for (int x = 0; x < SCREEN_COLORS; ++x) {
        /* atari ST paletter color is 0x0rgb, convert to RGBA */
        unsigned red           = (palette[x] >> 8) & 7;
        unsigned green         = (palette[x] >> 4) & 7;
        unsigned blue          = (palette[x] >> 0) & 7;
        palette_img[x * 4 + 0] = (red << 5) | (red << 2) | (red >> 1);
        palette_img[x * 4 + 1] = (green << 5) | (green << 2) | (green >> 1);
        palette_img[x * 4 + 2] = (blue << 5) | (blue << 2) | (blue >> 1);
        palette_img[x * 4 + 3] = 255;
#if 0
        printf("%d 0x%02x 0x%02x 0x%02x 0x%02x\n",
                x,
                palette_img[x * 4 + 0],
                palette_img[x * 4 + 1],
                palette_img[x * 4 + 2],
                palette_img[x * 4 + 3]);
#endif
    }
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCREEN_COLORS, 1, GL_RGBA, GL_UNSIGNED_BYTE, palette_img);
}

static void basic_destroy(struct game_renderer *renderer_)
{
    struct renderer_basic *renderer = renderer_basic(renderer_);
    glDeleteShader(renderer->scr_program);
    glDeleteTextures(1, &renderer->scr_tex);
    glDeleteTextures(1, &renderer->pal_tex);
    glDeleteBuffers(1, &renderer->vtx_buf);
    free(renderer);
}

static void init_gl(struct renderer_basic *renderer)
{
    /* Simple textured quad, for use with triangle strip */
    static float quad[] = {
        -1.0f, -1.0f, 0.0f, 1.0f,
        +1.0f, -1.0f, 1.0f, 1.0f,
        -1.0f, +1.0f, 0.0f, 0.0f,
        +1.0f, +1.0f, 1.0f, 0.0f
    };
    /* Create textures */
    /*  Screen texture is 320x200 8-bit luminance texture */
    glGenTextures(1, &renderer->scr_tex);
    glBindTexture(GL_TEXTURE_2D, renderer->scr_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

    /*  Palette is 256x1 32-bit RGBA texture. The width is kept at
     *  256 despite there being 16 colors to have direct byte value mapping without needing
     *  scaling in the fragment shading. The other 240 colors will be left unused.
     */
    glGenTextures(1, &renderer->pal_tex);
    glBindTexture(GL_TEXTURE_2D, renderer->pal_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    /* Create shaders */
    GLuint p1 = glCreateProgram();
    GLuint s2 = load_shader_resource("shaders/screen.vert", GL_VERTEX_SHADER);
    GLuint s3 = load_shader_resource("shaders/screen.frag", GL_FRAGMENT_SHADER);
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
    GLint u_pal_tex = glGetUniformLocation(p1, "pal_tex");
    glUniform1i(u_pal_tex, 1);
    renderer->scr_program_tint = glGetUniformLocation(p1, "tint");
    assert(glGetError() == GL_NONE);
    renderer->scr_program = p1;

    /* Build vertex buffer */
    glGenBuffers(1, &renderer->vtx_buf);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vtx_buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

struct game_renderer *new_renderer_basic(void)
{
    struct renderer_basic *renderer = CALLOC_STRUCT(renderer_basic);
    renderer->base.draw             = basic_draw;
    renderer->base.update_texture   = basic_update_texture;
    renderer->base.update_palette   = basic_update_palette;
    renderer->base.destroy          = basic_destroy;

    init_gl(renderer);

    return &renderer->base;
}
