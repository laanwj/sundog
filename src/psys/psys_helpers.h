/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
/* Helpers and utilities for bit twiddling */
#ifndef H_PSYS_HELPERS
#define H_PSYS_HELPERS

#include "psys_interpreter.h"
#include "psys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BIT
#define BIT(x) (1 << (x))
#endif
#ifndef W
/* Word offset - this is a macro to make it easier to switch to word-based
 * addressing (except for instruction pointer) at some point.
 */
#define W(base, ofs) ((base) + ((ofs)*2))
#endif
#ifdef FLIP_ENDIAN_HACK
/* Flip endian */
#define F(x) psys_flip_endian(x)
#else
/* Don't flip endian */
#define F(x) (x)
#endif

/* Sign extend byte */
static inline int signext_byte(psys_byte x)
{
    return (signed char)x;
}

/* Sign extend word */
static inline int signext_word(psys_word x)
{
    return (int16_t)x;
}

/* Flip non-native to native endian (or vice versa) */
static inline psys_word psys_flip_endian(psys_word x)
{
    return (x >> 8) | (x << 8);
}

/* Memory access */

/* Load byte from (word addr,offset).
 * Like ldp this uses word address + offset format, to make it possible
 * to access individual bytes even if memory is addressed in words.
 */
static inline psys_byte psys_ldb(const struct psys_state *state, psys_fulladdr addr, psys_fulladdr offset)
{
    return state->memory[addr + offset];
}
/* Load unsigned word from word addr */
static inline psys_word psys_ldw(const struct psys_state *state, psys_fulladdr addr)
{
    return F(*((psys_word *)&state->memory[addr]));
}
/* Loads signed word from word addr */
static inline psys_sword psys_ldsw(const struct psys_state *state, psys_fulladdr addr)
{
    return signext_word(psys_ldw(state, addr));
}
/* Load unsigned word from word addr, optionally flip endian
 * This is useful when loading values from constant pools.
 */
static inline psys_word psys_ldw_flip(const struct psys_state *state, psys_fulladdr addr, bool flip)
{
    psys_word rv = psys_ldw(state, addr);
    if (flip) {
        return psys_flip_endian(rv);
    } else {
        return rv;
    }
}
/* Load signed word from word addr, optionally flip endian.
 */
static inline psys_sword psys_ldsw_flip(const struct psys_state *state, psys_fulladdr addr, bool flip)
{
    psys_word rv = psys_ldw(state, addr);
    if (flip) {
        return signext_word(psys_flip_endian(rv));
    } else {
        return signext_word(rv);
    }
}
/* Store byte to (word addr, offset) */
static inline void psys_stb(struct psys_state *state, psys_fulladdr addr, psys_word offset, psys_byte value)
{
    state->memory[addr + offset] = value;
}
/* Store word to word addr */
static inline void psys_stw(struct psys_state *state, psys_fulladdr addr, psys_word value)
{
    *((psys_word *)&state->memory[addr]) = F(value);
}
/* Get direct access to bytes in memory */
static inline psys_byte *psys_bytes(struct psys_state *state, psys_fulladdr addr)
{
    return (psys_byte *)&state->memory[addr];
}
/* Get direct access to words in memory */
static inline psys_word *psys_words(struct psys_state *state, psys_fulladdr addr)
{
    return (psys_word *)&state->memory[addr];
}

/* push a word to the stack */
static inline void psys_push(struct psys_state *state, psys_word x)
{
    state->sp -= 2;
    psys_stw(state, state->sp, x);
}
/* pop an unsigned word from the stack */
static inline psys_word psys_pop(struct psys_state *state)
{
    psys_word rv = psys_ldw(state, state->sp);
    state->sp += 2;
    return rv;
}
/* pop a signed word from the stack */
static inline psys_sword psys_spop(struct psys_state *state)
{
    return signext_word(psys_pop(state));
}
/* pop multiple word from the stack */
static inline void psys_pop_n(struct psys_state *state, int n)
{
    state->sp += n * 2;
}
/* push multiple word to the stack */
static inline void psys_push_n(struct psys_state *state, int n)
{
    state->sp -= n * 2;
}
/* Get direct access to stack words in memory */
static inline psys_word *psys_stack_words(struct psys_state *state, psys_fulladdr offset)
{
    return psys_words(state, state->sp + offset * 2);
}

#ifdef __cplusplus
}
#endif

#endif
