#include "util_img.h"

#include "psys/psys_debug.h"
#include "util/util_minmax.h"

#include <string.h>

void util_img_decompress_image(uint8_t *dest, uint8_t *src, unsigned width, unsigned height, unsigned *srcsize_out)
{
    unsigned sptr = 0, dptr = 0;
    unsigned end = width * height;
    unsigned i, count, pixel;
    while (dptr < end) {
        uint8_t in1 = src[sptr++];
        if (in1 & 0x80) { /* Count follows */
            count = src[sptr++];
            if (in1 & 0x40) { /* Two-byte BE count */
                count = (count << 8) | src[sptr++];
            }
            count += 1;
            if (in1 & 0x10) {
                if (in1 & 0x20) { /* Pixels from previous line */
                    for (i = 0; i < count; ++i) {
                        dest[dptr] = dest[dptr - width];
                        dptr += 1;
                    }
                    dest[dptr++] = flip4(in1 & 15);
                } else { /* H4L4 encoded pixels */
                    if (count & 1) {
                        dest[dptr++] = flip4(in1 & 15);
                    }
                    count >>= 1;
                    for (i = 0; i < count; ++i) {
                        uint8_t hl   = src[sptr++];
                        dest[dptr++] = flip4(hl >> 4);
                        dest[dptr++] = flip4(hl & 15);
                    }
                }
            } else { /* RLE */
                pixel = flip4(in1 & 15);
                for (i = 0; i < count; ++i) {
                    dest[dptr++] = pixel;
                }
            }
        } else { /* RLE 0xxxyyyy */
            count = (in1 >> 4) + 1;
            pixel = flip4(in1 & 15);
            for (i = 0; i < count; ++i) {
                dest[dptr++] = pixel;
            }
        }
    }
    if (dptr != end) {
        psys_panic("util_img_decompress_image: overshot ending by %d pixels\n", (int)(dptr - end));
    }
    if (srcsize_out) {
        *srcsize_out = sptr;
    }
}

void util_img_unplanarize(uint8_t *destdata, unsigned bytes_per_line,
    const uint8_t *planar,
    unsigned srcx, unsigned srcy,
    unsigned width, unsigned height, unsigned src_wdwidth)
{
    unsigned x, y;
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            unsigned sx                      = srcx + x;
            unsigned sy                      = srcy + y;
            unsigned bitid                   = ~sx & 7;
            unsigned byteid                  = (sx >> 3) & 1;
            unsigned unitofs                 = src_wdwidth * 8 * sy + (sx >> 4) * 8;
            unsigned bit0                    = (planar[unitofs + 0 + byteid] >> bitid) & 1;
            unsigned bit1                    = (planar[unitofs + 2 + byteid] >> bitid) & 1;
            unsigned bit2                    = (planar[unitofs + 4 + byteid] >> bitid) & 1;
            unsigned bit3                    = (planar[unitofs + 6 + byteid] >> bitid) & 1;
            destdata[y * bytes_per_line + x] = (bit3 << 3) | (bit2 << 2) | (bit1 << 1) | bit0;
        }
    }
}

void util_img_planarize(uint8_t *planar, unsigned dst_wdwidth, const uint8_t *srcdata, unsigned width, unsigned height, unsigned bytes_per_line)
{
    unsigned x, y, i, remainder;
    for (y = 0; y < height; ++y) {
        unsigned srcofs = bytes_per_line * y;
        unsigned dstofs = dst_wdwidth * 8 * y;
        for (x = 0; x < width; x += 16) {
            /* handle span of 16 pixels in 8 bytes */
            memset(&planar[dstofs], 0, 8);
            remainder = umin(16, width - x);
            for (i = 0; i < remainder; ++i) {
                unsigned pixel  = srcdata[srcofs + i];
                unsigned bitid  = ~i & 7;
                unsigned byteid = (i >> 3) & 1;
                if (pixel & 1) {
                    planar[dstofs + 0 + byteid] |= 1 << bitid;
                }
                if (pixel & 2) {
                    planar[dstofs + 2 + byteid] |= 1 << bitid;
                }
                if (pixel & 4) {
                    planar[dstofs + 4 + byteid] |= 1 << bitid;
                }
                if (pixel & 8) {
                    planar[dstofs + 6 + byteid] |= 1 << bitid;
                }
            }
            dstofs += 8;
            srcofs += 16;
        }
    }
}
