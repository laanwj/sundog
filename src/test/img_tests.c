/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "test_common.h"
#include "test_util.h"

#include "psys/psys_debug.h"
#include "util/util_img.h"

#include <stdio.h>

unsigned width  = 320;
unsigned height = 200;
uint8_t scratch[320 * 200 * 4];
uint8_t destdata[320 * 200];
uint8_t verify[320 * 200];

int main()
{
    size_t sourcedata_size;
    uint8_t *sourcedata = load_file("test/compress_source.bin", &sourcedata_size);
    size_t destdata_planar_size;
    uint8_t *destdata_planar = load_file("test/compress_dest.bin", &destdata_planar_size);
    unsigned ptr;
    unsigned size_consumed;

    if (sourcedata == NULL) {
        printf("Cannot load test data\n");
        return 1;
    }
    /* Convert target from planar to one byte per pixel */
    util_img_unplanarize(destdata, width, destdata_planar, 0, 0, width, height, ((width + 15) >> 4));
    util_img_planarize(verify, ((width + 15) >> 4), destdata, width, height, width);
    /* Test planarize */
    for (ptr = 0; ptr < destdata_planar_size; ++ptr) {
        if (verify[ptr] != destdata_planar[ptr]) {
            printf("planarize: Mismatch at offset 0x%04x\n", ptr);
            break;
        }
    }

    /* Test decompression */
    util_img_decompress_image(scratch, sourcedata, width, height, &size_consumed);
    for (ptr = 0; ptr < (width * height); ++ptr) {
        if (scratch[ptr] != destdata[ptr]) {
            printf("decompress: Mismatch at 0x%08x our %02x ref %02x\n", ptr,
                scratch[ptr], destdata[ptr]);
            break;
        }
    }
    if (ptr != (width * height)) {
        printf("our\n");
        psys_debug_hexdump_ofs(scratch + 0x1f0, 0x1f0, 0x10);
        printf("ref\n");
        psys_debug_hexdump_ofs(destdata + 0x1f0, 0x1f0, 0x10);
        return 1;
    }
    if (size_consumed != sourcedata_size) {
        printf("%d source bytes consumed of %d.", (int)size_consumed, (int)sourcedata_size);
        return 1;
    }

    return 0;
}
