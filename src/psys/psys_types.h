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

struct psys_state;

/* standard types */
typedef uint16_t psys_word;
typedef uint8_t psys_byte;
typedef int16_t psys_sword;
typedef int8_t psys_sbyte;
/* code pool address */
typedef uint32_t psys_fulladdr;
/* host pointer */
typedef void *psys_hostptr;
/* native function call */
typedef void(psys_tracefunc)(struct psys_state *state, void *data);
/* binding call */
typedef void(psys_bindingfunc)(struct psys_state *state, void *data, psys_fulladdr segment, psys_fulladdr env_data);

/* 8-byte segment identifier */
struct psys_segment_id {
    union {
        char name[8];
        uint64_t num;
    };
};

struct psys_function_id {
    struct psys_segment_id seg;
    psys_byte proc_num;
};

/* Sets can be up to 256 words */
#define PSYS_MAX_SET_SIZE 256

/* Marker value for pointer return values if segment is not resident
 * or the lookup failed for another reason such as an out-of-range
 * segment or procedure number. VM error state will have been set
 * accordingly.
 */
static const psys_fulladdr PSYS_ADDR_ERROR = 0xffffffffUL;

#endif
