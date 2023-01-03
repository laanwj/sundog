/**
 * Convert Atari ST XBIOS dosound format to a .wav file.
 *
 * W.J. van der Laan 2021
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "emu2149.h"

// #define DEBUG

struct wave_header {
    // Riff Wave Header
    char chunkId[4];
    uint32_t chunkSize;
    char format[4];

    // Format Subchunk
    char subChunk1Id[4];
    uint32_t subChunk1Size;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;

    // Data Subchunk
    char subChunk2Id[4];
    uint32_t subChunk2Size;
};

struct wave_header make_wave_header(int sampleRate, int numChannels, int bitsPerSample) {
    struct wave_header myHeader;

    // RIFF WAVE Header
    myHeader.chunkId[0] = 'R';
    myHeader.chunkId[1] = 'I';
    myHeader.chunkId[2] = 'F';
    myHeader.chunkId[3] = 'F';
    myHeader.format[0] = 'W';
    myHeader.format[1] = 'A';
    myHeader.format[2] = 'V';
    myHeader.format[3] = 'E';

    // Format subchunk
    myHeader.subChunk1Id[0] = 'f';
    myHeader.subChunk1Id[1] = 'm';
    myHeader.subChunk1Id[2] = 't';
    myHeader.subChunk1Id[3] = ' ';
    myHeader.audioFormat = 1; // FOR PCM
    myHeader.numChannels = numChannels; // 1 for MONO, 2 for stereo
    myHeader.sampleRate = sampleRate; // ie 44100 hertz, cd quality audio
    myHeader.bitsPerSample = bitsPerSample; //
    myHeader.byteRate = myHeader.sampleRate * myHeader.numChannels * myHeader.bitsPerSample / 8;
    myHeader.blockAlign = myHeader.numChannels * myHeader.bitsPerSample/8;

    // Data subchunk
    myHeader.subChunk2Id[0] = 'd';
    myHeader.subChunk2Id[1] = 'a';
    myHeader.subChunk2Id[2] = 't';
    myHeader.subChunk2Id[3] = 'a';

    // All sizes for later:
    // chuckSize = 4 + (8 + subChunk1Size) + (8 + subChubk2Size)
    // subChunk1Size is constanst, i'm using 16 and staying with PCM
    // subChunk2Size = nSamples * nChannels * bitsPerSample/8
    // Whenever a sample is added:
    //    chunkSize += (nChannels * bitsPerSample/8)
    //    subChunk2Size += (nChannels * bitsPerSample/8)
    myHeader.chunkSize = 4+8+16+8+0;
    myHeader.subChunk1Size = 16;
    myHeader.subChunk2Size = 0;

    return myHeader;
}

uint8_t *unhexlify(const char *s, size_t *len_out) {
    size_t len = strlen(s);
    if (len & 1) return NULL;
    uint8_t *retval = malloc(len / 2);
    for (size_t ptr = 0; ptr < len; ++ptr) {
        size_t digit;
        if (s[ptr] >= '0' && s[ptr] <= '9') {
            digit = s[ptr] - '0';
        } else if (s[ptr] >= 'a' && s[ptr] <= 'f') {
            digit = s[ptr] - 'a' + 10;
        } else { // Parse error
            free(retval);
            return NULL;
        }
        if (ptr & 1) {
            retval[ptr / 2] |= digit;
        } else {
            retval[ptr / 2] = digit << 4;
        }
    }
    if (len_out) {
        *len_out = len / 2;
    }
    return retval;
}

void do_generate(struct wave_header *hdr, FILE *out, PSG *psg, int nsamples)
{
    for (int i = 0; i < nsamples; ++i) {
        int16_t sample = PSG_calc(psg);

        /* 16-bit samples are stored as 2's-complement signed integers, ranging from -32768 to 32767. */
        uint16_t sample_u = (uint16_t) sample;
        uint8_t sdata[2] = {sample_u & 0xff, sample_u >> 8};
        fwrite(sdata, sizeof(sdata), 1, out);
        hdr->chunkSize += sizeof(sdata);
        hdr->subChunk2Size += sizeof(sdata);
    }
}

