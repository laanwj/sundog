#include "swoosh.h"

#include "glutil.h"

#include <GLES2/gl2.h>
#include <SDL.h>

#include <assert.h>

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

static const size_t swoosh_nframes = 11;
static const int swoosh_frame_durations[11] = {10, 1, 1, 2, 2, 3, 3, 3, 3, 12, 40};

/** Load and show FTL swoosh animation. */
void swoosh(SDL_Window *window, const char *frames_path)
{
    size_t idx;
    size_t slen;
#define BUFLEN (200)
    char temp_path[BUFLEN];
    SDL_PixelFormat *target = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32);
    GLuint scr_tex, vtx_buf, scr_program;

    glGenTextures(1, &scr_tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scr_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    /* Create shader */
    GLuint p1 = glCreateProgram();
    GLuint s2 = glCreateShader(GL_VERTEX_SHADER);
    shader_source(s2, "vertex",
        "attribute mediump vec4 vertexCoord;\n"
        "attribute mediump vec2 vertexTexCoord;\n"
        "varying mediump vec2 texcoord;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vertexCoord;\n"
        "    texcoord = vertexTexCoord;\n"
        "}\n");
    glAttachShader(p1, s2);
    glDeleteShader(s2);
    GLuint s3 = glCreateShader(GL_FRAGMENT_SHADER);
    shader_source(s3, "fragment",
        "varying mediump vec2 texcoord;\n"
        "uniform sampler2D scr_tex;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = texture2D(scr_tex, texcoord);\n"
        "}\n");
    glAttachShader(p1, s3);
    glDeleteShader(s3);
    glBindAttribLocation(p1, 0, "vertexCoord");
    glBindAttribLocation(p1, 1, "vertexTexCoord");
    glLinkProgram(p1);
    glUseProgram(p1);
    GLint u_scr_tex = glGetUniformLocation(p1, "scr_tex");
    glUniform1i(u_scr_tex, 0);
    assert(glGetError() == GL_NONE);
    scr_program = p1;

    /* Build vertex buffer */
    static float quad[] = {
        -1.0f, -1.0f, 0.0f, 1.0f,
        +1.0f, -1.0f, 1.0f, 1.0f,
        -1.0f, +1.0f, 0.0f, 0.0f,
        +1.0f, +1.0f, 1.0f, 0.0f
    };
    glGenBuffers(1, &vtx_buf);
    glBindBuffer(GL_ARRAY_BUFFER, vtx_buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (const GLvoid *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (const GLvoid *)(sizeof(float) * 2));
    glEnableVertexAttribArray(1);

    /* Setup GL render */
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapWindow(window);

    for (idx = 0; idx < swoosh_nframes; ++idx) {
        /* Load frame */
        char istr[4];
        istr[0] = '0' + ((idx / 100) % 10);
        istr[1] = '0' + ((idx / 10) % 10);
        istr[2] = '0' + (idx % 10);
        istr[3] = 0;

        strncpy(temp_path, frames_path, BUFLEN);
        if (temp_path[BUFLEN-1]) {
            goto error;
        }
        slen = strnlen(temp_path, BUFLEN);
        if (slen > 0 && temp_path[slen - 1] != '/') {
            strncat(temp_path, "/", BUFLEN - 1);
        }
        strncat(temp_path, "frame", BUFLEN - 1);
        strncat(temp_path, istr, BUFLEN - 1);
        strncat(temp_path, ".bmp", BUFLEN - 1);
        if (temp_path[BUFLEN-2]) {
            goto error;
        }

        SDL_Surface *load = SDL_LoadBMP(temp_path);
        if (!load) {
            printf("Could not load swoosh animation frame %s\n", temp_path);
            goto error;
        }
        SDL_Surface *convert = SDL_ConvertSurface(load, target, 0);
        SDL_FreeSurface(load);
        if (!convert) {
            printf("Could not convert swoosh animation frame %s\n", temp_path);
            goto error;
        }
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, convert->w, convert->h, GL_RGBA, GL_UNSIGNED_BYTE, convert->pixels);
        SDL_FreeSurface(convert);

        /* Draw frame */
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        SDL_GL_SwapWindow(window);

        /* Sleep after frame */
        SDL_Delay(swoosh_frame_durations[idx] * 1000 / 50);
    }

error:
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glDeleteProgram(scr_program);
    glDeleteBuffers(1, &vtx_buf);
    glDeleteTextures(1, &scr_tex);
    SDL_FreeFormat(target);
}

