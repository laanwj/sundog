/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#ifndef H_PSYS_TYPES
#define H_PSYS_TYPES

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct psys_state;

/** Standard types */
typedef uint16_t psys_word;
typedef uint8_t psys_byte;
typedef int16_t psys_sword;
typedef int8_t psys_sbyte;
/** Code pool address (index into p-system memory) */
typedef uint32_t psys_fulladdr;
/** Function used for tracing */
typedef void(psys_tracefunc)(struct psys_state *state, void *data);
/** Function used for caling native binding */
typedef void(psys_bindingfunc)(struct psys_state *state, void *data, psys_fulladdr segment, psys_fulladdr env_data);

/** 8-byte segment identifier.
 * This can be accessed either as 8 separate characters for printing, or as a
 * 64-bit integer for hashing/comparison.
 */
struct psys_segment_id {
    union {
        char name[8];
        uint64_t num;
    };
};

/** Structure used to represent a segment:procedure pair, e.g. a fully
 * qualified procedure identifier.
 */
struct psys_function_id {
    struct psys_segment_id seg;
    psys_byte proc_num;
};

/** Maximum size of sets. In the p-system IV, sets can be up to 256 words
 * (4096 bits).
 */
#define PSYS_MAX_SET_SIZE 256

/** Marker value for pointer return values if segment is not resident
 * or the lookup failed for another reason such as an out-of-range
 * segment or procedure number. VM error state will have been set
 * accordingly.
 */
static const psys_fulladdr PSYS_ADDR_ERROR = 0xffffffffUL;

#ifdef __cplusplus
}
#endif

#endif
