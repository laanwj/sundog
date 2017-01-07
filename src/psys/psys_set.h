/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#ifndef H_PSYS_SET
#define H_PSYS_SET

#include "psys_types.h"

/* Used for declaration only */
typedef psys_word psys_set[PSYS_MAX_SET_SIZE + 1];

/* Get size in words of set (including length word) */
extern unsigned psys_set_words(psys_word *s);

/* Pop set from stack - returns true on success,
 * false if the set is too large to fit.
 */
extern bool psys_set_pop(struct psys_state *s, psys_word *out);

/* Push set to stack (only pushes as many words as necessary) */
extern void psys_set_push(struct psys_state *s, const psys_word *out);

/* Adjust set size to setsize, padding or truncating it as necessary. */
extern bool psys_set_adj(psys_word *inout, unsigned setsize);

/* Set union (logical OR).
 * out cannot overlap with in1 or in2 */
extern bool psys_set_union(psys_word *out, const psys_word *in1, const psys_word *in2);

/* Set union (logical AND).
 * out cannot overlap with in1 or in2 */
extern bool psys_set_intersection(psys_word *out, const psys_word *in1, const psys_word *in2);

/* Set difference (in1 AND NOT in2)
 * out cannot overlap with in1 or in2 */
extern bool psys_set_difference(psys_word *out, const psys_word *in1, const psys_word *in2);

/* Return whether in1 is the same as in2 */
extern bool psys_set_is_equal(const psys_word *in1, const psys_word *in2);

/* Return whether in1 is a subset of in2.
 * This means that all bits of in1 must be set in in2.
 */
extern bool psys_set_is_subset(const psys_word *in1, const psys_word *in2);

/* Return whether in1 is a superset of in2.
 * This means that all bits of in2 must be set in in1.
 */
extern bool psys_set_is_superset(const psys_word *in1, const psys_word *in2);

/* Return whether x is a member of in */
extern bool psys_set_in(const psys_word *in, unsigned x);

/* Create set from subrange */
extern bool psys_set_from_subrange(psys_word *out, unsigned a, unsigned b);

#endif
