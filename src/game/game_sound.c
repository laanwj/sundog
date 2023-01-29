/*
 * Copyright (c) 2022 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "game_sound.h"

#include "psys/psys_debug.h"
#include "util/memutil.h"

#include "emu2149.h"
#include <SDL.h>

#include <string.h>

#define RATE (22050)
#define CMDBUFSIZE (256)
//#define SOUND_DEBUG

struct sdl_sound {
    struct game_sound base;

    int samples_per_tick;
    PSG *psg;

    uint8_t cmd[CMDBUFSIZE];
    size_t cmd_ptr;
    size_t cmd_len;
    uint8_t tmp;

    int gen_count;
};

static inline struct sdl_sound *sdl_sound(struct game_sound *base)
{
    return (struct sdl_sound *)base;
}

static void sdlsound_callback(void *sound_, uint8_t *stream, int len)
{
    struct sdl_sound *sound = sdl_sound(sound_);
    int ptr = 0;
    len /= 2; /* size in samples */

    /* Generate samples */
    while (ptr < len) {
        /* Process commands. */
        while (sound->cmd_ptr < sound->cmd_len && sound->gen_count == 0) {
            size_t ofs = sound->cmd_ptr;
            uint8_t op = sound->cmd[sound->cmd_ptr++];
            if (op < 0x10) {
                if (sound->cmd_ptr == sound->cmd_len) {
                    printf("sdlsound: Out of range while reading sound chip register argument\n");
                } else {
                    uint8_t val = sound->cmd[sound->cmd_ptr++];
                    PSG_writeReg(sound->psg, op, val);
#ifdef SOUND_DEBUG
                    printf("%d reg[0x%02x]=%02x\n", (int)ofs, op, val);
#endif
                }
            } else if (op == 0x80) {
                if (sound->cmd_ptr == sound->cmd_len) {
                    printf("sdlsound: Out of range while reading temporary register argument\n");
                } else {
                    sound->tmp = sound->cmd[sound->cmd_ptr++];
#ifdef SOUND_DEBUG
                    printf("%d tmp=%02x\n", (int)ofs, sound->tmp);
#endif
                }
            } else if (op == 0x81) {
                if ((sound->cmd_ptr + 2) >= sound->cmd_len) {
                    printf("sdlsound: Out of range while reading repeat arguments\n");
                    sound->cmd_ptr = sound->cmd_len;
                } else {
                    uint8_t regnr = sound->cmd[sound->cmd_ptr++];
                    uint8_t delta = sound->cmd[sound->cmd_ptr++];
                    uint8_t endval = sound->cmd[sound->cmd_ptr++];
#ifdef SOUND_DEBUG
                    printf("%d repeat[%02x,%02x,%02x]\n", (int)ofs, regnr, delta, endval);
#endif
#ifdef SOUND_DEBUG
                    printf("  reg[0x%02x]=%02x\n", regnr, sound->tmp);
#endif
                    PSG_writeReg(sound->psg, regnr, sound->tmp);
                    sound->tmp += delta;
                    sound->gen_count = sound->samples_per_tick;
#ifdef SOUND_DEBUG
                    printf("  gen %d\n", sound->gen_count);
#endif

                    if (sound->tmp != endval) {
                        /* rewind command if we've not reached the end state */
                        sound->cmd_ptr = ofs;
                    }
                }
            } else if (op >= 0x82) {
                if (sound->cmd_ptr == sound->cmd_len) {
                    printf("sdlsound: Out of range while reading wait argument\n");
                } else {
                    uint8_t wait = sound->cmd[sound->cmd_ptr++];
                    if (wait == 0) {
                        /* end */
                        sound->cmd_ptr = sound->cmd_len;
                    } else {
                        sound->gen_count = (int)wait * sound->samples_per_tick;
#ifdef SOUND_DEBUG
                        printf("%d gen %d\n", (int)ofs, sound->gen_count);
#endif
                    }
                }
            } else {
                printf("sdlsound: Unknown op %02x at offset %i\n", op, (int)ptr);
                sound->cmd_ptr = sound->cmd_len;
            }
        }

        /* Generate sample. */
        ((int16_t*)stream)[ptr] = PSG_calc(sound->psg);
        ptr += 1;
        if (sound->gen_count > 0) {
            sound->gen_count -= 1;
        }
    }
}

static void sdlsound_play_sound(struct game_sound *sound_, const uint8_t *data, size_t len)
{
    struct sdl_sound *sound = sdl_sound(sound_);

    if (len > CMDBUFSIZE) {
        printf("sdlsound: Command buffer overflow (%d>%d)\n", (int)len, (int)CMDBUFSIZE);
        return;
    }
    SDL_LockAudioDevice(1);
    /* Completely replace the previous sound and reset state. */
    memcpy(sound->cmd, data, len);
    sound->cmd_ptr = 0;
    sound->cmd_len = len;
    sound->tmp = 0;
    sound->gen_count = 0;
    PSG_reset(sound->psg);
    SDL_UnlockAudioDevice(1);
}

static void sdlsound_destroy(struct game_sound *sound_)
{
    struct sdl_sound *sound = sdl_sound(sound_);

    SDL_CloseAudio();
    PSG_delete(sound->psg);

    free(sound);
}

struct game_sound *new_sdl_sound(void)
{
    struct sdl_sound *sound = CALLOC_STRUCT(sdl_sound);
    SDL_AudioSpec wanted;

    sound->base.play_sound  = &sdlsound_play_sound;
    sound->base.destroy     = &sdlsound_destroy;

    sound->samples_per_tick = RATE / 50;

    sound->gen_count        = 0;

    SDL_zero(wanted);
    wanted.freq = RATE;
    wanted.format = AUDIO_S16;
    wanted.channels = 1;
    wanted.samples = 1024;
    wanted.callback = sdlsound_callback;
    wanted.userdata = sound;

    if (SDL_OpenAudio(&wanted, NULL) < 0) {
        free(sound);
        return NULL;
    }

    sound->psg = PSG_new(2000000, RATE);
    PSG_setVolumeMode(sound->psg, EMU2149_VOL_YM2149);
    PSG_set_quality(sound->psg, 1); // high quality (i guess)
    PSG_reset(sound->psg);

    SDL_PauseAudio(0); /* start processing */

    return &sound->base;
}
