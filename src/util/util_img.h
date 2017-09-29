#ifndef H_UTIL_IMG
#define H_UTIL_IMG

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Flip 4 bits to the opposite bit order */
static inline unsigned flip4(unsigned x)
{
    return (((x >> 3) & 1) << 0) | (((x >> 2) & 1) << 1) | (((x >> 1) & 1) << 2) | (((x >> 0) & 1) << 3);
}

/* Double every bit in word w. E.g.
 * 1010101010101010 -> 11001100110011001100110011001100
 */
static inline uint32_t double_bits16(uint32_t x)
{
    x = ((x & 0xff00ff00)<<8) | (x & 0x00ff00ff);
    x = ((x & 0xf0f0f0f0)<<4) | (x & 0x0f0f0f0f);
    x = ((x & 0xcccccccc)<<2) | (x & 0x33333333);
    x = ((x & 0xaaaaaaaa)<<1) | (x & 0x55555555);
    return x | (x<<1);
}

/* Decompress color image from src in compressed format to dest in
 * one-byte-per-pixel format. Needs width*height bytes space
 * at dest.
 */
extern void util_img_decompress_image(uint8_t *dest, uint8_t *src, unsigned width, unsigned height, unsigned *srcsize_out);

/** Convert planar image to 1-byte-per-pixel grid */
extern void util_img_unplanarize(uint8_t *destdata, unsigned bytes_per_line, const uint8_t *planar, unsigned srcx, unsigned srcy, unsigned width, unsigned height, unsigned src_wdwidth);

/** Convert 1-byte-per-pixel grid image to planar */
extern void util_img_planarize(uint8_t *planar, unsigned dst_wdwidth, const uint8_t *srcdata, unsigned width, unsigned height, unsigned bytes_per_line);

#ifdef __cplusplus
}
#endif

#endif
