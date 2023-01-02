/*
 * Copyright (c) 2022 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "game_sound.h"

#include "psys/psys_debug.h"
#include "util/memutil.h"

#include <SDL_mixer.h>

#include <string.h>

/** Include generated extracted sound data structures. */
#include "sundog_soundfx.h"

struct sdl_sound {
    struct game_sound base;

    struct Mix_Chunk **samples;
};

static inline struct sdl_sound *sdl_sound(struct game_sound *base)
{
    return (struct sdl_sound *)base;
}

static void sdlsound_play_sound(struct game_sound *sound_, const uint8_t *data, size_t len)
{
    struct sdl_sound *sound = sdl_sound(sound_);
    size_t idx;

    for (idx = 0; idx < sundog_sound_fx_count; ++idx) {
        if (sundog_sound_fx[idx].len <= len && memcmp(data, sundog_sound_fx[idx].data, sundog_sound_fx[idx].len) == 0) {
            break;
        }
    }

    if (idx == sundog_sound_fx_count) {
        psys_debug("sdlsound_play_sound: No match found for sound\n");
        return;
    }

    if (idx == 0) {
        /* Sound off. */
        Mix_HaltChannel(0);
    } else if (sound->samples[idx]) {
        // psys_debug("sdlsound_play_sound: Playing %s\n", sundog_sound_fx[idx].name);
        Mix_PlayChannel(0, sound->samples[idx], 0);
    } else {
        // psys_debug("sdlsound_play_sound: Could not play %s, no sample loaded\n", sundog_sound_fx[idx].name);
    }
}

static void sdlsound_destroy(struct game_sound *sound_)
{
    struct sdl_sound *sound = sdl_sound(sound_);
    size_t idx;

    /* Just to be sure nothing is playing at the moment. */
    Mix_HaltChannel(0);

    for (idx = 0; idx < sundog_sound_fx_count; ++idx) {
        if (sound->samples[idx]) {
            Mix_FreeChunk(sound->samples[idx]);
        }
    }

    free(sound->samples);
    free(sound);
}

#define BUFLEN (200)
static void sdlsound_load_samples(struct sdl_sound *sound, game_sound_loader_func *loader, const char *samples_path)
{
    char temp_path[BUFLEN];
    size_t idx;
    sound->samples = calloc(sundog_sound_fx_count, sizeof(struct Mix_Chunk *));
    for (idx = 0; idx < sundog_sound_fx_count; ++idx) {
        size_t slen;
        /* Please, lecture me again how safe and completely normal C string handling is... */
        strncpy(temp_path, samples_path, BUFLEN);
        if (temp_path[BUFLEN - 1]) {
            psys_panic("sdlsound_load_samples: string buffer overrun\n");
        }
        slen = strnlen(temp_path, BUFLEN);
        if (slen > 0 && temp_path[slen - 1] != '/') {
            strncat(temp_path, "/", BUFLEN - 1);
        }
        strncat(temp_path, sundog_sound_fx[idx].name, BUFLEN - 1);
        strncat(temp_path, ".ogg", BUFLEN - 1);
        if (temp_path[BUFLEN - 2]) {
            psys_panic("sdlsound_load_samples: string buffer overrun\n");
        }
        SDL_RWops *resource = (SDL_RWops *)loader(temp_path);
        if (!resource) {
            psys_debug("sdlsound: Could not load \"%s\", sample will not be played\n", temp_path);
            continue;
        }
        sound->samples[idx] = Mix_LoadWAV_RW(resource, 1);
        if (!sound->samples[idx]) {
            psys_debug("sdlsound: Could not decode \"%s\", sample will not be played\n", temp_path);
        }
    }
}

struct game_sound *new_sdl_sound(game_sound_loader_func *loader, const char *samples_path)
{
    struct sdl_sound *sound = CALLOC_STRUCT(sdl_sound);
    sound->base.play_sound  = &sdlsound_play_sound;
    sound->base.destroy     = &sdlsound_destroy;

    sdlsound_load_samples(sound, loader, samples_path);

    return &sound->base;
}