int main(int argc, char **argv)
{
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "Usage: %s <out.wav> <hex> [<trailing-ticks>]\n", argv[0]);
        exit(1);
    }
    const int RATE = 44100;
    const int MAX_CYCLES = 256;
    const char *filename_out = argv[1];
    const char *data_hex = argv[2];
    int trailing_ticks = 0;
    if (argc > 3)
        trailing_ticks = atoi(argv[3]);

    FILE *out = fopen(filename_out, "wb");
    struct wave_header hdr = make_wave_header(RATE, 1, 16);
    fwrite(&hdr, sizeof(hdr), 1, out);

    PSG * psg = PSG_new(2000000, RATE);
    PSG_setVolumeMode(psg, EMU2149_VOL_YM2149);
    PSG_set_quality (psg, 1); // high quality (i guess)
    PSG_reset(psg);

/*
Command         Meaning

0x0x byte       Write byte into register x of the sound chip
0x80 byte       Load bytes into temporary register for use by command 0x81
0x81 byte1      Number of the sound register into which the value of the temporary register is to be transferred.
     byte2      Signed value to be added to the temporary register until value in third byte is met.
     byte3      Value of the temporary register at which the loop is terminated.
0x82..0xff byte The number of 50 Hz samples_per_tick (20 ms) to wait for the next command. If the byte holds the value 0 the processing is terminated.
*/
    size_t len;
    const uint8_t *data = unhexlify(data_hex, &len);
    if (!data) {
        fprintf(stderr, "Error parsing hex: %s\n", data_hex);
        exit(1);
    }

    int samples_per_tick = RATE / 50;
    size_t ptr = 0;
    uint8_t tmp = 0;
    while (ptr < len) {
        uint8_t op = data[ptr++];
        if (op < 0x10) {
            if (ptr == len) {
                fprintf(stderr, "Out of range while reading sound chip register argument\n");
                exit(1);
            }
            uint8_t val = data[ptr++];
            PSG_writeReg(psg, op, val);
#ifdef DEBUG
            printf("reg[0x%02x]=%02x\n", op, val);
#endif
        } else if (op == 0x80) {
            if (ptr == len) {
                fprintf(stderr, "Out of range while reading temporary register argument\n");
                exit(1);
            }
            tmp = data[ptr++];
#ifdef DEBUG
            printf("tmp=%02x\n", tmp);
#endif
        } else if (op == 0x81) {
            if ((ptr + 2) >= len) {
                fprintf(stderr, "Out of range while reading repeat arguments\n");
                exit(1);
            }
            uint8_t regnr = data[ptr++];
            uint8_t delta = data[ptr++];
            uint8_t endval = data[ptr++];
#ifdef DEBUG
            printf("repeat[%02x,%02x,%02x]\n", regnr, delta, endval);
#endif
            int cycles = 0; // cut off endlessly repeating sounds after a while
            do {
#ifdef DEBUG
                printf("  reg[0x%02x]=%02x\n", regnr, tmp);
#endif
                PSG_writeReg(psg, regnr, tmp);
                tmp += delta;
                do_generate(&hdr, out, psg, samples_per_tick);
                cycles += 1;
            } while (tmp != endval && cycles < MAX_CYCLES);
            if (cycles == MAX_CYCLES) {
                printf("warning: reached max cycles\n");
                break;
            }
        } else if (op >= 0x82) {
            if (ptr == len) {
                fprintf(stderr, "Out of range while reading wait argument\n");
                exit(1);
            }
            uint8_t wait = data[ptr++];
            if (wait == 0) {
                break;
            }
            do_generate(&hdr, out, psg, wait * samples_per_tick);
        } else {
            fprintf(stderr, "Unknown op %02x at offset %i\n", op, (int)ptr);
            exit(1);
        }
    }

    do_generate(&hdr, out, psg, samples_per_tick * trailing_ticks);

    /* re-write header with final sizes. */
    (void)fseek(out, 0, SEEK_SET);
    fwrite(&hdr, sizeof(hdr), 1, out);

    fclose(out);

    return 0;
}
