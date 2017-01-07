/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "psys_set.h"

#include "psys_helpers.h"
#include "util/util_minmax.h"

#include <string.h>

/* Read from set (in native endian) */
#define I(x, y) F((x)[(y)])
/* Write to set (in native endian) */
#define O(x, y, v) (x)[(y)] = F(v)
/* Get size of set */
#define S(x) (I((x), 0))
/* Set size of set */
#define SS(x, y) (O((x), 0, (y)))

unsigned psys_set_words(psys_word *s)
{
    return 1 + S(s);
}

bool psys_set_pop(struct psys_state *s, psys_word *out)
{
    const psys_word *in = psys_stack_words(s, 0);
    if (S(in) > PSYS_MAX_SET_SIZE)
        return false;
    memcpy(out, in, 2 + S(in) * 2);
    psys_pop_n(s, 1 + S(in));
    return true;
}

#ifdef PSYS_TRIM_SETS
/* find first non-zero element, starting from the end,
 * to find the optimal set size.
 */
static inline void psys_set_trim(psys_word *inout)
{
    int setsize = S(inout);
    while (setsize > 0 && inout[setsize] == 0)
        setsize -= 1;
    SS(inout, setsize);
}
#endif

void psys_set_push(struct psys_state *s, const psys_word *in)
{
    psys_push_n(s, 1 + S(in));                         /* make room for enough words on stack */
    memcpy(psys_stack_words(s, 0), in, 2 + S(in) * 2); /* copy entire structure */
}

bool psys_set_adj(psys_word *inout, unsigned setsize)
{
    unsigned cursize;
    if (setsize > PSYS_MAX_SET_SIZE) {
        return false;
    }
    /* if requested size larger, pad with zeros */
    cursize = S(inout);
    while (cursize < setsize)
        inout[++cursize] = 0;
    SS(inout, setsize);
    return true;
}

bool psys_set_union(psys_word *out, const psys_word *in1, const psys_word *in2)
{
    unsigned x, outsize, in1size, in2size;
    /* output size of union is maximum */
    in1size = S(in1);
    in2size = S(in2);
    outsize = umax(in1size, in2size);
    if (outsize > PSYS_MAX_SET_SIZE)
        return false;
    SS(out, outsize);
    for (x = 1; x <= outsize; ++x) {
        psys_word in1word = x <= in1size ? in1[x] : 0;
        psys_word in2word = x <= in2size ? in2[x] : 0;
        out[x]            = in1word | in2word;
    }
#ifdef PSYS_TRIM_SETS
    psys_set_trim(out);
#endif
    return true;
}

bool psys_set_intersection(psys_word *out, const psys_word *in1, const psys_word *in2)
{
    unsigned x, outsize;
    /* output size of intersection is minimum */
    outsize = umin(S(in1), S(in2));
    if (outsize > PSYS_MAX_SET_SIZE)
        return false;
    SS(out, outsize);
    for (x = 1; x <= outsize; ++x) {
        out[x] = in1[x] & in2[x];
    }
#ifdef PSYS_TRIM_SETS
    psys_set_trim(out);
#endif
    return true;
}

bool psys_set_difference(psys_word *out, const psys_word *in1, const psys_word *in2)
{
    unsigned x, outsize, in2size;
    /* in2 can only clear bits, so output size is in1 at most */
    outsize = S(in1);
    in2size = S(in2);
    if (outsize > PSYS_MAX_SET_SIZE)
        return false;
    SS(out, outsize);
    for (x = 1; x <= outsize; ++x) {
        if (x <= in2size) {
            out[x] = in1[x] & ~in2[x];
        } else {
            out[x] = in1[x];
        }
    }
#ifdef PSYS_TRIM_SETS
    psys_set_trim(out);
#endif
    return true;
}

/* Return whether in1 is the same as in2 */
bool psys_set_is_equal(const psys_word *in1, const psys_word *in2)
{
    unsigned x, maxsize, in1size, in2size;
    in1size = S(in1);
    in2size = S(in2);
    maxsize = umax(in1size, in2size);
    for (x = 1; x <= maxsize; ++x) {
        psys_word in1word = x <= in1size ? in1[x] : 0;
        psys_word in2word = x <= in2size ? in2[x] : 0;
        if (in1word != in2word)
            return false;
    }
    return true;
}

bool psys_set_is_subset(const psys_word *in1, const psys_word *in2)
{
    unsigned x, in1size, in2size;
    /* Iterate only over size of in1, because the bits of in2
     * outside the range of in1 don't influence whether
     * in1 is a subset. */
    in1size = S(in1);
    in2size = S(in2);
    for (x = 1; x <= in1size; ++x) {
        psys_word in1word = in1[x];
        psys_word in2word = x <= in2size ? in2[x] : 0;
        if (in1word & ~in2word)
            return false;
    }
    return true;
}

bool psys_set_is_superset(const psys_word *in1, const psys_word *in2)
{
    unsigned x, in1size, in2size;
    /* Iterate only over size of in2, because the bits of in1
     * outside the range of in2 don't influence whether
     * in1 is a superset. */
    in1size = S(in1);
    in2size = S(in2);
    for (x = 1; x <= in2size; ++x) {
        psys_word in1word = x <= in1size ? in1[x] : 0;
        psys_word in2word = in2[x];
        if (~in1word & in2word)
            return false;
    }
    return true;
}

bool psys_set_in(const psys_word *in, unsigned x)
{
    return (x >> 4) < S(in) && (I(in, 1 + (x >> 4)) & BIT(x & 15));
}

bool psys_set_from_subrange(psys_word *out, unsigned a, unsigned b)
{
    unsigned x, outsize;
    if (a >= PSYS_MAX_SET_SIZE * 16 || b >= PSYS_MAX_SET_SIZE * 16)
        return false;
    if (b < a) { /* empty set if b<a */
        SS(out, 0);
        return true;
    }
    /* make set large enough to contain upper range */
    outsize = (b / 16) + 1;
    /* start with zeros */
    SS(out, outsize);
    memset(&out[1], 0, 2 * outsize);
    /* set bits that should be one */
    for (x = a; x <= b; ++x) {
        out[1 + (x >> 4)] |= F(BIT(x & 15));
    }
    return true;
}
