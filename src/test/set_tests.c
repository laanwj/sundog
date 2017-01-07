/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "psys/psys_helpers.h"
#include "psys/psys_set.h"
#include "test_common.h"

#include <stdio.h>
#include <string.h>

#ifdef FLIP_ENDIAN_HACK
/* Flip endian */
#define F(x) psys_flip_endian(x)
#else
/* Don't flip endian */
#define F(x) (x)
#endif
/* Read from set (in native endian) */
#define I(x, y) F((x)[(y)])
/* Write to set (in native endian) */
#define O(x, y, v) (x)[(y)] = F(v)

int main()
{
    psys_set a, b, c, d, full, empty;
    CHECK(psys_set_from_subrange(empty, 4095, 0));
    CHECK(psys_set_from_subrange(full, 0, 4095));

    /* subrange test 1 */
    psys_set_from_subrange(a, 0, 15);
    for (int x = 0; x < 16; ++x) {
        CHECK(psys_set_in(a, x));
    }
    CHECK(!psys_set_in(a, 16));
    CHECK_EQUAL(I(a, 0), 1);

    /* subrange test 2 */
    psys_set_from_subrange(a, 0, 31);
    for (int x = 0; x < 31; ++x) {
        CHECK(psys_set_in(a, x));
    }
    CHECK(!psys_set_in(a, 32));
    CHECK_EQUAL(I(a, 0), 2);

    /* subrange test 3 */
    psys_set_from_subrange(a, 8, 9);
    for (int x = 8; x <= 9; ++x) {
        CHECK(psys_set_in(a, x));
    }
    CHECK(!psys_set_in(a, 0));
    CHECK_EQUAL(I(a, 0), 1);

    /* subrange test 4 */
    psys_set_from_subrange(a, 0, 4095);
    for (int x = 0; x <= 4095; ++x) {
        CHECK(psys_set_in(a, x));
    }
    CHECK(!psys_set_in(a, 4096));
    CHECK_EQUAL(I(a, 0), 256);

    /* subrange test 5 */
    CHECK(!psys_set_from_subrange(a, 0, 4096));

    /* set union */
    psys_set_from_subrange(a, 0, 2047);
    psys_set_from_subrange(b, 2048, 4095);
    psys_set_union(c, a, b);
    for (int x = 0; x <= 4095; ++x) {
        CHECK(psys_set_in(c, x));
    }

    /* set intersection */
    psys_set_from_subrange(a, 0, 2047 + 128);
    psys_set_from_subrange(b, 2048 - 128, 4095);
    psys_set_intersection(c, a, b);
    for (int x = 0; x < 4097; ++x) {
        CHECK_EQUAL(psys_set_in(c, x), x >= (2048 - 128) && x <= (2047 + 128));
    }

    /* set difference */
    psys_set_from_subrange(a, 0, 4095);
    psys_set_from_subrange(b, 2048, 4095);
    psys_set_difference(c, a, b);
    for (int x = 0; x < 4097; ++x) {
        CHECK_EQUAL(psys_set_in(c, x), x >= 0 && x <= 2047);
    }

    psys_set_from_subrange(a, 0, 127);
    psys_set_from_subrange(b, 2048, 4095);
    psys_set_difference(c, a, b);
    for (int x = 0; x < 4097; ++x) {
        CHECK_EQUAL(psys_set_in(c, x), x >= 0 && x <= 127);
    }

    psys_set_from_subrange(a, 256, 767);
    psys_set_from_subrange(b, 2048, 4095);
    psys_set_difference(c, a, b);
    for (int x = 0; x < 4097; ++x) {
        CHECK_EQUAL(psys_set_in(c, x), x >= 256 && x <= 767);
    }

    /* empty set */
    psys_set_from_subrange(c, 4095, 0);

    for (int x = 0; x < 4097; ++x) {
        CHECK_EQUAL(psys_set_in(c, x), 0);
    }

    /* pattern */
    psys_set_from_subrange(c, 4095, 0);

    for (int x = 0; x < 4096; ++x) {
        if (x % 2) {
            psys_set_from_subrange(a, x, x);
            memcpy(b, c, I(c, 0) * 2 + 2); /* make copy, cannot do union in-place */
            psys_set_union(c, a, b);
        }
    }
    for (int x = 0; x < 4096; ++x) {
        CHECK_EQUAL(psys_set_in(c, x), x % 2);
    }

    /* adjust */
    CHECK(psys_set_from_subrange(c, 0, 4095));
    CHECK(psys_set_adj(c, 16));
    CHECK_EQUAL(I(c, 0), 16);
    CHECK(psys_set_adj(c, 32));
    CHECK_EQUAL(I(c, 0), 32);
    CHECK(psys_set_adj(c, 0));
    CHECK_EQUAL(I(c, 0), 0);
    CHECK(psys_set_adj(c, 256));
    CHECK_EQUAL(I(c, 0), 256);
    CHECK(!psys_set_adj(c, 257));

    /* equals / subset / superset */
    CHECK(psys_set_from_subrange(a, 0, 4095));
    CHECK(psys_set_from_subrange(b, 0, 4095));
    CHECK(psys_set_from_subrange(c, 4095, 0));
    CHECK(psys_set_is_equal(a, b));
    CHECK(!psys_set_is_equal(a, c));
    CHECK(psys_set_is_subset(a, b));   /* equal is als subset */
    CHECK(psys_set_is_superset(a, b)); /* equal is als superset */
    CHECK(psys_set_is_subset(c, a));   /* empty set is subset of a */
    CHECK(psys_set_is_superset(a, c)); /* a is superset of empty set */

    CHECK(psys_set_from_subrange(a, 128, 128));
    CHECK(psys_set_from_subrange(b, 256, 256));
    CHECK(psys_set_union(c, a, b));
    CHECK(psys_set_from_subrange(d, 768, 768));

    CHECK(psys_set_is_subset(a, c));
    CHECK(psys_set_is_subset(b, c));

    CHECK(psys_set_is_superset(c, a));
    CHECK(psys_set_is_superset(c, b));

    CHECK(!psys_set_is_subset(d, a));
    CHECK(!psys_set_is_subset(d, b));
    CHECK(!psys_set_is_superset(d, a));
    CHECK(!psys_set_is_superset(d, b));

    CHECK(psys_set_is_subset(a, full));
    CHECK(psys_set_is_subset(b, full));
    CHECK(psys_set_is_subset(empty, a));
    CHECK(psys_set_is_subset(empty, b));
    CHECK(psys_set_is_superset(full, a));
    CHECK(psys_set_is_superset(full, b));
    CHECK(psys_set_is_superset(a, empty));
    CHECK(psys_set_is_superset(b, empty));

    CHECK(!psys_set_is_equal(full, empty));
    CHECK(psys_set_is_equal(full, full));
    CHECK(psys_set_is_equal(empty, empty));
    CHECK_EQUAL(I(full, 0), 256);
    CHECK_EQUAL(I(empty, 0), 0);
}
